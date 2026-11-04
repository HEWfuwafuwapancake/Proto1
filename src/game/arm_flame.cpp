#include "game/arm_flame.h"
#include "game/fightermanager.h"
#include "game/fighter.h"
#include "core/gametime.h"
#include "physics/areacalc.h"
#include "physics/armcalc.h"
#include "render/shape.h"

// 調整用の値
static constexpr float PHASE_SECONDS[] = { 1.5f, 0.5f }; // 炎・休み
static constexpr float TICK_SECONDS = 0.15f;
static constexpr float MOVE_SCALE = 0.5f;
static constexpr float FLAME_RADIUS = 3.5f;
static constexpr float FLAME_HALF_ANGLE = XMConvertToRadians(25.0f);
static constexpr float FLAME_HALF_HEIGHT = 0.9f;
static constexpr float TICK_DAMAGE = 1.0f;
static constexpr float BURN_SECONDS = 2.0f;
static constexpr float BURN_PER_SECOND = 2.0f;
static constexpr float STRAIGHTEN_SECONDS = 0.1f;
static constexpr float ARM_LENGTH = 0.5f;
static constexpr float NOZZLE_LENGTH = 0.2f;
static constexpr float NOZZLE_RADIUS = 0.06f;
static constexpr int FLAME_BALLS = 5;
static constexpr float FLAME_BALL_MIN = 0.15f; // 手元の炎の球の半径。
static constexpr float FLAME_BALL_MAX = 0.6f; // 先の炎の球の半径。
static constexpr float FLAME_WOBBLE = 0.15f; // 炎の球を前後に揺らす量（描画だけ）。
static constexpr XMFLOAT4 FLAME_COLOR{ 1.0f, 0.45f, 0.1f, 1.0f };
static constexpr float TICK_RUMBLE_SMALL = 0.25f;
static constexpr float TICK_RUMBLE_LARGE = 0.0f;
static constexpr float TICK_RUMBLE_SECONDS = 0.15f;

enum FLAME_PHASE { FLAME_FIRE, FLAME_RECOVER, FLAME_PHASE_COUNT };

FlameArm::FlameArm(int hand, const ARM_LINKS& links) : Arm(hand, links)
{
	Reset();
}

void FlameArm::Reset()
{
	m_Phase.Reset();
	m_Straighten = 0.0f;
}

void FlameArm::Update(bool trigger)
{
	const float dt = fmaxf(0.0f, GameTimeGetDeltaTime());
	if (GetPlayer().DidRespawn()) Reset();
	if (m_Phase.IsIdle() && trigger) m_Phase.Start(FLAME_FIRE);
	else if (m_Phase.GetPhase() == FLAME_FIRE && trigger) m_Phase.Start(FLAME_RECOVER);
	if (m_Phase.GetPhase() == FLAME_FIRE) m_Direction = GetAim(false);
	const ARM_PHASE_STEP step = m_Phase.Step(dt, PHASE_SECONDS, FLAME_PHASE_COUNT, TICK_SECONDS);
	const int ticks = m_Phase.GetPhase() == FLAME_FIRE || step.Entered == FLAME_RECOVER ? step.Ticks : 0;
	bool hit = false;
	for (int t = 0; t < ticks; ++t) {
		const VEC3 shoulder = GetShoulder();
		for (int i = 0; i < FIGHTER_COUNT; ++i) {
			const TENTACLE_TARGET target = GetFighters().GetTarget(i, GetSlot());
			if (!target.Available) continue;
			if (!AreaCalcInSector(shoulder, m_Direction, 0.0f, FLAME_RADIUS, FLAME_HALF_ANGLE, FLAME_HALF_HEIGHT, target.Body)) continue;
			GetFighters().AddDamage(i, GetSlot(), TICK_DAMAGE);
			GetFighters().ApplyStatus(i, GetSlot(), STATUS_BURN, BURN_SECONDS, BURN_PER_SECOND);
			hit = true;
		}
	}
	if (hit) Rumble(TICK_RUMBLE_SMALL, TICK_RUMBLE_LARGE, TICK_RUMBLE_SECONDS);
	const float speed = dt / STRAIGHTEN_SECONDS;
	m_Straighten = IsActive() ? fminf(1.0f, m_Straighten + speed) : fmaxf(0.0f, m_Straighten - speed);
}

void FlameArm::Draw()
{
	const VEC3 direction = ArmCalcBlendDirection(GetRestDirection(), m_Direction, m_Straighten);
	const XMFLOAT4 color = GetColor(IsActive());
	const VEC3 hand = DrawStubArm(direction, ARM_LENGTH, color);
	const VEC3 nozzle = Vec3Add(hand, Vec3Scale(direction, NOZZLE_LENGTH));
	ShapeDrawCylinder({ hand.X, hand.Y, hand.Z }, { nozzle.X, nozzle.Y, nozzle.Z }, NOZZLE_RADIUS, color);
	if (!IsActive()) return;
	// 当たりの扇の中心の線に、だんだん大きくなる炎の球を並べる。
	const VEC3 shoulder = GetShoulder();
	for (int i = 1; i <= FLAME_BALLS; ++i) {
		const float ratio = static_cast<float>(i) / FLAME_BALLS;
		const float wobble = FLAME_WOBBLE * sinf(m_Phase.GetTime() * 25.0f + i);
		const VEC3 p = Vec3Add(shoulder, Vec3Scale(m_Direction, FLAME_RADIUS * ratio + wobble));
		ShapeDrawSphere({ p.X, p.Y, p.Z }, FLAME_BALL_MIN + (FLAME_BALL_MAX - FLAME_BALL_MIN) * ratio, FLAME_COLOR);
	}
}

bool FlameArm::IsActive() const
{
	return m_Phase.GetPhase() == FLAME_FIRE;
}

bool FlameArm::IsHolding() const
{
	return false;
}

float FlameArm::GetMoveScale() const
{
	return IsActive() ? MOVE_SCALE : 1.0f;
}
