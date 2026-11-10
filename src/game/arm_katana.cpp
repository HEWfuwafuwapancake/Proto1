#include "game/arm_katana.h"
#include "game/camera.h"
#include "game/fightermanager.h"
#include "game/fighter.h"
#include "game/stage.h"
#include "core/gametime.h"
#include "core/mathutil.h"
#include "physics/areacalc.h"
#include "physics/armcalc.h"
#include "physics/breachcalc.h"
#include "physics/grapplecalc.h"
#include "render/shape.h"

// 調整用の値
// 単押し：居合
static constexpr float IAI_SECONDS[] = { 0.3f, 0.12f, 0.3f }; // 納刀の構え・一閃・残心
static constexpr float IAI_PARRY_SECONDS = 0.2f; // 構えの出だしで攻撃を受け流せる秒数。
static constexpr float IAI_MOVE_SCALE = 0.3f; // 居合の間の歩く速さの倍率。
static constexpr float IAI_STEP_SPEED = 20.0f; // 一閃で前へ踏み込む速さ。
static constexpr float IAI_RADIUS = 3.0f;
static constexpr float IAI_HALF_ANGLE = XMConvertToRadians(60.0f);
static constexpr float IAI_HALF_HEIGHT = 1.5f; // 肩の高さから上下それぞれの当たる幅。
static constexpr float IAI_DAMAGE = 12.0f;
static constexpr float IAI_POWER = 2.4f;
static constexpr float IAI_COUNTER_DAMAGE = 20.0f; // 受け流したあとの一閃のダメージ %。
static constexpr float IAI_COUNTER_POWER = 3.2f;
static constexpr float IAI_UP = 0.6f; // 水平の単位方向に足す上向きの量。
// 長押し：狙った場所へ一直線に飛ぶ
static constexpr float BREACH_RANGE = 25.0f; // 狙える距離。
static constexpr float BREACH_SPEED = 60.0f; // 飛ぶ速さ。
static constexpr float BREACH_ARRIVE_DISTANCE = 0.5f; // これより近づいたら着いたことにする。
static constexpr float BREACH_RECOVER_SECONDS = 0.2f; // 着いたあとの休み。
static constexpr float BREACH_EYE_RATE = 0.8f; // 狙いの線の始まりの高さ（体の高さに対する割合）。
static constexpr float BREACH_RING_RADIUS = 1.0f; // 着地点に出す輪の半径。
static constexpr float RIFT_SECONDS = 1.0f; // 裂け目を残す秒数。
static constexpr float RIFT_HEIGHT = 2.4f;
static constexpr float RIFT_RADIUS = 0.07f;
static constexpr float RIFT_TRAIL_GAP = 1.0f; // 飛んだ線に並べる球の間隔。
static constexpr float RIFT_TRAIL_RADIUS = 0.06f;
static constexpr XMFLOAT4 RIFT_COLOR{ 0.6f, 0.25f, 1.0f, 1.0f };
// 描画（形は arm_shield_base・arm_knuckle_fist と、つばを原点にした刀の部品）
static constexpr float BLEND_SECONDS = 0.08f;
static constexpr float MODEL_SCALE = 0.08f * Arm::MODEL_SIZE;
static constexpr float ARM_LENGTH = 19.250364f; // arm_shield_base の長さ（cm）。
static constexpr float FIST_LENGTH = 11.7f; // arm_knuckle_fist の長さ（cm）。
static constexpr float FIST_SIZE = 0.9f; // 拳を描く倍率。
static constexpr float PALM_CENTER = 5.75f; // 手首から手のひらの中心までの距離（cm）。ここで刀を握る。
static constexpr float GRIP_HALF = 4.5f; // つかの中心からつばまでの距離（cm）。
static constexpr float BLADE_LENGTH = 38.7f; // arm_katana_blade の長さ（cm）。
static constexpr XMFLOAT4 BLADE_COLOR{ 0.85f, 0.88f, 0.92f, 1.0f };
static constexpr XMFLOAT4 GRIP_COLOR{ 0.12f, 0.12f, 0.16f, 1.0f };
static constexpr XMFLOAT4 SHEATH_COLOR{ 0.22f, 0.1f, 0.12f, 1.0f };
static constexpr float REST_SWORD_PITCH = XMConvertToRadians(-15.0f); // 待機のときの刀の上下の向き。
static constexpr float HIP_HEIGHT_RATE = 0.5f; // 構えで手を寄せる腰の高さ（体の高さに対する割合）。
static constexpr float HIP_SIDE = 0.2f; // 構えで手を寄せる、反対の腰の横の距離。
static constexpr float HIP_FORWARD = 0.25f; // 構えで手を寄せる、体の前への距離。
static constexpr float SHEATH_PITCH = XMConvertToRadians(-25.0f); // 納刀の刀の後ろへの傾き。
static constexpr float SWING_START = XMConvertToRadians(100.0f); // 一閃の始まりの反対側への角度。
static constexpr float SWING_END = XMConvertToRadians(70.0f); // 一閃の終わりの手の側への角度。
static constexpr float ARM_SWING_RATE = 0.6f; // 刀の振りに対する腕の振りの割合。
static constexpr float SLASH_ARM_PITCH = XMConvertToRadians(-20.0f); // 一閃の腕の上下の向き。
static constexpr float AIM_ARM_PITCH = XMConvertToRadians(-15.0f); // 狙っている間の腕の上下の向き。
static constexpr float AIM_SWORD_PITCH = XMConvertToRadians(30.0f); // 狙っている間の刀の上下の向き。
static constexpr float DASH_ARM_PITCH = XMConvertToRadians(-50.0f); // 飛んでいる間の腕の上下の向き。
static constexpr float DASH_SWORD_PITCH = XMConvertToRadians(-10.0f); // 飛んでいる間、後ろへ流す刀の上下の向き。
static constexpr float HIT_RUMBLE_SMALL = 0.6f;
static constexpr float HIT_RUMBLE_LARGE = 0.6f;
static constexpr float HIT_RUMBLE_SECONDS = 0.12f;
static constexpr float COUNTER_RUMBLE_SECONDS = 0.2f;

