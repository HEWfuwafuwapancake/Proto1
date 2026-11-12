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
#include "physics/risecalc.h"
#include "render/shape.h"

// 調整用の値
// 単押し：ただの斬撃（押したらすぐ振る。当たった瞬間にダメージ）
static constexpr float IAI_SECONDS[] = { 0.1f, 0.1f, 0.25f }; // 振りかぶり・斬る・休み
static constexpr float IAI_CLIP_SECONDS[] = { 8.0f / 24.0f, 6.0f / 24.0f, 12.0f / 24.0f }; // クリップの区切り（katana_iai の抜いて斬る動きを使う）
static constexpr float TAP_MOVE_SCALE = 0.6f; // 斬撃の間の歩く速さの倍率。
static constexpr float TAP_RADIUS = 2.2f;
static constexpr float TAP_HALF_ANGLE = XMConvertToRadians(60.0f);
static constexpr float TAP_DAMAGE = 8.0f;
static constexpr float TAP_POWER = 1.6f;
// 長押しで飛んで着いた瞬間の、居合の一閃（少しあとからダメージが入る）
static constexpr float IAI_ARRIVE_SECONDS = 0.12f; // 着いてから一閃の当たりを調べる秒数。
static constexpr float IAI_RADIUS = 3.0f;
static constexpr float IAI_HALF_ANGLE = XMConvertToRadians(60.0f);
static constexpr float IAI_HALF_HEIGHT = 1.5f; // 肩の高さから上下それぞれの当たる幅。
static constexpr float IAI_DAMAGE = 12.0f;
static constexpr float IAI_POWER = 2.4f;
static constexpr float IAI_UP = 0.6f; // 水平の単位方向に足す上向きの量。
static constexpr float IAI_DELAY_SECONDS = 0.4f; // 一閃が当たってから斬撃のダメージが入るまでの秒数。
// 場外から戻る：狙いに着地点がなければ、いちばん近い足場の上へ飛ぶ
static constexpr float RETURN_INSET = 1.0f; // 足場のふちから内側へ入れる距離。
static constexpr float IAI_SHEATH_RATE = 0.6f; // 残心のこの割合から鞘を見せる（納刀）。
static constexpr float SLASH_SHOW_SECONDS = 0.25f; // 斬撃の線を見せる秒数。
static constexpr float SLASH_LENGTH = 1.1f; // 斬撃の線の中心から端までの長さ。
static constexpr float SLASH_RADIUS = 0.05f; // 斬撃の線の太さ。
static constexpr float SLASH_MARK_LENGTH = 0.8f; // 斬撃が入るのを待つ間の、細い線の長さ。
static constexpr float SLASH_MARK_RADIUS = 0.012f;
static constexpr XMFLOAT4 SLASH_COLOR{ 1.0f, 0.95f, 0.85f, 1.0f };
static constexpr float SLASH_RUMBLE_SMALL = 1.0f;
static constexpr float SLASH_RUMBLE_LARGE = 1.0f;
static constexpr float SLASH_RUMBLE_SECONDS = 0.2f;
// 長押し：構えて、離すと空間を斬って狙った場所へ一直線に飛ぶ（アサルト）
static constexpr float BREACH_RANGE = 25.0f; // 狙える距離。
static constexpr float DASH_HIT_RADIUS = 0.9f;
static constexpr float DASH_DAMAGE = 6.0f;
static constexpr float DASH_POWER = 1.4f;
static constexpr float DASH_UP = 0.6f;
static constexpr float BREACH_SPEED = 60.0f; // 飛ぶ速さ。
static constexpr float BREACH_ARRIVE_DISTANCE = 0.5f; // これより近づいたら着いたことにする。
static constexpr float BREACH_RAISE_SECONDS = 0.25f; // 刀を構えるまでの秒数。
static constexpr float BREACH_CUT_SECONDS = 0.38f; // 飛ぶ前に空間を斬る秒数。
static constexpr float BREACH_DASH_POSE_SECONDS = 0.08f; // 飛ぶ形になるまでの秒数。
static constexpr float BREACH_RECOVER_SECONDS = 0.2f; // 着いたあとの休み。
static constexpr float BREACH_EYE_RATE = 0.8f; // 狙いの線の始まりの高さ（体の高さに対する割合）。
static constexpr float BREACH_RING_RADIUS = 1.0f; // 着地点に出す輪の半径。
static constexpr float BREACH_CLIP_SECONDS[] = { 8.0f / 24.0f, 12.0f / 24.0f, 4.0f / 24.0f, 10.0f / 24.0f }; // クリップの区切り（katana_assault。構え・斬る・飛ぶ・着地）
static constexpr float BREACH_PHASE_SECONDS[] = { BREACH_RAISE_SECONDS, BREACH_CUT_SECONDS, BREACH_DASH_POSE_SECONDS, BREACH_RECOVER_SECONDS };
static constexpr float RIFT_SECONDS = 1.0f; // 裂け目を残す秒数。
static constexpr float RIFT_HEIGHT = 2.4f;
static constexpr float RIFT_RADIUS = 0.07f;
static constexpr float RIFT_TRAIL_GAP = 1.0f; // 飛んだ線に並べる球の間隔。
static constexpr float RIFT_TRAIL_RADIUS = 0.06f;
static constexpr float CUT_FORWARD = 1.3f; // 体から斬った裂け目までの前への距離。
static constexpr float CUT_HEIGHT_RATE = 0.65f; // 裂け目の中心の高さ（体の高さに対する割合）。
static constexpr float CUT_HALF_LENGTH = 1.6f; // 裂け目の中心から端までの長さ。
static constexpr float CUT_SIDE = 0.65f; // 裂け目の斜めの横の量（上向きを 1 とする）。
static constexpr float CUT_RADIUS = 0.1f; // 裂け目の太さ。
static constexpr float CUT_CORE_RADIUS = 0.13f; // 裂け目の真ん中の明るい部分の太さ。
static constexpr float CUT_CORE_RATE = 0.45f; // 裂け目の長さのうち、明るい部分の割合。
static constexpr XMFLOAT4 RIFT_COLOR{ 0.6f, 0.25f, 1.0f, 1.0f };
static constexpr XMFLOAT4 RIFT_CORE_COLOR{ 0.95f, 0.9f, 1.0f, 1.0f };
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

