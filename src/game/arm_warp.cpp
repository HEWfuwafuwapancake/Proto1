#include "game/arm_warp.h"
#include "game/fightermanager.h"
#include "game/fighter.h"
#include "game/stage.h"
#include "core/gametime.h"
#include "physics/armcalc.h"
#include "physics/warpcalc.h"
#include "render/shape.h"

// 調整用の値
static constexpr float PHASE_SECONDS[] = { 0.15f, 1.5f }; // ため・休み
static constexpr float TARGET_GAP = 1.2f; // 相手の手前に残す距離。
static constexpr float FORWARD_DISTANCE = 5.0f; // 相手がいないときに進む距離。
static constexpr float STRAIGHTEN_SECONDS = 0.1f;
static constexpr float ARM_LENGTH = 0.5f;
static constexpr float ORB_RADIUS = 0.15f; // ため終わりの球の半径。
static constexpr float WARP_RUMBLE_SMALL = 0.5f;
static constexpr float WARP_RUMBLE_LARGE = 0.0f;
static constexpr float WARP_RUMBLE_SECONDS = 0.1f;

enum WARP_PHASE { WARP_CHARGE, WARP_COOLDOWN, WARP_PHASE_COUNT };

WarpArm::WarpArm(int hand, const ARM_LINKS& links) : Arm(hand, links)
{
	Reset();
}

void WarpArm::Reset()
{
	m_Phase.Reset();
	m_Straighten = 0.0f;
}

void WarpArm::Update(bool trigger)
{
	const float dt = fmaxf(0.0f, GameTimeGetDeltaTime());
	if (GetPlayer().DidRespawn()) Reset();
	if (m_Phase.IsIdle() && trigger) m_Phase.Start(WARP_CHARGE);
	const ARM_PHASE_STEP step = m_Phase.Step(dt, PHASE_SECONDS, WARP_PHASE_COUNT);
	if (step.Entered == WARP_COOLDOWN) {
		const XMFLOAT3 feet = GetPlayer().GetPosition();
		const VEC3 start{ feet.x, feet.y, feet.z };
		const int target = GetAimTarget();
		VEC3 desired = Vec3Add(start, Vec3Scale(GetAim(false), FORWARD_DISTANCE));
		if (target >= 0) {
			const CAPSULE body = GetFighters().GetTarget(target, GetSlot()).Body;
			desired = WarpCalcNearTarget(start, Vec3Scale(Vec3Add(body.Start, body.End), 0.5f), TARGET_GAP);
		}
		GetPlayer().Teleport(WarpCalcDestination(start, desired, GetStage().GetStage()));
		Rumble(WARP_RUMBLE_SMALL, WARP_RUMBLE_LARGE, WARP_RUMBLE_SECONDS);
	}
	const float speed = dt / STRAIGHTEN_SECONDS;
	const bool charging = m_Phase.GetPhase() == WARP_CHARGE;
	m_Straighten = charging ? fminf(1.0f, m_Straighten + speed) : fmaxf(0.0f, m_Straighten - speed);
}

void WarpArm::Draw()
{
	const VEC3 direction = ArmCalcBlendDirection(GetRestDirection(), GetForward(), m_Straighten);
	const bool charging = m_Phase.GetPhase() == WARP_CHARGE;
	const XMFLOAT4 color = GetColor(charging);
	const VEC3 hand = DrawStubArm(direction, ARM_LENGTH, color);
	if (charging) {
		const float ratio = fminf(1.0f, m_Phase.GetTime() / PHASE_SECONDS[WARP_CHARGE]);
		ShapeDrawSphere({ hand.X, hand.Y, hand.Z }, ORB_RADIUS * ratio, color);
	}
}

bool WarpArm::IsActive() const
{
	return m_Phase.GetPhase() == WARP_CHARGE;
}

bool WarpArm::IsHolding() const
{
	return false;
}
