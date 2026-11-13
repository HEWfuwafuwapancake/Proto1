#include "game/arm_hammer.h"
#include "game/fightermanager.h"
#include "game/fighter.h"
#include "game/camera.h"
#include "core/gametime.h"
#include "physics/areacalc.h"
#include "physics/armcalc.h"
#include "render/shape.h"

// 調整用の値
static constexpr int TAP_HIT_COST = 50;
static constexpr int TAP_MISS_COST = 25;
static constexpr int WAVE_HIT_COST = 50;
static constexpr int WAVE_MISS_COST = 30;
static constexpr int AIR_TAP_COST = 50;
static constexpr float TAP_SECONDS[] = { 0.9f, 0.1f, 0.4f }; // 単押し：ため・叩き・休み
static constexpr float WAVE_SECONDS[] = { 1.2f, 0.1f, 0.5f }; // 長押し：ため・叩き・休み
static constexpr float TAP_CLIP_SECONDS[] = { 16.0f / 24.0f, 4.0f / 24.0f, 10.0f / 24.0f }; // 単押しのクリップの区切り（hammer_tap）
static constexpr float WAVE_CLIP_SECONDS[] = { 22.0f / 24.0f, 4.0f / 24.0f, 14.0f / 24.0f }; // 長押しのクリップの区切り（hammer_slam）
static constexpr float AIR_TAP_SECONDS[] = { 0.3f, 0.1f, 0.4f }; // 空中の単押し：短いため・叩き・休み
static constexpr float HOP_HEIGHT = 4.0f; // 空中の単押しで上へ跳ねる高さ（場外から戻る）。
static constexpr float HOP_SIDE = 2.0f; // 跳ねる間に、移動の入力の向きへ進む距離。
static constexpr float HOP_SECONDS = 0.3f; // 跳ね上がるのにかける秒数。
static constexpr float MOVE_SCALE = 0.5f;
static constexpr float IMPACT_FORWARD = 1.6f; // 足元から叩く所までの前への距離。
static constexpr float IMPACT_HEIGHT = 0.6f; // 叩く所の足元からの高さ。
static constexpr float IMPACT_RADIUS = 1.0f;
static constexpr float IMPACT_DAMAGE = 18.0f;
static constexpr float IMPACT_POWER = 2.8f;
static constexpr float IMPACT_UP = 1.25f; // 外への水平の単位方向に足す上向きの量。
static constexpr float WAVE_HEIGHT = 0.3f; // 衝撃波の中心の足元からの高さ。
static constexpr float WAVE_RADIUS = 4.5f;
static constexpr float WAVE_DAMAGE = 18.0f;
static constexpr float WAVE_POWER = 2.4f;
static constexpr float WAVE_UP = 0.8f;
static constexpr float RANGE_SHOW_SECONDS = 0.2f; // 当たる範囲を見せる秒数。
static constexpr float RAISE_PITCH = XMConvertToRadians(60.0f);
static constexpr float SLAM_PITCH = XMConvertToRadians(-30.0f);
static constexpr float CARRY_PITCH = XMConvertToRadians(25.0f); // 待機のときのハンマーの上下の向き。
static constexpr float STRAIGHTEN_SECONDS = 0.1f;
static constexpr float MODEL_SCALE = 0.08f * Arm::MODEL_SIZE;
static constexpr float ARM_LENGTH = 19.250364f; // arm_shield_base の長さ（cm）。
static constexpr float HEAD_LENGTH = 32.5f; // arm_hammer_head の長さ（cm）。
static constexpr XMFLOAT4 RANGE_COLOR{ 1.0f, 0.9f, 0.3f, 1.0f };
static constexpr float HIT_RUMBLE_SMALL = 0.8f;
static constexpr float HIT_RUMBLE_LARGE = 1.0f;
static constexpr float HIT_RUMBLE_SECONDS = 0.2f;

enum HAMMER_PHASE { HAMMER_CHARGE, HAMMER_SLAM, HAMMER_RECOVER, HAMMER_PHASE_COUNT };