void KatanaArm::CancelMotion()
{
	ReleasePull();
	m_Phase.Reset();
	m_Breach = BREACH_NONE;
	m_HasLanding = false;
	m_Straighten = 0.0f;
	m_RiftSeconds = 0.0f;
}

void KatanaArm::Reset()
{
	CancelMotion();
	m_Delayed.Reset();
	for (float& seconds : m_SlashSeconds) seconds = 0.0f;
}

void KatanaArm::Interrupt()
{
	CancelMotion();
	StepDelayedHits(fmaxf(0.0f, GameTimeGetDeltaTime()));
}

void KatanaArm::SetBreach(BREACH_STATE state)
{
	m_Breach = state;
	m_BreachTime = 0.0f;
}

void KatanaArm::StartIai()
{
	m_IaiConsumed = false;
	m_Direction = GetAim(false);
	m_Phase.Start(IAI_STANCE);
}

void KatanaArm::BeginDraw()
{
	m_Direction = GetAim(false);
	m_Hits.Clear();
}

// 単押しの斬撃：前の扇の相手に、当たった瞬間にダメージを入れる。
void KatanaArm::SlashTargets()
{
	bool hit = false;
	for (int i = 0; i < FIGHTER_COUNT; ++i) {
		const TENTACLE_TARGET target = GetFighters().GetTarget(i, GetSlot());
		if (!target.Available || m_Hits.Contains(i)
			|| !AreaCalcInSector(GetShoulder(), m_Direction, 0.0f, TAP_RADIUS, TAP_HALF_ANGLE, IAI_HALF_HEIGHT, target.Body)) continue;
		m_Hits.TryAdd(i);
		const bool applied = GetFighters().Launch(i, GetSlot(), AreaCalcLaunchDirection(m_Direction, IAI_UP), TAP_DAMAGE, TAP_POWER);
		if (applied && !m_IaiConsumed) {
			ConsumeUse();
			m_IaiConsumed = true;
		}
		m_SlashDirections[i] = m_Direction;
		m_SlashSeconds[i] = SLASH_SHOW_SECONDS;
		hit = true;
	}
	if (hit) Rumble(HIT_RUMBLE_SMALL, HIT_RUMBLE_LARGE, HIT_RUMBLE_SECONDS);
}

