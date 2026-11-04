#include "game/arm_axe.h"
#include "game/fightermanager.h"
#include "game/fighter.h"
#include "core/gametime.h"
#include "physics/areacalc.h"
#include "physics/armcalc.h"
#include "render/shape.h"

// 調整用の値
static constexpr float PHASE_SECONDS[] = { 0.35f, 0.15f, 0.6f }; // 振りかぶり・振り・休み
static constexpr float MOVE_SCALE = 0.5f;
static constexpr float SWING_RADIUS = 2.2f;
static constexpr float SWING_HALF_ANGLE = XMConvertToRadians(80.0f);
static constexpr float SWING_HALF_HEIGHT = 1.0f;
static constexpr float SWING_DAMAGE = 11.0f;
static constexpr float SWING_POWER = 2.0f;
static constexpr float SWING_UP = 0.2f;
static constexpr float BREAK_STUN = 1.2f; // ガードを崩したときのしびれ（吹っ飛ばさないので追撃できる）。
static constexpr float BREAK_DAMAGE = 6.0f;
static constexpr float STRAIGHTEN_SECONDS = 0.1f;
static constexpr float ARM_LENGTH = 0.45f;
static constexpr float HANDLE_LENGTH = 0.55f;
static constexpr float HANDLE_RADIUS = 0.035f;
static constexpr XMFLOAT3 HEAD_SIZE{ 0.3f, 0.3f, 0.3f };
static constexpr float SWING_DRAW_ANGLE = XMConvertToRadians(80.0f); // 振りの描画で回す片側の角度。
static constexpr float HIT_RUMBLE_SMALL = 0.7f;
static constexpr float HIT_RUMBLE_LARGE = 0.8f;
static constexpr float HIT_RUMBLE_SECONDS = 0.18f;

enum AXE_PHASE { AXE_WINDUP, AXE_SWING, AXE_RECOVER, AXE_PHASE_COUNT };

AxeArm::AxeArm(int hand, const ARM_LINKS& links) : Arm(hand, links)
{
	Reset();
}

void AxeArm::Reset()
{
	m_Phase.Reset();
	m_Straighten = 0.0f;
}

void AxeArm::Update(bool trigger)
{
	const float dt = fmaxf(0.0f, GameTimeGetDeltaTime());
	if (GetPlayer().DidRespawn()) Reset();
	if (m_Phase.IsIdle() && trigger) {
		m_Direction = GetAim(false);
		m_Phase.Start(AXE_WINDUP);
	}
	const ARM_PHASE_STEP step = m_Phase.Step(dt, PHASE_SECONDS, AXE_PHASE_COUNT);
	if (step.Entered == AXE_SWING) {
		const VEC3 shoulder = GetShoulder();
		bool hit = false;
		for (int i = 0; i < FIGHTER_COUNT; ++i) {
			const TENTACLE_TARGET target = GetFighters().GetTarget(i, GetSlot());
			if (!target.Available) continue;
			if (!AreaCalcInSector(shoulder, m_Direction, 0.0f, SWING_RADIUS, SWING_HALF_ANGLE, SWING_HALF_HEIGHT, target.Body)) continue;
			// ガードしている相手はガードを崩してしびれさせ、していなければ吹っ飛ばす。
			if (GetFighters().IsGuarded(i, GetSlot())) GetFighters().BreakGuard(i, GetSlot(), BREAK_STUN, BREAK_DAMAGE);
			else GetFighters().Launch(i, GetSlot(), AreaCalcLaunchDirection(m_Direction, SWING_UP), SWING_DAMAGE, SWING_POWER);
			hit = true;
		}
		if (hit) Rumble(HIT_RUMBLE_SMALL, HIT_RUMBLE_LARGE, HIT_RUMBLE_SECONDS);
	}
	const float speed = dt / STRAIGHTEN_SECONDS;
	m_Straighten = m_Phase.IsIdle() ? fmaxf(0.0f, m_Straighten - speed) : fminf(1.0f, m_Straighten + speed);
}

void AxeArm::Draw()
{
	// 振りかぶりは外側の端、振りで内側の端まで回す（左手は鏡写し）。
	const float side = GetHand() == 0 ? -1.0f : 1.0f;
	float ratio = 0.0f;
	if (m_Phase.GetPhase() == AXE_SWING) ratio = fminf(1.0f, m_Phase.GetTime() / PHASE_SECONDS[AXE_SWING]);
	else if (m_Phase.GetPhase() == AXE_RECOVER) ratio = 1.0f;
	const VEC3 swing = AreaCalcRotateYaw(m_Direction, side * SWING_DRAW_ANGLE * (1.0f - 2.0f * ratio));
	const VEC3 direction = ArmCalcBlendDirection(GetRestDirection(), swing, m_Straighten);
	const XMFLOAT4 color = GetColor(!m_Phase.IsIdle());
	const VEC3 hand = DrawStubArm(direction, ARM_LENGTH, color);
	const VEC3 head = Vec3Add(hand, Vec3Scale(direction, HANDLE_LENGTH));
	ShapeDrawCylinder({ hand.X, hand.Y, hand.Z }, { head.X, head.Y, head.Z }, HANDLE_RADIUS, color);
	ShapeDrawBox({ head.X, head.Y, head.Z }, HEAD_SIZE, color);
}

bool AxeArm::IsActive() const
{
	return !m_Phase.IsIdle();
}

bool AxeArm::IsHolding() const
{
	return false;
}

float AxeArm::GetMoveScale() const
{
	return m_Phase.GetPhase() == AXE_WINDUP ? MOVE_SCALE : 1.0f;
}
