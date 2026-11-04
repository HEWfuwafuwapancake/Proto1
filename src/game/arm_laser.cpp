#include "game/arm_laser.h"
#include "game/fightermanager.h"
#include "game/fighter.h"
#include "core/gametime.h"
#include "physics/areacalc.h"
#include "physics/armcalc.h"
#include "render/shape.h"

// 調整用の値
static constexpr float PHASE_SECONDS[] = { 0.25f, 0.6f, 0.5f }; // 溜め・照射・休み
static constexpr float TICK_SECONDS = 0.1f;
static constexpr int BEAM_TICKS = 6; // 照射の刻みの数（最後の刻みで吹っ飛ばす）。
static constexpr float MOVE_SCALE = 0.6f;
static constexpr float BEAM_LENGTH = 18.0f;
static constexpr float BEAM_RADIUS = 0.3f;
static constexpr float TICK_DAMAGE = 2.0f;
static constexpr float FINAL_POWER = 1.2f;
static constexpr float FINAL_UP = 0.1f;
static constexpr float STRAIGHTEN_SECONDS = 0.1f;
static constexpr float ARM_LENGTH = 0.55f;
static constexpr float CHARGE_RADIUS = 0.12f; // 溜め終わりの球の半径。
static constexpr float BEAM_DRAW_RADIUS = 0.12f;
static constexpr float TICK_RUMBLE_SMALL = 0.3f;
static constexpr float TICK_RUMBLE_LARGE = 0.0f;
static constexpr float TICK_RUMBLE_SECONDS = 0.1f;

enum LASER_PHASE { LASER_CHARGE, LASER_BEAM, LASER_RECOVER, LASER_PHASE_COUNT };

LaserArm::LaserArm(int hand, const ARM_LINKS& links) : Arm(hand, links)
{
	Reset();
}

void LaserArm::Reset()
{
	m_Phase.Reset();
	m_Ticks = 0;
	m_Straighten = 0.0f;
}

void LaserArm::Update(bool trigger)
{
	const float dt = fmaxf(0.0f, GameTimeGetDeltaTime());
	if (GetPlayer().DidRespawn()) Reset();
	if (m_Phase.IsIdle() && trigger) {
		m_Direction = GetAim(true);
		m_Phase.Start(LASER_CHARGE);
	}
	else if (m_Phase.GetPhase() == LASER_BEAM && trigger) {
		m_Phase.Start(LASER_RECOVER);
	}
	const ARM_PHASE_STEP step = m_Phase.Step(dt, PHASE_SECONDS, LASER_PHASE_COUNT, TICK_SECONDS);
	if (step.Entered == LASER_BEAM) {
		m_Direction = GetAim(true);
		m_Ticks = 0;
	}
	// 照射の刻み（照射に入ったフレームの刻みは数えない）。
	const int ticks = m_Phase.GetPhase() == LASER_BEAM && step.Entered != LASER_BEAM ? step.Ticks
		: (step.Entered == LASER_RECOVER ? step.Ticks : 0);
	bool hit = false;
	for (int t = 0; t < ticks && m_Ticks < BEAM_TICKS; ++t) {
		++m_Ticks;
		const bool final = m_Ticks >= BEAM_TICKS;
		const VEC3 shoulder = GetShoulder();
		const CAPSULE beam{ shoulder, Vec3Add(shoulder, Vec3Scale(m_Direction, BEAM_LENGTH)), BEAM_RADIUS };
		for (int i = 0; i < FIGHTER_COUNT; ++i) {
			const TENTACLE_TARGET target = GetFighters().GetTarget(i, GetSlot());
			if (!target.Available || !CollisionCapsuleCapsule(beam, target.Body).Hit) continue;
			if (final) GetFighters().Launch(i, GetSlot(), AreaCalcLaunchDirection(m_Direction, FINAL_UP), TICK_DAMAGE, FINAL_POWER);
			else GetFighters().AddDamage(i, GetSlot(), TICK_DAMAGE);
			hit = true;
		}
	}
	if (hit) Rumble(TICK_RUMBLE_SMALL, TICK_RUMBLE_LARGE, TICK_RUMBLE_SECONDS);
	const float speed = dt / STRAIGHTEN_SECONDS;
	m_Straighten = m_Phase.IsIdle() ? fmaxf(0.0f, m_Straighten - speed) : fminf(1.0f, m_Straighten + speed);
}

void LaserArm::Draw()
{
	const VEC3 direction = ArmCalcBlendDirection(GetRestDirection(), m_Direction, m_Straighten);
	const XMFLOAT4 color = GetColor(!m_Phase.IsIdle());
	const VEC3 hand = DrawStubArm(direction, ARM_LENGTH, color);
	if (m_Phase.GetPhase() == LASER_CHARGE) {
		const float ratio = fminf(1.0f, m_Phase.GetTime() / PHASE_SECONDS[LASER_CHARGE]);
		ShapeDrawSphere({ hand.X, hand.Y, hand.Z }, CHARGE_RADIUS * ratio, color);
	}
	else if (m_Phase.GetPhase() == LASER_BEAM) {
		const VEC3 end = Vec3Add(GetShoulder(), Vec3Scale(m_Direction, BEAM_LENGTH));
		ShapeDrawCylinder({ hand.X, hand.Y, hand.Z }, { end.X, end.Y, end.Z }, BEAM_DRAW_RADIUS, color);
	}
}

bool LaserArm::IsActive() const
{
	return !m_Phase.IsIdle();
}

bool LaserArm::IsHolding() const
{
	return false;
}

float LaserArm::GetMoveScale() const
{
	const int phase = m_Phase.GetPhase();
	return phase == LASER_CHARGE || phase == LASER_BEAM ? MOVE_SCALE : 1.0f;
}