// 長押しで飛んで着いたときの居合の一閃：当たったことだけを覚え、少しあとから斬撃としてダメージを入れる。
void KatanaArm::IaiTargets()
{
	bool hit = false;
	for (int i = 0; i < FIGHTER_COUNT; ++i) {
		const TENTACLE_TARGET target = GetFighters().GetTarget(i, GetSlot());
		if (!target.Available || m_Hits.Contains(i)
			|| !AreaCalcInSector(GetShoulder(), m_IaiDirection, 0.0f, IAI_RADIUS, IAI_HALF_ANGLE, IAI_HALF_HEIGHT, target.Body)) continue;
		m_Hits.TryAdd(i);
		DELAYED_HIT slash;
		slash.Target = i;
		slash.Direction = AreaCalcLaunchDirection(m_IaiDirection, IAI_UP);
		slash.Damage = IAI_DAMAGE;
		slash.Power = IAI_POWER;
		slash.Seconds = IAI_DELAY_SECONDS;
		m_Delayed.Add(slash);
		m_SlashDirections[i] = m_IaiDirection;
		hit = true;
	}
	if (hit) Rumble(HIT_RUMBLE_SMALL, HIT_RUMBLE_LARGE, HIT_RUMBLE_SECONDS);
}

// 飛んだ通り道をまとめて調べ、高速移動でも相手を通り抜けて見逃さない。
void KatanaArm::DashTargets(const VEC3& from, const VEC3& to)
{
	const VEC3 height{ 0.0f, GetPlayer().GetHeight() * 0.5f, 0.0f };
	const CAPSULE path{ Vec3Add(from, height), Vec3Add(to, height), DASH_HIT_RADIUS };
	const VEC3 flat = ArmCalcPitchDirection(m_Direction, 0.0f);
	bool hit = false;
	for (int i = 0; i < FIGHTER_COUNT; ++i) {
		const TENTACLE_TARGET target = GetFighters().GetTarget(i, GetSlot());
		if (!target.Available || !CollisionCapsuleCapsule(path, target.Body).Hit || !m_Hits.TryAdd(i)) continue;
		GetFighters().Launch(i, GetSlot(), AreaCalcLaunchDirection(flat, DASH_UP), DASH_DAMAGE, DASH_POWER);
		m_SlashDirections[i] = flat;
		m_SlashSeconds[i] = SLASH_SHOW_SECONDS;
		hit = true;
	}
	if (hit) Rumble(HIT_RUMBLE_SMALL, HIT_RUMBLE_LARGE, HIT_RUMBLE_SECONDS);
}

void KatanaArm::StepDelayedHits(float dt)
{
	DELAYED_HIT hits[DELAYED_HIT_MAX];
	const int count = m_Delayed.Step(dt, hits, DELAYED_HIT_MAX);
	for (int i = 0; i < count; ++i) {
		if (!GetFighters().GetTarget(hits[i].Target, GetSlot()).Available) continue;
		const bool applied = GetFighters().Launch(hits[i].Target, GetSlot(), hits[i].Direction, hits[i].Damage, hits[i].Power);
		if (applied && !m_IaiConsumed) {
			ConsumeUse();
			m_IaiConsumed = true;
		}
		m_SlashSeconds[hits[i].Target] = SLASH_SHOW_SECONDS;
		Rumble(SLASH_RUMBLE_SMALL, SLASH_RUMBLE_LARGE, SLASH_RUMBLE_SECONDS);
	}
	for (float& seconds : m_SlashSeconds) seconds = fmaxf(0.0f, seconds - dt);
}

void KatanaArm::StepIai(float dt)
{
	if (m_Phase.IsIdle()) return;
	const ARM_PHASE_STEP step = m_Phase.Step(dt, IAI_SECONDS, IAI_PHASE_COUNT);
	if (step.Entered == IAI_DRAW) BeginDraw();
	if (m_Phase.GetPhase() == IAI_DRAW) SlashTargets();
}

