#include "game/arm_whip.h"
#include "game/fightermanager.h"
#include "game/fighter.h"
#include "core/gametime.h"
#include "physics/areacalc.h"
#include "physics/armcalc.h"
#include "render/shape.h"

// 調整用の値
static constexpr float PHASE_SECONDS[] = { 0.15f, 0.2f, 0.4f }; // 溜め・振り・休み
static constexpr float SWEEP_MIN_RADIUS = 0.5f;
static constexpr float SWEEP_RADIUS = 4.5f;
static constexpr float SWEEP_HALF_ANGLE = XMConvertToRadians(70.0f);
static constexpr float SWEEP_HALF_HEIGHT = 1.2f;
static constexpr float SWEEP_DAMAGE = 6.0f;
static constexpr float SWEEP_POWER = 1.6f;
static constexpr float OUTWARD_WEIGHT = 0.6f; // 吹っ飛ぶ向き：外へ
static constexpr float SIDE_WEIGHT = 0.8f; // 吹っ飛ぶ向き：振る向き
static constexpr float SWEEP_UP = 0.2f;
static constexpr float STRAIGHTEN_SECONDS = 0.1f;
static constexpr float ARM_LENGTH = 0.4f;
static constexpr float REST_WHIP_LENGTH = 0.8f; // 待機で垂らす鞭の長さ。
static constexpr int WHIP_BALLS = 8;
static constexpr float WHIP_BALL_RADIUS = 0.06f;
static constexpr float HIT_RUMBLE_SMALL = 0.5f;
static constexpr float HIT_RUMBLE_LARGE = 0.3f;
static constexpr float HIT_RUMBLE_SECONDS = 0.12f;

enum WHIP_PHASE { WHIP_WINDUP, WHIP_SWEEP, WHIP_RECOVER, WHIP_PHASE_COUNT };

WhipArm::WhipArm(int hand, const ARM_LINKS& links) : Arm(hand, links)
{
	Reset();
}

void WhipArm::Reset()
{
	m_Phase.Reset();
	m_Straighten = 0.0f;
}

float WhipArm::GetSweepSign() const
{
	// 右手は右から左へ（yaw が減る向き）、左手は左から右へ振る。
	return GetHand() == 1 ? -1.0f : 1.0f;
}

void WhipArm::Update(bool trigger)
{
	const float dt = fmaxf(0.0f, GameTimeGetDeltaTime());
	if (GetPlayer().DidRespawn()) Reset();
	if (m_Phase.IsIdle() && trigger) {
		m_Direction = GetAim(false);
		m_Phase.Start(WHIP_WINDUP);
	}
	const ARM_PHASE_STEP step = m_Phase.Step(dt, PHASE_SECONDS, WHIP_PHASE_COUNT);
	if (step.Entered == WHIP_SWEEP) {
		const VEC3 shoulder = GetShoulder();
		const VEC3 side = AreaCalcRotateYaw(m_Direction, GetSweepSign() * XM_PIDIV2);
		bool hit = false;
		for (int i = 0; i < FIGHTER_COUNT; ++i) {
			const TENTACLE_TARGET target = GetFighters().GetTarget(i, GetSlot());
			if (!target.Available) continue;
			if (!AreaCalcInSector(shoulder, m_Direction, SWEEP_MIN_RADIUS, SWEEP_RADIUS, SWEEP_HALF_ANGLE, SWEEP_HALF_HEIGHT, target.Body)) continue;
			const VEC3 outward = AreaCalcRadialDirection(shoulder, target.Body, 0.0f, m_Direction);
			const VEC3 push = Vec3Add(Vec3Scale(outward, OUTWARD_WEIGHT), Vec3Scale(side, SIDE_WEIGHT));
			GetFighters().Launch(i, GetSlot(), AreaCalcLaunchDirection(push, SWEEP_UP), SWEEP_DAMAGE, SWEEP_POWER);
			hit = true;
		}
		if (hit) Rumble(HIT_RUMBLE_SMALL, HIT_RUMBLE_LARGE, HIT_RUMBLE_SECONDS);
	}
	const float speed = dt / STRAIGHTEN_SECONDS;
	m_Straighten = m_Phase.IsIdle() ? fmaxf(0.0f, m_Straighten - speed) : fminf(1.0f, m_Straighten + speed);
}

void WhipArm::Draw()
{
	const int phase = m_Phase.GetPhase();
	float ratio = 0.0f; // 0：振り始めの端、1：振り終わりの端
	if (phase == WHIP_SWEEP) ratio = fminf(1.0f, m_Phase.GetTime() / PHASE_SECONDS[WHIP_SWEEP]);
	else if (phase == WHIP_RECOVER) ratio = 1.0f;
	const VEC3 swing = AreaCalcRotateYaw(m_Direction, -GetSweepSign() * SWEEP_HALF_ANGLE * (1.0f - 2.0f * ratio));
	const VEC3 direction = ArmCalcBlendDirection(GetRestDirection(), swing, m_Straighten);
	const XMFLOAT4 color = GetColor(!m_Phase.IsIdle());
	const VEC3 hand = DrawStubArm(direction, ARM_LENGTH, color);
	// 振る間は扇の端まで伸ばし、ほかは手から垂らす。
	const float length = phase == WHIP_SWEEP ? SWEEP_RADIUS - ARM_LENGTH : REST_WHIP_LENGTH;
	const VEC3 whip = phase == WHIP_SWEEP ? swing : direction;
	for (int i = 1; i <= WHIP_BALLS; ++i) {
		const VEC3 p = Vec3Add(hand, Vec3Scale(whip, length * i / WHIP_BALLS));
		ShapeDrawSphere({ p.X, p.Y, p.Z }, WHIP_BALL_RADIUS, color);
	}
}

bool WhipArm::IsActive() const
{
	return !m_Phase.IsIdle();
}

bool WhipArm::IsHolding() const
{
	return false;
}
