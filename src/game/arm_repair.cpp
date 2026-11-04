#include "game/arm_repair.h"
#include "game/fightermanager.h"
#include "game/fighter.h"
#include "core/gametime.h"
#include "physics/armcalc.h"
#include "render/shape.h"

// 調整用の値
static constexpr float PHASE_SECONDS[] = { 1.5f, 4.0f }; // 修理・休み
static constexpr float HEAL_AMOUNT = 25.0f;
static constexpr float STRAIGHTEN_SECONDS = 0.1f;
static constexpr float ARM_LENGTH = 0.45f;
static constexpr float HOLD_PITCH = XMConvertToRadians(-40.0f); // 修理中の腕の上下（胸の前）。
static constexpr float ORB_RADIUS = 0.1f;
static constexpr float ORB_PULSE = 0.03f; // 球の大きさの揺れ（描画だけ）。
static constexpr float ORB_PULSE_SPEED = 10.0f;
static constexpr XMFLOAT4 ORB_COLOR{ 0.3f, 1.0f, 0.4f, 1.0f };
static constexpr float HEAL_RUMBLE_SMALL = 0.3f;
static constexpr float HEAL_RUMBLE_LARGE = 0.3f;
static constexpr float HEAL_RUMBLE_SECONDS = 0.2f;

enum REPAIR_PHASE { REPAIR_WORK, REPAIR_COOLDOWN, REPAIR_PHASE_COUNT };

RepairArm::RepairArm(int hand, const ARM_LINKS& links) : Arm(hand, links)
{
	Reset();
}

void RepairArm::Reset()
{
	m_Phase.Reset();
	m_Straighten = 0.0f;
}

void RepairArm::Update(bool trigger)
{
	const float dt = fmaxf(0.0f, GameTimeGetDeltaTime());
	if (GetPlayer().DidRespawn()) Reset();
	if (m_Phase.IsIdle() && trigger) m_Phase.Start(REPAIR_WORK);
	else if (m_Phase.GetPhase() == REPAIR_WORK && trigger) m_Phase.Start(REPAIR_COOLDOWN); // 取りやめ（直さない）
	const ARM_PHASE_STEP step = m_Phase.Step(dt, PHASE_SECONDS, REPAIR_PHASE_COUNT);
	if (step.Entered == REPAIR_COOLDOWN) {
		GetFighters().Heal(GetSlot(), HEAL_AMOUNT);
		Rumble(HEAL_RUMBLE_SMALL, HEAL_RUMBLE_LARGE, HEAL_RUMBLE_SECONDS);
	}
	const float speed = dt / STRAIGHTEN_SECONDS;
	m_Straighten = IsActive() ? fminf(1.0f, m_Straighten + speed) : fmaxf(0.0f, m_Straighten - speed);
}

void RepairArm::Draw()
{
	const VEC3 hold = ArmCalcPitchDirection(GetForward(), HOLD_PITCH);
	const VEC3 direction = ArmCalcBlendDirection(GetRestDirection(), hold, m_Straighten);
	const VEC3 hand = DrawStubArm(direction, ARM_LENGTH, GetColor(IsActive()));
	if (!IsActive()) return;
	const float radius = ORB_RADIUS + ORB_PULSE * sinf(m_Phase.GetTime() * ORB_PULSE_SPEED);
	ShapeDrawSphere({ hand.X, hand.Y, hand.Z }, radius, ORB_COLOR);
}

bool RepairArm::IsActive() const
{
	return m_Phase.GetPhase() == REPAIR_WORK;
}

bool RepairArm::IsHolding() const
{
	return false;
}

float RepairArm::GetMoveScale() const
{
	return IsActive() ? 0.0f : 1.0f;
}