void KatanaArm::UpdateLanding()
{
	const XMFLOAT3 p = GetPlayer().GetPosition();
	const XMFLOAT3 view = GetCamera().GetDirection();
	const STAGE& stage = GetStage().GetStage();
	const float range = BREACH_RANGE * TuningCalcValue(GetTuning(), TUNING_RUSH_DISTANCE);
	m_HasLanding = BreachCalcFindLanding(stage, { p.x, p.y + GetPlayer().GetHeight() * BREACH_EYE_RATE, p.z },
		{ view.x, view.y, view.z }, range, m_Landing);
	// 場外に落ちている間は、狙いが外れていても、いちばん近い足場の上へ戻れる。
	if (!m_HasLanding && !GetPlayer().IsOnGround() && p.y < stage.TopY) {
		m_HasLanding = RiseCalcNearestLanding(stage, { p.x, p.y, p.z }, RETURN_INSET, range, m_Landing);
	}
}

void KatanaArm::StartCut()
{
	const XMFLOAT3 p = GetPlayer().GetPosition();
	const VEC3 start{ p.x, p.y, p.z };
	m_Direction = ArmCalcBlendDirection(GetForward(), Vec3Sub(m_Landing, start), 1.0f);
	const VEC3 flat = ArmCalcPitchDirection(m_Direction, 0.0f);
	// 手の側の上から、反対の下へ斬り下ろす線にする。
	const float side = GetHand() == 0 ? -1.0f : 1.0f;
	const VEC3 right{ flat.Z, 0.0f, -flat.X };
	const VEC3 axis = Vec3Add({ 0.0f, 1.0f, 0.0f }, Vec3Scale(right, side * CUT_SIDE));
	m_CutAxis = Vec3Scale(axis, 1.0f / Vec3Length(axis));
	m_CutCenter = Vec3Add({ p.x, p.y + GetPlayer().GetHeight() * CUT_HEIGHT_RATE, p.z }, Vec3Scale(flat, CUT_FORWARD));
	SetBreach(BREACH_CUT);
	// 空中で斬り始めたら、斬っている間は落ちない（場外から戻れるように）。
	if (!GetPlayer().IsOnGround()) {
		GetPlayer().SetPulled(true);
		m_OwnPull = true;
	}
}

void KatanaArm::StartDash()
{
	const XMFLOAT3 p = GetPlayer().GetPosition();
	m_RiftStart = { p.x, p.y, p.z };
	m_RiftEnd = m_Landing;
	m_RiftSeconds = RIFT_SECONDS;
	m_Direction = ArmCalcBlendDirection(GetForward(), Vec3Sub(m_Landing, m_RiftStart), 1.0f);
	SetBreach(BREACH_DASH);
	m_Hits.Clear();
	GetPlayer().SetPulled(true);
	m_OwnPull = true;
	ConsumeUse();
	m_IaiConsumed = true; // 使用回数は飛んだときに減らし、着いたあとの一閃では減らさない。
	Rumble(HIT_RUMBLE_SMALL, HIT_RUMBLE_LARGE, HIT_RUMBLE_SECONDS);
}

// 自分が止めていた体の計算を、ふつうに戻す。
void KatanaArm::ReleasePull()
{
	if (!m_OwnPull) return;
	m_OwnPull = false;
	GetPlayer().SetPulled(false);
}

void KatanaArm::StepBreach(const ARM_INPUT& input, float dt)
{
	m_BreachTime += dt;
	if (m_Breach == BREACH_AIM) {
		UpdateLanding();
		if (input.Pressed) return;
		// 離したときに着地点があれば空間を斬る（もう一方の手や触手で動いている間は斬らない）。
		if (m_HasLanding && !GetPlayer().IsPulled()) StartCut();
		else SetBreach(BREACH_NONE);
		m_HasLanding = false;
		return;
	}
	if (m_Breach == BREACH_CUT) {
		if (m_BreachTime < BREACH_CUT_SECONDS) return;
		// もう一方の手や触手が体を動かしている間は飛ばない（自分が空中で止めているだけなら飛ぶ）。
		if (GetPlayer().IsPulled() && !m_OwnPull) SetBreach(BREACH_NONE);
		else StartDash();
		return;
	}
	if (m_Breach == BREACH_DASH) {
		const XMFLOAT3 p = GetPlayer().GetPosition();
		VEC3 feet{ p.x, p.y, p.z };
		const bool arrived = GrappleCalcPull(feet, m_Landing, BREACH_SPEED, BREACH_ARRIVE_DISTANCE, dt);
		const VEC3 next = arrived ? m_Landing : feet;
		GetPlayer().SetPosition({ next.X, next.Y, next.Z });
		DashTargets({ p.x, p.y, p.z }, next);
		if (arrived) {
			ReleasePull();
			SetBreach(BREACH_RECOVER);
			// 着いた瞬間に、飛んだ向きを前として居合の一閃を出す。
			m_IaiDirection = ArmCalcPitchDirection(m_Direction, 0.0f);
			IaiTargets();
		}
		return;
	}
	if (m_Breach == BREACH_RECOVER) {
		if (m_BreachTime < IAI_ARRIVE_SECONDS) IaiTargets();
		if (m_BreachTime >= BREACH_RECOVER_SECONDS) SetBreach(BREACH_NONE);
	}
}

