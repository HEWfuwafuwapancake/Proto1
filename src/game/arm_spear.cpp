#include "game/arm_spear.h"
#include "game/fightermanager.h"
#include "game/fighter.h"
#include "core/gametime.h"
#include "physics/areacalc.h"
#include "physics/armcalc.h"
#include "render/shape.h"

// 調整用の値
static constexpr float PHASE_SECONDS[] = { 0.2f, 0.12f, 0.45f }; // 溜め・突き・休み
static constexpr float THRUST_RANGE = 3.5f; // 肩から当たりの線の終わりまで。
static constexpr float THRUST_RADIUS = 0.35f;
static constexpr float THRUST_DAMAGE = 9.0f;
static constexpr float THRUST_POWER = 2.0f;
static constexpr float THRUST_UP = 0.2f;
static constexpr float STRAIGHTEN_SECONDS = 0.1f;
static constexpr float ARM_LENGTH = 0.45f;
static constexpr float SHAFT_LENGTH = 1.0f; // 待機・溜めの柄の長さ。
static constexpr float SHAFT_RADIUS = 0.035f;
static constexpr float TIP_RADIUS = 0.08f;
static constexpr float HIT_RUMBLE_SMALL = 0.6f;
static constexpr float HIT_RUMBLE_LARGE = 0.4f;
static constexpr float HIT_RUMBLE_SECONDS = 0.15f;

enum SPEAR_PHASE { SPEAR_WINDUP, SPEAR_THRUST, SPEAR_RECOVER, SPEAR_PHASE_COUNT };

SpearArm::SpearArm(int hand, const ARM_LINKS& links) : Arm(hand, links)
{
	Reset();
}

void SpearArm::Reset()
{
	m_Phase.Reset();
	m_Straighten = 0.0f;
}

void SpearArm::Update(bool trigger)
{
	const float dt = fmaxf(0.0f, GameTimeGetDeltaTime());
	if (GetPlayer().DidRespawn()) Reset();
	if (m_Phase.IsIdle() && trigger) {
		m_Direction = GetAim(false);
		m_Phase.Start(SPEAR_WINDUP);
	}
	const ARM_PHASE_STEP step = m_Phase.Step(dt, PHASE_SECONDS, SPEAR_PHASE_COUNT);
	if (step.Entered == SPEAR_THRUST) {
		const VEC3 shoulder = GetShoulder();
		const CAPSULE thrust{ shoulder, Vec3Add(shoulder, Vec3Scale(m_Direction, THRUST_RANGE)), THRUST_RADIUS };
		bool hit = false;
		for (int i = 0; i < FIGHTER_COUNT; ++i) {
			const TENTACLE_TARGET target = GetFighters().GetTarget(i, GetSlot());
			if (!target.Available || !CollisionCapsuleCapsule(thrust, target.Body).Hit) continue;
			GetFighters().Launch(i, GetSlot(), AreaCalcLaunchDirection(m_Direction, THRUST_UP), THRUST_DAMAGE, THRUST_POWER);
			hit = true;
		}
		if (hit) Rumble(HIT_RUMBLE_SMALL, HIT_RUMBLE_LARGE, HIT_RUMBLE_SECONDS);
	}
	const float speed = dt / STRAIGHTEN_SECONDS;
	m_Straighten = m_Phase.IsIdle() ? fmaxf(0.0f, m_Straighten - speed) : fminf(1.0f, m_Straighten + speed);
}

void SpearArm::Draw()
{
	const VEC3 direction = ArmCalcBlendDirection(GetRestDirection(), m_Direction, m_Straighten);
	const XMFLOAT4 color = GetColor(!m_Phase.IsIdle());
	const VEC3 hand = DrawStubArm(direction, ARM_LENGTH, color);
	// 突く間だけ、当たりの線の終わりまで柄を伸ばす。
	const float shaft = m_Phase.GetPhase() == SPEAR_THRUST ? THRUST_RANGE - ARM_LENGTH : SHAFT_LENGTH;
	const VEC3 tip = Vec3Add(hand, Vec3Scale(direction, shaft));
	ShapeDrawCylinder({ hand.X, hand.Y, hand.Z }, { tip.X, tip.Y, tip.Z }, SHAFT_RADIUS, color);
	ShapeDrawSphere({ tip.X, tip.Y, tip.Z }, TIP_RADIUS, color);
}

bool SpearArm::IsActive() const
{
	return !m_Phase.IsIdle();
}

bool SpearArm::IsHolding() const
{
	return false;
}