enum IAI_PHASE { IAI_STANCE, IAI_DRAW, IAI_RECOVER, IAI_PHASE_COUNT };

KatanaArm::KatanaArm(int hand, const ARM_LINKS& links) : Arm(hand, links)
{
	m_ArmModel = LoadModel("assets/models/arm_shield_base.obj");
	m_FistModel = LoadModel("assets/models/arm_knuckle_fist.obj");
	m_BladeModel = LoadModel("assets/models/arm_katana_blade.obj");
	m_GripModel = LoadModel("assets/models/arm_katana_grip.obj");
	m_SheathModel = LoadModel("assets/models/arm_katana_sheath.obj");
	Reset();
}

void KatanaArm::Reset()
{
	StopDash();
	m_Phase.Reset();
	m_Breach = BREACH_NONE;
	m_Countered = false;
	m_HasLanding = false;
	m_Straighten = 0.0f;
	m_RiftSeconds = 0.0f;
}

void KatanaArm::StartIai()
{
	GetFighters().ConsumeGuardedHit(GetSlot()); // 前の受け流しの記録を持ち越さない。
	m_Countered = false;
	m_Direction = GetAim(false);
	m_Phase.Start(IAI_STANCE);
}

void KatanaArm::BeginDraw()
{
	m_Direction = GetAim(false);
	m_Hits.Clear();
}

void KatanaArm::SlashTargets()
{
	bool hit = false;
	for (int i = 0; i < FIGHTER_COUNT; ++i) {
		const TENTACLE_TARGET target = GetFighters().GetTarget(i, GetSlot());
		if (!target.Available || m_Hits.Contains(i)
			|| !AreaCalcInSector(GetShoulder(), m_Direction, 0.0f, IAI_RADIUS, IAI_HALF_ANGLE, IAI_HALF_HEIGHT, target.Body)) continue;
		m_Hits.TryAdd(i);
		GetFighters().Launch(i, GetSlot(), AreaCalcLaunchDirection(m_Direction, IAI_UP),
			m_Countered ? IAI_COUNTER_DAMAGE : IAI_DAMAGE, m_Countered ? IAI_COUNTER_POWER : IAI_POWER);
		hit = true;
	}
	if (hit) Rumble(HIT_RUMBLE_SMALL, HIT_RUMBLE_LARGE, HIT_RUMBLE_SECONDS);
}

void KatanaArm::StepIai(float dt)
{
	if (m_Phase.IsIdle()) return;
	// 構えの出だしで攻撃を防いだら、すぐに強い一閃を返す。
	if (m_Phase.GetPhase() == IAI_STANCE && GetFighters().ConsumeGuardedHit(GetSlot())) {
		m_Countered = true;
		Rumble(HIT_RUMBLE_SMALL, HIT_RUMBLE_LARGE, COUNTER_RUMBLE_SECONDS);
		m_Phase.Start(IAI_DRAW);
		BeginDraw();
	}
	const ARM_PHASE_STEP step = m_Phase.Step(dt, IAI_SECONDS, IAI_PHASE_COUNT);
	if (step.Entered == IAI_DRAW) BeginDraw();
	if (m_Phase.GetPhase() == IAI_DRAW) SlashTargets();
}

