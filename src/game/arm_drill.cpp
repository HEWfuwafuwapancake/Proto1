#include "game/arm.h"
#include "game/camera.h"
#include "game/arm_drill.h"
#include "game/dummy.h"
#include "game/player.h"
#include "core/gametime.h"
#include "physics/armcalc.h"
#include "physics/drillcalc.h"
#include "input/pad.h"

// 調整用の値
static constexpr float MODEL_SCALE = 0.08f;
static constexpr DRILL_GRIP DRILL_MODEL_GRIP{
	{ 0.311337f, -5.719436f, -5.081741f }, { 0.0f, -0.707106781f, 0.707106781f }, MODEL_SCALE
};
static constexpr float HIT_START = 12.0f;
static constexpr float HIT_END = 18.060484f;
static constexpr float BIT_RADIUS = 0.35f;
static constexpr float STRIKE_SECONDS = 0.4f;
static constexpr float BORE_SECONDS = 0.5f;
static constexpr float TICK_SECONDS = 0.1f;
static constexpr int TICK_COUNT = 5;
static constexpr float COOLDOWN_SECONDS = 0.3f;
static constexpr DRILL_PARAMS PARAMS{
	DRILL_MODEL_GRIP, HIT_START, HIT_END, BIT_RADIUS, STRIKE_SECONDS, BORE_SECONDS, TICK_SECONDS, TICK_COUNT, COOLDOWN_SECONDS
};
static constexpr float TICK_DAMAGE = 4.0f;
static constexpr float PUSH_POWER = 1.5f;
static constexpr float PUSH_UP = 0.15f;
static constexpr float DRILL_AIM_SECONDS = 0.1f;
static constexpr float SPIN_SPEED = 20.0f;
static constexpr float BASE_LENGTH = 15.189075f;
static constexpr float BIT_LENGTH = 18.060484f;
// 触腕の「つかむ」「投げる」と同じ強さ・秒数。
static constexpr float TICK_RUMBLE_SMALL = 0.4f;
static constexpr float TICK_RUMBLE_LARGE = 0.0f;
static constexpr float TICK_RUMBLE_SECONDS = 0.1f;
static constexpr float PUSH_RUMBLE_SMALL = 0.6f;
static constexpr float PUSH_RUMBLE_LARGE = 0.6f;
static constexpr float PUSH_RUMBLE_SECONDS = 0.15f;

void DrillArm::Reset()
{
	m_Arm.Reset();
	m_Straighten = 0.0f;
	m_Spin = 0.0f;
}

DrillArm::DrillArm(int hand, const ARM_LINKS& links) : Arm(hand, links)
{
	Reset();
	m_BaseModel = LoadModel("assets/models/arm_drill_base.obj");
	m_BitModel = LoadModel("assets/models/arm_drill_bit.obj");
}

DrillArm::~DrillArm()
{
	Reset();
}

bool DrillArm::IsThrusting() const
{
	return m_Arm.GetState() == DRILL_STRIKING || m_Arm.GetState() == DRILL_BORING;
}

void DrillArm::Update(bool trigger)
{
	const int hand = GetHand();
	DrillCalc& arm = m_Arm;
	const VEC3 origin = GetShoulder();
	const float yaw = GetCamera().GetYaw();
	const VEC3 forward{ sinf(yaw), 0.0f, cosf(yaw) };
	TENTACLE_TARGET targets[DUMMY_COUNT];
	for (int i = 0; i < DUMMY_COUNT; ++i) targets[i] = GetDummies().GetTarget(i);
	const int target = GetCamera().GetTarget();
	const bool hasTarget = target >= 0 && target < DUMMY_COUNT;
	const VEC3 center = hasTarget
		? Vec3Scale(Vec3Add(targets[target].Body.Start, targets[target].Body.End), 0.5f) : VEC3{};
	const VEC3 aim = TentacleCalcAim(origin, center, hasTarget, forward);
	const float dt = fmaxf(0.0f, GameTimeGetDeltaTime());
	const DRILL_EVENT event = arm.Step(origin, aim, hand == 1, trigger, GetPlayer().DidRespawn(),
		dt, PARAMS, targets, DUMMY_COUNT);
	for (int tick = 0; tick < event.DamageTicks; ++tick) {
		GetDummies().AddDamage(event.Target, TICK_DAMAGE);
		PadRumble(TICK_RUMBLE_SMALL, TICK_RUMBLE_LARGE, TICK_RUMBLE_SECONDS);
	}
	if (event.Push) {
		GetDummies().Launch(event.Target, DrillCalcPushDirection(arm.GetDirection(), PUSH_UP), 0.0f, PUSH_POWER);
		PadRumble(PUSH_RUMBLE_SMALL, PUSH_RUMBLE_LARGE, PUSH_RUMBLE_SECONDS);
	}
	const float step = dt / DRILL_AIM_SECONDS;
	m_Straighten = IsThrusting()
		? fminf(1.0f, m_Straighten + step)
		: fmaxf(0.0f, m_Straighten - step);
	if (IsThrusting()) m_Spin = fmodf(m_Spin + SPIN_SPEED * dt, XM_2PI);
}

void DrillArm::Draw()
{
	const int hand = GetHand();
	const DrillCalc& arm = m_Arm;
	const VEC3 origin = GetShoulder();
	const VEC3 direction = ArmCalcBlendDirection(
		GetRestDirection(),
		DrillCalcThrustArmDirection(arm.GetDirection(), PARAMS.Grip), m_Straighten);
	const XMFLOAT4 color = GetColor(arm.GetState() == DRILL_BORING);
	// 付け根もビットも、常に同じ長さ・倍率で描く。間の円柱や伸縮はない。
	const VEC3 end = Vec3Add(origin, Vec3Scale(direction, BASE_LENGTH * MODEL_SCALE));
	DrawPiece(m_BaseModel, origin, end, BASE_LENGTH, MODEL_SCALE, color, XM_PI);
	const DRILL_AXIS axis = DrillCalcAxis(origin, direction, PARAMS.Grip, hand == 1);
	const VEC3 tip = Vec3Add(axis.Rear, Vec3Scale(axis.Direction, BIT_LENGTH * MODEL_SCALE));
	DrawPiece(m_BitModel, axis.Rear, tip, BIT_LENGTH, MODEL_SCALE, color, m_Spin);
}

bool DrillArm::IsActive() const
{
	return m_Arm.GetState() != DRILL_IDLE;
}

bool DrillArm::IsHolding() const
{
	// くい込み中も敵をつかまない。カメラはロックオンを続ける。
	return false;
}
