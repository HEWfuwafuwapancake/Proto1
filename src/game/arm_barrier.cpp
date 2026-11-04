#include "game/arm_barrier.h"
#include "game/fightermanager.h"
#include "game/fighter.h"
#include "core/gametime.h"
#include "physics/areacalc.h"
#include "render/shape.h"

// 調整用の値
static constexpr float PHASE_SECONDS[] = { 1.5f, 4.0f }; // バリア・休み
static constexpr float MOVE_SCALE = 0.7f;
static constexpr float BURST_RADIUS = 2.0f;
static constexpr float BURST_SPEED = 8.0f; // % を変えずに押す速さ。
static constexpr float BURST_UP = 0.2f;
static constexpr float BURST_SHOW_SECONDS = 0.15f;
static constexpr float RING_RADIUS = 0.9f; // バリアの球を並べる半径。
static constexpr float RING_HEIGHTS[] = { 0.4f, 0.9f, 1.4f }; // 足元からの高さ（3段）
static constexpr int RING_BALLS = 12;
static constexpr float RING_BALL_RADIUS = 0.08f;
static constexpr XMFLOAT4 BARRIER_COLOR{ 0.4f, 0.8f, 1.0f, 1.0f };
static constexpr float ARM_LENGTH = 0.45f;
static constexpr float BURST_RUMBLE_SMALL = 0.6f;
static constexpr float BURST_RUMBLE_LARGE = 0.6f;
static constexpr float BURST_RUMBLE_SECONDS = 0.15f;

enum BARRIER_PHASE { BARRIER_UP, BARRIER_COOLDOWN, BARRIER_PHASE_COUNT };

BarrierArm::BarrierArm(int hand, const ARM_LINKS& links) : Arm(hand, links)
{
	Reset();
}

void BarrierArm::Reset()
{
	m_Phase.Reset();
	m_BurstTime = 0.0f;
}

void BarrierArm::Update(bool trigger)
{
	const float dt = fmaxf(0.0f, GameTimeGetDeltaTime());
	if (GetPlayer().DidRespawn()) Reset();
	m_BurstTime = fmaxf(0.0f, m_BurstTime - dt);
	if (m_Phase.IsIdle() && trigger) {
		m_Phase.Start(BARRIER_UP);
	}
	else if (m_Phase.GetPhase() == BARRIER_UP && trigger) {
		// 弾けて、まわりの相手を外へ押す（% は変えない）。
		const CAPSULE self = GetPlayer().GetBody();
		const VEC3 center = Vec3Scale(Vec3Add(self.Start, self.End), 0.5f);
		bool hit = false;
		for (int i = 0; i < FIGHTER_COUNT; ++i) {
			const TENTACLE_TARGET target = GetFighters().GetTarget(i, GetSlot());
			if (!target.Available || !CollisionSphereCapsule({ center, BURST_RADIUS }, target.Body).Hit) continue;
			GetFighters().Push(i, GetSlot(), Vec3Scale(AreaCalcRadialDirection(center, target.Body, BURST_UP, GetForward()), BURST_SPEED));
			hit = true;
		}
		if (hit) Rumble(BURST_RUMBLE_SMALL, BURST_RUMBLE_LARGE, BURST_RUMBLE_SECONDS);
		m_BurstTime = BURST_SHOW_SECONDS;
		m_Phase.Start(BARRIER_COOLDOWN);
	}
	m_Phase.Step(dt, PHASE_SECONDS, BARRIER_PHASE_COUNT);
}

void BarrierArm::DrawRing(float radius, const XMFLOAT4& color) const
{
	const XMFLOAT3 feet = GetPlayer().GetPosition();
	for (float height : RING_HEIGHTS) {
		for (int k = 0; k < RING_BALLS; ++k) {
			const float a = XM_2PI * k / RING_BALLS;
			ShapeDrawSphere({ feet.x + cosf(a) * radius, feet.y + height, feet.z + sinf(a) * radius }, RING_BALL_RADIUS, color);
		}
	}
}

void BarrierArm::Draw()
{
	const bool up = m_Phase.GetPhase() == BARRIER_UP;
	DrawStubArm(GetRestDirection(), ARM_LENGTH, GetColor(up));
	if (up) DrawRing(RING_RADIUS, BARRIER_COLOR);
	if (m_BurstTime > 0.0f) DrawRing(BURST_RADIUS, BARRIER_COLOR);
}

bool BarrierArm::IsActive() const
{
	return false;
}

bool BarrierArm::IsHolding() const
{
	return false;
}

float BarrierArm::GetMoveScale() const
{
	return m_Phase.GetPhase() == BARRIER_UP ? MOVE_SCALE : 1.0f;
}

GUARD BarrierArm::GetGuard() const
{
	GUARD guard;
	if (m_Phase.GetPhase() == BARRIER_UP) guard.Kind = GUARD_ALL;
	return guard;
}