void KatanaArm::UpdateLanding()
{
	const XMFLOAT3 p = GetPlayer().GetPosition();
	const XMFLOAT3 view = GetCamera().GetDirection();
	m_HasLanding = BreachCalcFindLanding(GetStage().GetStage(), { p.x, p.y + GetPlayer().GetHeight() * BREACH_EYE_RATE, p.z },
		{ view.x, view.y, view.z }, BREACH_RANGE, m_Landing);
}

void KatanaArm::StartDash()
{
	const XMFLOAT3 p = GetPlayer().GetPosition();
	m_RiftStart = { p.x, p.y, p.z };
	m_RiftEnd = m_Landing;
	m_RiftSeconds = RIFT_SECONDS;
	m_Direction = ArmCalcBlendDirection(GetForward(), Vec3Sub(m_Landing, m_RiftStart), 1.0f);
	m_Breach = BREACH_DASH;
	GetPlayer().SetPulled(true);
	Rumble(HIT_RUMBLE_SMALL, HIT_RUMBLE_LARGE, HIT_RUMBLE_SECONDS);
}

void KatanaArm::StopDash()
{
	if (m_Breach != BREACH_DASH) return;
	m_Breach = BREACH_NONE;
	GetPlayer().SetPulled(false);
}

void KatanaArm::StepBreach(const ARM_INPUT& input, float dt)
{
	if (m_Breach == BREACH_AIM) {
		UpdateLanding();
		if (input.Pressed) return;
		// 離したときに着地点があれば飛ぶ（もう一方の手や触手で動いている間は飛ばない）。
		if (m_HasLanding && !GetPlayer().IsPulled()) StartDash();
		else m_Breach = BREACH_NONE;
		m_HasLanding = false;
		return;
	}
	if (m_Breach == BREACH_DASH) {
		const XMFLOAT3 p = GetPlayer().GetPosition();
		VEC3 feet{ p.x, p.y, p.z };
		const bool arrived = GrappleCalcPull(feet, m_Landing, BREACH_SPEED, BREACH_ARRIVE_DISTANCE, dt);
		const VEC3 next = arrived ? m_Landing : feet;
		GetPlayer().SetPosition({ next.X, next.Y, next.Z });
		if (arrived) {
			StopDash();
			m_Breach = BREACH_RECOVER;
			m_BreachSeconds = BREACH_RECOVER_SECONDS;
		}
		return;
	}
	if (m_Breach == BREACH_RECOVER) {
		m_BreachSeconds -= dt;
		if (m_BreachSeconds <= 0.0f) m_Breach = BREACH_NONE;
	}
}

void KatanaArm::Update(const ARM_INPUT& input)
{
	const float dt = fmaxf(0.0f, GameTimeGetDeltaTime());
	if (GetPlayer().DidRespawn()) Reset();
	if (!IsActive()) {
		if (input.Tap) StartIai();
		else if (input.HoldStarted) m_Breach = BREACH_AIM;
	}
	StepIai(dt);
	StepBreach(input, dt);
	m_RiftSeconds = fmaxf(0.0f, m_RiftSeconds - dt);
	const float speed = dt / BLEND_SECONDS;
	m_Straighten = IsActive() ? fminf(1.0f, m_Straighten + speed) : fmaxf(0.0f, m_Straighten - speed);
}

// 今の動作の、腕と刀の向きを求める（待機のあいだは呼び出し側の値のまま）。
void KatanaArm::GetPose(VEC3& arm, VEC3& sword, bool& sheathed) const
{
	const float side = GetHand() == 0 ? -1.0f : 1.0f;
	const int phase = m_Phase.GetPhase();
	if (phase == IAI_STANCE) {
		// 反対の腰へ手を寄せ、刀は鞘に納めて後ろへ流す。
		const XMFLOAT3 p = GetPlayer().GetPosition();
		const float yaw = GetPlayer().GetYaw();
		const VEC3 hip{ p.x + sinf(yaw) * HIP_FORWARD - cosf(yaw) * side * HIP_SIDE, p.y + GetPlayer().GetHeight() * HIP_HEIGHT_RATE,
			p.z + cosf(yaw) * HIP_FORWARD + sinf(yaw) * side * HIP_SIDE };
		arm = Vec3Sub(hip, GetDrawShoulder());
		sword = ArmCalcPitchDirection(Vec3Scale(GetForward(), -1.0f), SHEATH_PITCH);
		sheathed = true;
	}
	else if (phase == IAI_DRAW || phase == IAI_RECOVER) {
		// 反対の腰から手の側へ、一気に振り抜く（残心は振り抜いた形のまま）。
		const float t = phase == IAI_DRAW ? fminf(1.0f, m_Phase.GetTime() / IAI_SECONDS[IAI_DRAW]) : 1.0f;
		const float angle = Lerp(-side * SWING_START, side * SWING_END, t);
		sword = AreaCalcRotateYaw(m_Direction, angle);
		arm = ArmCalcPitchDirection(AreaCalcRotateYaw(m_Direction, angle * ARM_SWING_RATE), SLASH_ARM_PITCH);
	}
	else if (m_Breach == BREACH_AIM) {
		arm = ArmCalcPitchDirection(GetForward(), AIM_ARM_PITCH);
		sword = ArmCalcPitchDirection(GetForward(), AIM_SWORD_PITCH);
	}
	else if (m_Breach != BREACH_NONE) {
		arm = ArmCalcPitchDirection(m_Direction, DASH_ARM_PITCH);
		sword = ArmCalcPitchDirection(Vec3Scale(m_Direction, -1.0f), DASH_SWORD_PITCH);
	}
}

