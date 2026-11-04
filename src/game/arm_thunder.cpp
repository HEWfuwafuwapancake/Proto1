#include "game/arm_thunder.h"
#include "game/fightermanager.h"
#include "game/fighter.h"
#include "core/gametime.h"
#include "physics/areacalc.h"
#include "physics/armcalc.h"
#include "physics/thundercalc.h"
#include "render/shape.h"

// 調整用の値
static constexpr float PHASE_SECONDS[] = { 0.2f, 1.0f }; // ため・休み
static constexpr float BOLT_RANGE = 7.0f;
static constexpr float BOLT_RADIUS = 0.6f;
static constexpr float FIRST_DAMAGE = 6.0f;
static constexpr float FIRST_STUN = 0.8f;
static constexpr float CHAIN_RANGE = 4.0f;
static constexpr float CHAIN_DAMAGE = 4.0f;
static constexpr float CHAIN_STUN = 0.5f;
static constexpr float BOLT_SHOW_SECONDS = 0.15f;
static constexpr float STRAIGHTEN_SECONDS = 0.1f;
static constexpr float ARM_LENGTH = 0.55f;
static constexpr float BOLT_DRAW_RADIUS = 0.05f;
static constexpr float CHARGE_RADIUS = 0.12f;
static constexpr XMFLOAT4 BOLT_COLOR{ 1.0f, 0.95f, 0.3f, 1.0f };
static constexpr float HIT_RUMBLE_SMALL = 0.8f;
static constexpr float HIT_RUMBLE_LARGE = 0.3f;
static constexpr float HIT_RUMBLE_SECONDS = 0.2f;

enum THUNDER_PHASE { THUNDER_CHARGE, THUNDER_RECOVER, THUNDER_PHASE_COUNT };

ThunderArm::ThunderArm(int hand, const ARM_LINKS& links) : Arm(hand, links)
{
	Reset();
}

void ThunderArm::Reset()
{
	m_Phase.Reset();
	m_PointCount = 0;
	m_BoltTime = 0.0f;
	m_Straighten = 0.0f;
}

void ThunderArm::Update(bool trigger)
{
	const float dt = fmaxf(0.0f, GameTimeGetDeltaTime());
	if (GetPlayer().DidRespawn()) Reset();
	if (m_Phase.IsIdle() && trigger) {
		m_Direction = GetAim(false);
		m_Phase.Start(THUNDER_CHARGE);
	}
	m_BoltTime = fmaxf(0.0f, m_BoltTime - dt);
	const ARM_PHASE_STEP step = m_Phase.Step(dt, PHASE_SECONDS, THUNDER_PHASE_COUNT);
	if (step.Entered == THUNDER_RECOVER) {
		TENTACLE_TARGET targets[FIGHTER_COUNT];
		for (int i = 0; i < FIGHTER_COUNT; ++i) targets[i] = GetFighters().GetTarget(i, GetSlot());
		const VEC3 shoulder = GetShoulder();
		const int first = AreaCalcFirstOnRay(shoulder, m_Direction, BOLT_RANGE, BOLT_RADIUS, targets, FIGHTER_COUNT, nullptr);
		int order[MAX_CHAIN];
		const int chain = ThunderCalcChain(targets, FIGHTER_COUNT, first, CHAIN_RANGE, MAX_CHAIN, order);
		m_Points[0] = shoulder;
		m_PointCount = 1;
		for (int k = 0; k < chain; ++k) {
			const int i = order[k];
			GetFighters().AddDamage(i, GetSlot(), k == 0 ? FIRST_DAMAGE : CHAIN_DAMAGE);
			GetFighters().ApplyStatus(i, GetSlot(), STATUS_STUN, k == 0 ? FIRST_STUN : CHAIN_STUN, 0.0f);
			m_Points[m_PointCount++] = Vec3Scale(Vec3Add(targets[i].Body.Start, targets[i].Body.End), 0.5f);
		}
		// 外れたときは、届く所までの雷だけ見せる。
		if (chain == 0) m_Points[m_PointCount++] = Vec3Add(shoulder, Vec3Scale(m_Direction, BOLT_RANGE));
		m_BoltTime = BOLT_SHOW_SECONDS;
		if (chain > 0) Rumble(HIT_RUMBLE_SMALL, HIT_RUMBLE_LARGE, HIT_RUMBLE_SECONDS);
	}
	const float speed = dt / STRAIGHTEN_SECONDS;
	const bool aiming = IsActive() || m_BoltTime > 0.0f;
	m_Straighten = aiming ? fminf(1.0f, m_Straighten + speed) : fmaxf(0.0f, m_Straighten - speed);
}

void ThunderArm::Draw()
{
	const VEC3 direction = ArmCalcBlendDirection(GetRestDirection(), m_Direction, m_Straighten);
	const VEC3 hand = DrawStubArm(direction, ARM_LENGTH, GetColor(IsActive()));
	if (IsActive()) {
		const float ratio = fminf(1.0f, m_Phase.GetTime() / PHASE_SECONDS[THUNDER_CHARGE]);
		ShapeDrawSphere({ hand.X, hand.Y, hand.Z }, CHARGE_RADIUS * ratio, BOLT_COLOR);
	}
	if (m_BoltTime <= 0.0f) return;
	for (int k = 0; k + 1 < m_PointCount; ++k) {
		const VEC3 a = k == 0 ? hand : m_Points[k];
		const VEC3 b = m_Points[k + 1];
		ShapeDrawCylinder({ a.X, a.Y, a.Z }, { b.X, b.Y, b.Z }, BOLT_DRAW_RADIUS, BOLT_COLOR);
	}
}

bool ThunderArm::IsActive() const
{
	return m_Phase.GetPhase() == THUNDER_CHARGE;
}

bool ThunderArm::IsHolding() const
{
	return false;
}