void KatanaArm::Update(const ARM_INPUT& input)
{
	const float dt = fmaxf(0.0f, GameTimeGetDeltaTime());
	if (GetPlayer().DidRespawn()) Reset();
	if (!IsActive()) {
		if (input.Tap) StartIai();
		else if (input.HoldStarted) SetBreach(BREACH_AIM);
	}
	StepIai(dt);
	StepBreach(input, dt);
	StepDelayedHits(dt);
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
		// 残心の終わりは納刀に入るので鞘を見せる。
		sheathed = phase == IAI_RECOVER && m_Phase.GetTime() >= IAI_SECONDS[IAI_RECOVER] * IAI_SHEATH_RATE;
	}
	else if (m_Breach == BREACH_AIM || m_Breach == BREACH_CUT) {
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

// 空間を斬った裂け目を描く（斬っている間は刀の動きに合わせて伸びる）。
void KatanaArm::DrawCutRift() const
{
	const float open = m_Breach == BREACH_CUT ? fminf(1.0f, m_BreachTime / BREACH_CUT_SECONDS) : 1.0f;
	const float length = CUT_HALF_LENGTH * open;
	// 右手は上から斬り下ろし、左手は下から斬り上げるので、伸び始める端を変える。
	const VEC3 axis = GetHand() == 0 ? Vec3Scale(m_CutAxis, -1.0f) : m_CutAxis;
	const VEC3 top = Vec3Add(m_CutCenter, Vec3Scale(axis, CUT_HALF_LENGTH));
	const VEC3 bottom = Vec3Add(top, Vec3Scale(axis, -2.0f * length));
	ShapeDrawCylinder({ top.X, top.Y, top.Z }, { bottom.X, bottom.Y, bottom.Z }, CUT_RADIUS * open, RIFT_COLOR);
	const VEC3 middle = Vec3Add(top, Vec3Scale(axis, -length));
	const VEC3 coreTop = Vec3Add(middle, Vec3Scale(axis, length * CUT_CORE_RATE));
	const VEC3 coreBottom = Vec3Add(middle, Vec3Scale(axis, -length * CUT_CORE_RATE));
	ShapeDrawCylinder({ coreTop.X, coreTop.Y, coreTop.Z }, { coreBottom.X, coreBottom.Y, coreBottom.Z }, CUT_CORE_RADIUS * open, RIFT_CORE_COLOR);
}

// 待っている斬撃の細い線と、入った瞬間の十字の斬撃を相手の体に描く。
void KatanaArm::DrawSlashes() const
{
	for (int i = 0; i < m_Delayed.GetCount(); ++i) {
		const DELAYED_HIT& hit = m_Delayed.Get(i);
		const CAPSULE body = GetFighters().GetTarget(hit.Target, GetSlot()).Body;
		const VEC3 center = Vec3Scale(Vec3Add(body.Start, body.End), 0.5f);
		const VEC3 flat = ArmCalcPitchDirection(hit.Direction, 0.0f);
		const VEC3 line = Vec3Scale(VEC3{ flat.Z, 0.5f, -flat.X }, SLASH_MARK_LENGTH);
		const VEC3 a = Vec3Add(center, line);
		const VEC3 b = Vec3Sub(center, line);
		ShapeDrawCylinder({ a.X, a.Y, a.Z }, { b.X, b.Y, b.Z }, SLASH_MARK_RADIUS, SLASH_COLOR);
	}
	for (int i = 0; i < FIGHTER_COUNT; ++i) {
		if (m_SlashSeconds[i] <= 0.0f) continue;
		const CAPSULE body = GetFighters().GetTarget(i, GetSlot()).Body;
		const VEC3 center = Vec3Scale(Vec3Add(body.Start, body.End), 0.5f);
		const VEC3 right{ m_SlashDirections[i].Z, 0.0f, -m_SlashDirections[i].X };
		const float width = SLASH_RADIUS * (m_SlashSeconds[i] / SLASH_SHOW_SECONDS);
		const VEC3 lines[] = {
			Vec3Add(Vec3Scale(right, SLASH_LENGTH * 0.77f), { 0.0f, SLASH_LENGTH * 0.64f, 0.0f }),
			Vec3Add(Vec3Scale(right, -SLASH_LENGTH * 0.77f), { 0.0f, SLASH_LENGTH * 0.64f, 0.0f }),
			Vec3Scale(right, SLASH_LENGTH),
		};
		for (const VEC3& line : lines) {
			const VEC3 a = Vec3Add(center, line);
			const VEC3 b = Vec3Sub(center, line);
			ShapeDrawCylinder({ a.X, a.Y, a.Z }, { b.X, b.Y, b.Z }, width, SLASH_COLOR);
		}
	}
}

void KatanaArm::Draw()
{
	const VEC3 restArm = GetRestDirection();
	const VEC3 restSword = ArmCalcPitchDirection(GetForward(), REST_SWORD_PITCH);
	VEC3 armPose = restArm;
	VEC3 swordPose = restSword;
	bool sheathed = false;
	GetPose(armPose, swordPose, sheathed);
	VEC3 arm = ArmCalcBlendDirection(restArm, armPose, m_Straighten);
	VEC3 sword = ArmCalcBlendDirection(restSword, swordPose, m_Straighten);
	BlendClipPose(arm, sword);
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
	if (m_Breach == BREACH_CUT || m_RiftSeconds > 0.0f) DrawCutRift();
	if (m_RiftSeconds > 0.0f) {
		DrawRift(m_RiftEnd);
		const VEC3 line = Vec3Sub(m_RiftEnd, m_RiftStart);
		const int dots = (int)(Vec3Length(line) / RIFT_TRAIL_GAP);
		for (int i = 1; i < dots; ++i) {
			const VEC3 point = Vec3Add(m_RiftStart, Vec3Scale(line, (float)i / dots));
			ShapeDrawSphere({ point.X, point.Y + RIFT_HEIGHT * 0.5f, point.Z }, RIFT_TRAIL_RADIUS, RIFT_COLOR);
		}
	}
	DrawSlashes();
}

bool KatanaArm::IsActive() const
{
	return !m_Phase.IsIdle() || m_Breach != BREACH_NONE;
}

bool KatanaArm::IsBusy() const
{
	if (m_Delayed.GetCount() > 0) return true;
	for (float seconds : m_SlashSeconds) {
		if (seconds > 0.0f) return true;
	}
	return false;
}

bool KatanaArm::CanTakeTap() const
{
	return !IsActive() && (GetRemainingUses() < 0 || m_Delayed.GetCount() == 0);
}

bool KatanaArm::IsHolding() const
{
	return false;
}

float KatanaArm::GetMoveScale() const
{
	if (m_Breach == BREACH_CUT) return 0.0f;
	return m_Phase.IsIdle() ? 1.0f : TAP_MOVE_SCALE;
}

GUARD KatanaArm::GetGuard() const
{
	if (m_Breach != BREACH_DASH) return {};
	GUARD guard;
	guard.Kind = GUARD_ALL;
	return guard;
}

ARM_BODY_CLIP KatanaArm::GetBodyClip() const
{
	ARM_BODY_CLIP clip;
	if (!m_Phase.IsIdle()) {
		clip.Active = true;
		clip.Kind = ATTACK_CLIP::KatanaIai;
		clip.FullBody = true;
		clip.Time = ArmCalcClipTime(m_Phase.GetPhase(), m_Phase.GetTime(), IAI_SECONDS, IAI_CLIP_SECONDS);
		return clip;
	}
	if (m_Breach == BREACH_NONE) return clip;
	clip.Active = true;
	clip.Kind = ATTACK_CLIP::KatanaAssault;
	// 狙っている間は歩けるので上半身だけ、斬ってからは全身で見せる。
	clip.FullBody = m_Breach != BREACH_AIM;
	clip.Time = ArmCalcClipTime(m_Breach - BREACH_AIM, m_BreachTime, BREACH_PHASE_SECONDS, BREACH_CLIP_SECONDS);
	return clip;
}