void KatanaArm::DrawRift(const VEC3& feet) const
{
	ShapeDrawCylinder({ feet.X, feet.Y, feet.Z }, { feet.X, feet.Y + RIFT_HEIGHT, feet.Z }, RIFT_RADIUS, RIFT_COLOR);
}

void KatanaArm::Draw()
{
	const VEC3 restArm = GetRestDirection();
	const VEC3 restSword = ArmCalcPitchDirection(GetForward(), REST_SWORD_PITCH);
	VEC3 armPose = restArm;
	VEC3 swordPose = restSword;
	bool sheathed = false;
	GetPose(armPose, swordPose, sheathed);
	const VEC3 arm = ArmCalcBlendDirection(restArm, armPose, m_Straighten);
	const VEC3 sword = ArmCalcBlendDirection(restSword, swordPose, m_Straighten);
	const XMFLOAT4 color = GetColor(IsActive());
	const float scale = MODEL_SCALE * GetDrawScale();
	const float fistScale = scale * FIST_SIZE;
	const VEC3 wrist = DrawAlong(m_ArmModel, GetDrawShoulder(), arm, ARM_LENGTH, scale, color);
	// 拳の指の並び（モデルの X）を刀の向きにそろえ、つかを握って見せる。
	const VEC3 palmDown{ arm.Y * sword.Z - arm.Z * sword.Y, arm.Z * sword.X - arm.X * sword.Z, arm.X * sword.Y - arm.Y * sword.X };
	if (m_FistModel >= 0) {
		DrawPiece(m_FistModel, wrist, Vec3Add(wrist, Vec3Scale(arm, FIST_LENGTH * fistScale)), FIST_LENGTH, fistScale, color, 0.0f, palmDown);
	}
	const VEC3 grip = Vec3Add(wrist, Vec3Scale(arm, PALM_CENTER * fistScale));
	const VEC3 guard = Vec3Add(grip, Vec3Scale(sword, GRIP_HALF * scale));
	DrawAlong(m_BladeModel, guard, sword, BLADE_LENGTH, scale, BLADE_COLOR);
	DrawAlong(m_GripModel, guard, sword, BLADE_LENGTH, scale, GRIP_COLOR);
	if (sheathed) DrawAlong(m_SheathModel, guard, sword, BLADE_LENGTH, scale, SHEATH_COLOR);
	if (m_Breach == BREACH_AIM && m_HasLanding) DrawRangeRing(m_Landing, BREACH_RING_RADIUS, RIFT_COLOR);
	if (m_RiftSeconds > 0.0f) {
		DrawRift(m_RiftStart);
		DrawRift(m_RiftEnd);
		const VEC3 line = Vec3Sub(m_RiftEnd, m_RiftStart);
		const int dots = (int)(Vec3Length(line) / RIFT_TRAIL_GAP);
		for (int i = 1; i < dots; ++i) {
			const VEC3 point = Vec3Add(m_RiftStart, Vec3Scale(line, (float)i / dots));
			ShapeDrawSphere({ point.X, point.Y + RIFT_HEIGHT * 0.5f, point.Z }, RIFT_TRAIL_RADIUS, RIFT_COLOR);
		}
	}
}

bool KatanaArm::IsActive() const
{
	return !m_Phase.IsIdle() || m_Breach != BREACH_NONE;
}

bool KatanaArm::IsHolding() const
{
	return false;
}

float KatanaArm::GetMoveScale() const
{
	return m_Phase.IsIdle() ? 1.0f : IAI_MOVE_SCALE;
}

VEC3 KatanaArm::GetRushVelocity() const
{
	return m_Phase.GetPhase() == IAI_DRAW ? Vec3Scale(m_Direction, IAI_STEP_SPEED) : VEC3{};
}

GUARD KatanaArm::GetGuard() const
{
	const bool parry = m_Phase.GetPhase() == IAI_STANCE && m_Phase.GetTime() < IAI_PARRY_SECONDS;
	if (!parry && m_Breach != BREACH_DASH) return {};
	GUARD guard;
	guard.Kind = GUARD_ALL;
	return guard;
}
