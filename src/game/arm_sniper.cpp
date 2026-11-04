#include "game/arm_sniper.h"
#include "game/fightermanager.h"
#include "game/fighter.h"
#include "core/gametime.h"
#include "core/mathutil.h"
#include "physics/areacalc.h"
#include "physics/armcalc.h"
#include "render/shape.h"

// 調整用の値
static constexpr float AIM_SECONDS = 1.0f;
static constexpr float RECOVER_SECONDS = 0.8f;
static constexpr float CANCEL_SECONDS = 0.3f;
static constexpr float MOVE_SCALE = 0.3f;
static constexpr float SHOT_RANGE = 40.0f;
static constexpr float SHOT_RADIUS = 0.2f;
static constexpr float SHOT_DAMAGE = 14.0f;
static constexpr float SHOT_POWER = 2.0f;
static constexpr float FAR_DISTANCE = 20.0f; // この距離で強さの上乗せが最大になる。
static constexpr float FAR_BONUS = 0.5f; // 最大で強さに掛ける上乗せ（1 ＋ 0.5）。
static constexpr float SHOT_UP = 0.15f;
static constexpr float TRACE_SECONDS = 0.1f;
static constexpr float STRAIGHTEN_SECONDS = 0.1f;
static constexpr float ARM_LENGTH = 0.55f;
static constexpr float BARREL_LENGTH = 0.5f;
static constexpr float BARREL_RADIUS = 0.035f;
static constexpr float SIGHT_RADIUS = 0.025f;
static constexpr XMFLOAT4 SIGHT_COLOR{ 1.0f, 0.1f, 0.1f, 1.0f };
static constexpr float TRACE_RADIUS = 0.03f;
static constexpr float FIRE_RUMBLE_SMALL = 0.8f;
static constexpr float FIRE_RUMBLE_LARGE = 0.8f;
static constexpr float FIRE_RUMBLE_SECONDS = 0.15f;

enum SNIPER_PHASE { SNIPER_AIM, SNIPER_RECOVER, SNIPER_PHASE_COUNT };

SniperArm::SniperArm(int hand, const ARM_LINKS& links) : Arm(hand, links)
{
	Reset();
}

void SniperArm::Reset()
{
	m_Phase.Reset();
	m_Seconds[SNIPER_AIM] = AIM_SECONDS;
	m_Seconds[SNIPER_RECOVER] = RECOVER_SECONDS;
	m_TraceLength = 0.0f;
	m_Straighten = 0.0f;
}

void SniperArm::Update(bool trigger)
{
	const float dt = fmaxf(0.0f, GameTimeGetDeltaTime());
	if (GetPlayer().DidRespawn()) Reset();
	if (m_Phase.IsIdle() && trigger) {
		m_Seconds[SNIPER_RECOVER] = RECOVER_SECONDS;
		m_TraceLength = 0.0f;
		m_Phase.Start(SNIPER_AIM);
	}
	else if (m_Phase.GetPhase() == SNIPER_AIM && trigger) {
		m_Seconds[SNIPER_RECOVER] = CANCEL_SECONDS;
		m_Phase.Start(SNIPER_RECOVER);
	}
	if (m_Phase.GetPhase() == SNIPER_AIM) m_Direction = GetAim(true);
	const ARM_PHASE_STEP step = m_Phase.Step(dt, m_Seconds, SNIPER_PHASE_COUNT);
	if (step.Entered == SNIPER_RECOVER) {
		TENTACLE_TARGET targets[FIGHTER_COUNT];
		for (int i = 0; i < FIGHTER_COUNT; ++i) targets[i] = GetFighters().GetTarget(i, GetSlot());
		float distance = SHOT_RANGE;
		const int hit = AreaCalcFirstOnRay(GetShoulder(), m_Direction, SHOT_RANGE, SHOT_RADIUS, targets, FIGHTER_COUNT, &distance);
		if (hit >= 0) {
			const float power = SHOT_POWER * (1.0f + FAR_BONUS * Saturate(distance / FAR_DISTANCE));
			GetFighters().Launch(hit, GetSlot(), AreaCalcLaunchDirection(m_Direction, SHOT_UP), SHOT_DAMAGE, power);
		}
		m_TraceLength = distance;
		Rumble(FIRE_RUMBLE_SMALL, FIRE_RUMBLE_LARGE, FIRE_RUMBLE_SECONDS);
	}
	const float speed = dt / STRAIGHTEN_SECONDS;
	m_Straighten = m_Phase.IsIdle() ? fmaxf(0.0f, m_Straighten - speed) : fminf(1.0f, m_Straighten + speed);
}

void SniperArm::Draw()
{
	const VEC3 direction = ArmCalcBlendDirection(GetRestDirection(), m_Direction, m_Straighten);
	const XMFLOAT4 color = GetColor(!m_Phase.IsIdle());
	const VEC3 hand = DrawStubArm(direction, ARM_LENGTH, color);
	const VEC3 muzzle = Vec3Add(hand, Vec3Scale(direction, BARREL_LENGTH));
	ShapeDrawCylinder({ hand.X, hand.Y, hand.Z }, { muzzle.X, muzzle.Y, muzzle.Z }, BARREL_RADIUS, color);
	const VEC3 shoulder = GetShoulder();
	if (m_Phase.GetPhase() == SNIPER_AIM) {
		const VEC3 end = Vec3Add(shoulder, Vec3Scale(m_Direction, SHOT_RANGE));
		ShapeDrawCylinder({ muzzle.X, muzzle.Y, muzzle.Z }, { end.X, end.Y, end.Z }, SIGHT_RADIUS, SIGHT_COLOR);
	}
	else if (m_Phase.GetPhase() == SNIPER_RECOVER && m_TraceLength > 0.0f && m_Phase.GetTime() < TRACE_SECONDS) {
		const VEC3 end = Vec3Add(shoulder, Vec3Scale(m_Direction, m_TraceLength));
		ShapeDrawCylinder({ muzzle.X, muzzle.Y, muzzle.Z }, { end.X, end.Y, end.Z }, TRACE_RADIUS, color);
	}
}

bool SniperArm::IsActive() const
{
	return !m_Phase.IsIdle();
}

bool SniperArm::IsHolding() const
{
	return false;
}

float SniperArm::GetMoveScale() const
{
	return m_Phase.GetPhase() == SNIPER_AIM ? MOVE_SCALE : 1.0f;
}