HammerArm::HammerArm(int hand, const ARM_LINKS& links) : Arm(hand, links)
{
	m_ArmModel = LoadModel("assets/models/arm_shield_base.obj");
	m_HeadModel = LoadModel("assets/models/arm_hammer_head.obj");
	Reset();
}

// 今の動作の、段階ごとの秒数の並びを返す。
const float* HammerArm::GetSeconds() const
{
	return m_Wave ? WAVE_SECONDS : (m_AirTap ? AIR_TAP_SECONDS : TAP_SECONDS);
}

// 跳ねるために止めていた体の計算を、ふつうに戻す。
void HammerArm::StopHop()
{
	if (!m_Hopping) return;
	m_Hopping = false;
	m_Rise.Reset();
	GetPlayer().SetPulled(false);
}

// 空中の単押し：振った瞬間に上へ跳ねる。水平の速度しか体に伝わらないので、自分で体を動かす。
void HammerArm::StartHop()
{
	if (GetPlayer().IsOnGround() || GetPlayer().IsPulled()) return;
	const XMFLOAT3 feet = GetPlayer().GetPosition();
	const ACTION_AXIS move = GetPlayer().GetMoveInput();
	const float yaw = GetCamera().GetYaw();
	const VEC3 offset{ (cosf(yaw) * move.X + sinf(yaw) * move.Y) * HOP_SIDE, HOP_HEIGHT, (-sinf(yaw) * move.X + cosf(yaw) * move.Y) * HOP_SIDE };
	m_Rise.Start({ feet.x, feet.y, feet.z }, offset, HOP_SECONDS, true);
	m_Hopping = true;
	GetPlayer().SetPulled(true);
}

void HammerArm::Reset()
{
	if (!m_Phase.IsIdle() && !m_UseConsumed) ConsumeDurability(m_Wave ? WAVE_MISS_COST : TAP_MISS_COST);
	m_UseConsumed = true;
	StopHop();
	m_Phase.Reset();
	m_Straighten = 0.0f;
	m_RangeSeconds = 0.0f;
}

void HammerArm::Impact()
{
	const XMFLOAT3 feet = GetPlayer().GetPosition();
	m_RangeRadius = m_Wave ? WAVE_RADIUS : IMPACT_RADIUS;
	m_RangeCenter = m_Wave ? VEC3{ feet.x, feet.y + WAVE_HEIGHT, feet.z }
		: Vec3Add({ feet.x, feet.y + IMPACT_HEIGHT, feet.z }, Vec3Scale(m_Direction, IMPACT_FORWARD));
	m_RangeSeconds = RANGE_SHOW_SECONDS;
	bool hit = false;
	bool affected = false;
	for (int i = 0; i < FIGHTER_COUNT; ++i) {
		const TENTACLE_TARGET target = GetFighters().GetTarget(i, GetSlot());
		if (!target.Available || !CollisionSphereCapsule({ m_RangeCenter, m_RangeRadius }, target.Body).Hit) continue;
		// 衝撃波はまわりへ押し出し、単押しは叩いた相手を叩いた向きへ飛ばす。
		const VEC3 launch = m_Wave ? AreaCalcRadialDirection(m_RangeCenter, target.Body, WAVE_UP, m_Direction)
			: AreaCalcLaunchDirection(m_Direction, IMPACT_UP);
		if (GetFighters().Launch(i, GetSlot(), launch,
			m_Wave ? WAVE_DAMAGE : IMPACT_DAMAGE, m_Wave ? WAVE_POWER : IMPACT_POWER)) affected = true;
		const float stunSeconds = TuningCalcValue(GetTuning(), TUNING_HAMMER_STUN);
		if (m_Wave && stunSeconds > 0.0f && GetFighters().ApplyStatus(i, GetSlot(), STATUS_STUN, stunSeconds, 0.0f)) affected = true;
		hit = true;
	}
	if (affected && !m_UseConsumed) {
		ConsumeDurability(m_Wave ? WAVE_HIT_COST : TAP_HIT_COST);
		m_UseConsumed = true;
	}
	if (hit) Rumble(HIT_RUMBLE_SMALL, HIT_RUMBLE_LARGE, HIT_RUMBLE_SECONDS);
}

