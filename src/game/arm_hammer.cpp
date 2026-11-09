#include "game/arm_hammer.h"
#include "game/fightermanager.h"
#include "game/fighter.h"
#include "core/gametime.h"
#include "physics/areacalc.h"
#include "physics/armcalc.h"
#include "render/shape.h"

// 調整用の値
static constexpr float PHASE_SECONDS[] = { 0.5f, 0.1f, 0.6f }; // 振り上げ・叩き・休み
static constexpr float MOVE_SCALE = 0.5f;
static constexpr float IMPACT_FORWARD = 1.6f; // 足元から叩く所までの前への距離。
static constexpr float IMPACT_HEIGHT = 0.3f; // 叩く所の足元からの高さ。
static constexpr float IMPACT_RADIUS = 1.6f;
static constexpr float IMPACT_DAMAGE = 13.0f;
static constexpr float IMPACT_POWER = 2.4f;
static constexpr float IMPACT_UP = 1.25f; // 外へ 0.8 ＋ 上 1.0 と同じ向き（水平の単位方向に足す上の量）。
static constexpr float RAISE_PITCH = XMConvertToRadians(60.0f);
static constexpr float SLAM_PITCH = XMConvertToRadians(-30.0f);
static constexpr float STRAIGHTEN_SECONDS = 0.1f;
static constexpr float ARM_LENGTH = 0.45f;
static constexpr float HANDLE_LENGTH = 0.5f;
static constexpr float HANDLE_RADIUS = 0.04f;
static constexpr XMFLOAT3 HEAD_SIZE{ 0.45f, 0.3f, 0.3f };
static constexpr float HIT_RUMBLE_SMALL = 0.8f;
static constexpr float HIT_RUMBLE_LARGE = 1.0f;
static constexpr float HIT_RUMBLE_SECONDS = 0.2f;

enum HAMMER_PHASE { HAMMER_RAISE, HAMMER_SLAM, HAMMER_RECOVER, HAMMER_PHASE_COUNT };

HammerArm::HammerArm(int hand, const ARM_LINKS& links) : Arm(hand, links)
{
	Reset();
}

void HammerArm::Reset()
{
	m_Phase.Reset();
	m_Straighten = 0.0f;
}

void HammerArm::Update(const ARM_INPUT& input)
{
	const float dt = fmaxf(0.0f, GameTimeGetDeltaTime());
	if (GetPlayer().DidRespawn()) Reset();
	if (m_Phase.IsIdle() && input.Tap) {
		m_Direction = GetAim(false);
		m_Phase.Start(HAMMER_RAISE);
	}
	const ARM_PHASE_STEP step = m_Phase.Step(dt, PHASE_SECONDS, HAMMER_PHASE_COUNT);
	if (step.Entered == HAMMER_SLAM) {
		const XMFLOAT3 feet = GetPlayer().GetPosition();
		const VEC3 center = Vec3Add({ feet.x, feet.y + IMPACT_HEIGHT, feet.z }, Vec3Scale(m_Direction, IMPACT_FORWARD));
		bool hit = false;
		for (int i = 0; i < FIGHTER_COUNT; ++i) {
			const TENTACLE_TARGET target = GetFighters().GetTarget(i, GetSlot());
			if (!target.Available || !CollisionSphereCapsule({ center, IMPACT_RADIUS }, target.Body).Hit) continue;
			GetFighters().Launch(i, GetSlot(), AreaCalcRadialDirection(center, target.Body, IMPACT_UP, m_Direction),
				IMPACT_DAMAGE, IMPACT_POWER);
			hit = true;
		}
		if (hit) Rumble(HIT_RUMBLE_SMALL, HIT_RUMBLE_LARGE, HIT_RUMBLE_SECONDS);
	}
	const float speed = dt / STRAIGHTEN_SECONDS;
	m_Straighten = m_Phase.IsIdle() ? fmaxf(0.0f, m_Straighten - speed) : fminf(1.0f, m_Straighten + speed);
}

void HammerArm::Draw()
{
	const float pitch = m_Phase.GetPhase() == HAMMER_RAISE ? RAISE_PITCH : SLAM_PITCH;
	const VEC3 aim = ArmCalcPitchDirection(m_Direction, pitch);
	const VEC3 direction = ArmCalcBlendDirection(GetRestDirection(), aim, m_Straighten);
	const XMFLOAT4 color = GetColor(!m_Phase.IsIdle());
	const VEC3 hand = DrawStubArm(direction, ARM_LENGTH, color);
	const VEC3 head = Vec3Add(hand, Vec3Scale(direction, HANDLE_LENGTH));
	ShapeDrawCylinder({ hand.X, hand.Y, hand.Z }, { head.X, head.Y, head.Z }, HANDLE_RADIUS, color);
	ShapeDrawBox({ head.X, head.Y, head.Z }, HEAD_SIZE, color);
}

bool HammerArm::IsActive() const
{
	return !m_Phase.IsIdle();
}

bool HammerArm::IsHolding() const
{
	return false;
}

float HammerArm::GetMoveScale() const
{
	return m_Phase.IsIdle() ? 1.0f : MOVE_SCALE;
}
