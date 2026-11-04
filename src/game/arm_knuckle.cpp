#include "game/arm_knuckle.h"
#include "game/fightermanager.h"
#include "game/fighter.h"
#include "core/gametime.h"
#include "physics/areacalc.h"
#include "physics/armcalc.h"
#include "physics/knucklecalc.h"
#include "render/shape.h"

// 調整用の値
static constexpr float WINDUP_SECONDS = 0.12f;
static constexpr float PUNCH_SECONDS = 0.1f;
static constexpr float RECOVER_SECONDS = 0.35f;
static constexpr float PILE_RECOVER_SECONDS = 0.5f;
static constexpr float PUNCH_RANGE = 1.6f; // 肩から当たりのカプセルの終わりまで。
static constexpr float PUNCH_RADIUS = 0.5f;
static constexpr float LAUNCH_UP = 0.25f;
static constexpr KNUCKLE_PARAMS PARAMS{ 1.0f, { 10.0f, 2.2f, false }, { 14.0f, 2.8f, true } };
static constexpr float STRAIGHTEN_SECONDS = 0.1f;
static constexpr float ARM_LENGTH = 0.45f;
static constexpr float PUNCH_ARM_LENGTH = 0.75f; // 殴る間だけ腕が伸びて見える。
static constexpr float FIST_SIZE = 0.2f;
static constexpr float PILE_LENGTH = 0.4f;
static constexpr float PILE_RADIUS = 0.04f;
static constexpr float HIT_RUMBLE_SMALL = 0.6f;
static constexpr float HIT_RUMBLE_LARGE = 0.6f;
static constexpr float HIT_RUMBLE_SECONDS = 0.15f;
static constexpr float PILE_RUMBLE_SMALL = 1.0f;
static constexpr float PILE_RUMBLE_LARGE = 1.0f;
static constexpr float PILE_RUMBLE_SECONDS = 0.2f;

enum KNUCKLE_PHASE { KNUCKLE_WINDUP, KNUCKLE_PUNCH, KNUCKLE_RECOVER, KNUCKLE_PHASE_COUNT };

KnuckleArm::KnuckleArm(int hand, const ARM_LINKS& links) : Arm(hand, links)
{
	Reset();
}

void KnuckleArm::Reset()
{
	m_Phase.Reset();
	m_Seconds[KNUCKLE_WINDUP] = WINDUP_SECONDS;
	m_Seconds[KNUCKLE_PUNCH] = PUNCH_SECONDS;
	m_Seconds[KNUCKLE_RECOVER] = RECOVER_SECONDS;
	m_Pile = false;
	m_Straighten = 0.0f;
}

void KnuckleArm::Update(bool trigger)
{
	const float dt = fmaxf(0.0f, GameTimeGetDeltaTime());
	if (GetPlayer().DidRespawn()) Reset();
	if (m_Phase.IsIdle() && trigger) {
		m_Direction = GetAim(false);
		m_Seconds[KNUCKLE_RECOVER] = RECOVER_SECONDS;
		m_Pile = false;
		m_Phase.Start(KNUCKLE_WINDUP);
	}
	const ARM_PHASE_STEP step = m_Phase.Step(dt, m_Seconds, KNUCKLE_PHASE_COUNT);
	if (step.Entered == KNUCKLE_PUNCH) {
		const VEC3 shoulder = GetShoulder();
		const CAPSULE punch{ shoulder, Vec3Add(shoulder, Vec3Scale(m_Direction, PUNCH_RANGE)), PUNCH_RADIUS };
		const VEC3 direction = AreaCalcLaunchDirection(m_Direction, LAUNCH_UP);
		bool hit = false;
		for (int i = 0; i < FIGHTER_COUNT; ++i) {
			const TENTACLE_TARGET target = GetFighters().GetTarget(i, GetSlot());
			if (!target.Available || !CollisionCapsuleCapsule(punch, target.Body).Hit) continue;
			const VEC3 center = Vec3Scale(Vec3Add(target.Body.Start, target.Body.End), 0.5f);
			const float distance = Vec3Length({ center.X - shoulder.X, 0.0f, center.Z - shoulder.Z });
			const KNUCKLE_HIT power = KnuckleCalcPick(distance, PARAMS);
			GetFighters().Launch(i, GetSlot(), direction, power.Damage, power.Power);
			m_Pile = m_Pile || power.Pile;
			hit = true;
		}
		if (m_Pile) {
			m_Seconds[KNUCKLE_RECOVER] = PILE_RECOVER_SECONDS;
			Rumble(PILE_RUMBLE_SMALL, PILE_RUMBLE_LARGE, PILE_RUMBLE_SECONDS);
		}
		else if (hit) {
			Rumble(HIT_RUMBLE_SMALL, HIT_RUMBLE_LARGE, HIT_RUMBLE_SECONDS);
		}
	}
	const float speed = dt / STRAIGHTEN_SECONDS;
	m_Straighten = m_Phase.IsIdle() ? fmaxf(0.0f, m_Straighten - speed) : fminf(1.0f, m_Straighten + speed);
}

void KnuckleArm::Draw()
{
	const bool punching = m_Phase.GetPhase() == KNUCKLE_PUNCH;
	const VEC3 direction = ArmCalcBlendDirection(GetRestDirection(), m_Direction, m_Straighten);
	const XMFLOAT4 color = GetColor(!m_Phase.IsIdle());
	const VEC3 hand = DrawStubArm(direction, punching ? PUNCH_ARM_LENGTH : ARM_LENGTH, color);
	ShapeDrawBox({ hand.X, hand.Y, hand.Z }, { FIST_SIZE, FIST_SIZE, FIST_SIZE }, color);
	if (punching && m_Pile) {
		const VEC3 tip = Vec3Add(hand, Vec3Scale(direction, PILE_LENGTH));
		ShapeDrawCylinder({ hand.X, hand.Y, hand.Z }, { tip.X, tip.Y, tip.Z }, PILE_RADIUS, color);
	}
}

bool KnuckleArm::IsActive() const
{
	return !m_Phase.IsIdle();
}

bool KnuckleArm::IsHolding() const
{
	return false;
}