void HammerArm::Update(const ARM_INPUT& input)
{
	const float dt = fmaxf(0.0f, GameTimeGetDeltaTime());
	if (GetPlayer().DidRespawn()) Reset();
	if (m_Phase.IsIdle() && (input.Tap || input.HoldStarted)) {
		m_Wave = input.HoldStarted;
		// 空中の単押しは、ためを短くして、振った瞬間に上へ跳ねる。
		m_AirTap = !m_Wave && !GetPlayer().IsOnGround();
		m_UseConsumed = false;
		m_Direction = GetAim(false);
		m_Phase.Start(HAMMER_CHARGE);
	}
	// ためている間は狙いを追い続け、叩く瞬間の向き（ロックオン中は相手の方）に叩く。
	if (m_Phase.GetPhase() == HAMMER_CHARGE) m_Direction = GetAim(false);
	const ARM_PHASE_STEP step = m_Phase.Step(dt, GetSeconds(), HAMMER_PHASE_COUNT);
	if (step.Entered == HAMMER_SLAM) {
		m_Direction = GetAim(false);
		if (m_AirTap) {
			ConsumeDurability(AIR_TAP_COST);
			m_UseConsumed = true;
			StartHop();
		}
		Impact();
	}
	if (m_Hopping) {
		if (!m_Phase.IsIdle() && m_Rise.IsActive()) {
			const VEC3 next = m_Rise.Step(dt);
			GetPlayer().SetPosition({ next.X, next.Y, next.Z });
		}
		else StopHop();
	}
	m_RangeSeconds = fmaxf(0.0f, m_RangeSeconds - dt);
	if (step.Finished && !m_UseConsumed) {
		ConsumeDurability(m_Wave ? WAVE_MISS_COST : TAP_MISS_COST);
		m_UseConsumed = true;
	}
	const float speed = dt / STRAIGHTEN_SECONDS;
	m_Straighten = m_Phase.IsIdle() ? fmaxf(0.0f, m_Straighten - speed) : fminf(1.0f, m_Straighten + speed);
}

void HammerArm::Draw()
{
	const float pitch = m_Phase.GetPhase() == HAMMER_CHARGE ? RAISE_PITCH : SLAM_PITCH;
	const VEC3 aim = ArmCalcPitchDirection(m_Direction, pitch);
	VEC3 armDirection = ArmCalcBlendDirection(GetRestDirection(), aim, m_Straighten);
	VEC3 headDirection = ArmCalcBlendDirection(ArmCalcPitchDirection(GetForward(), CARRY_PITCH), aim, m_Straighten);
	BlendClipPose(armDirection, headDirection);
	const XMFLOAT4 color = GetColor(!m_Phase.IsIdle());
	const float scale = MODEL_SCALE * GetDrawScale();
	const VEC3 hand = DrawAlong(m_ArmModel, GetDrawShoulder(), armDirection, ARM_LENGTH, scale, color);
	DrawAlong(m_HeadModel, hand, headDirection, HEAD_LENGTH, scale, color);
	if (m_RangeSeconds > 0.0f) DrawRangeRing(m_RangeCenter, m_RangeRadius, RANGE_COLOR);
}

bool HammerArm::IsActive() const
{
	return !m_Phase.IsIdle();
}

ARM_BODY_CLIP HammerArm::GetBodyClip() const
{
	ARM_BODY_CLIP clip;
	if (m_Phase.IsIdle()) return clip;
	clip.Active = true;
	clip.Kind = m_Wave ? ATTACK_CLIP::HammerSlam : ATTACK_CLIP::HammerTap;
	clip.FullBody = m_Wave;
	clip.Time = ArmCalcClipTime(m_Phase.GetPhase(), m_Phase.GetTime(), GetSeconds(), m_Wave ? WAVE_CLIP_SECONDS : TAP_CLIP_SECONDS);
	return clip;
}

bool HammerArm::IsHolding() const
{
	return false;
}

float HammerArm::GetMoveScale() const
{
	return m_Phase.IsIdle() ? 1.0f : MOVE_SCALE;
}
