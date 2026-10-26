#include "game/arm.h"
#include "game/camera.h"
#include "game/dummy.h"
#include "game/player.h"
#include "core/gametime.h"
#include "physics/armcalc.h"
#include "physics/drillcalc.h"
#include "input/pad.h"

// 調整用の値
static constexpr float BIT_START = 1.2f;
static constexpr float REACH = 2.65f;
static constexpr float BIT_RADIUS = 0.35f;
static constexpr float STRIKE_SECONDS = 0.4f;
static constexpr float BORE_SECONDS = 0.5f;
static constexpr float TICK_SECONDS = 0.1f;
static constexpr int TICK_COUNT = 5;
static constexpr float COOLDOWN_SECONDS = 0.3f;
static constexpr DRILL_PARAMS PARAMS{
	BIT_START, REACH, BIT_RADIUS, STRIKE_SECONDS, BORE_SECONDS, TICK_SECONDS, TICK_COUNT, COOLDOWN_SECONDS
};
static constexpr float TICK_DAMAGE = 4.0f;
static constexpr float PUSH_POWER = 1.5f;
static constexpr float PUSH_UP = 0.15f;
static constexpr float DRILL_AIM_SECONDS = 0.1f;
static constexpr float SPIN_SPEED = 20.0f;
static constexpr float MODEL_SCALE = 0.08f;
static constexpr float BASE_LENGTH = 15.189075f;
static constexpr float BIT_LENGTH = 18.060484f;
static constexpr VEC3 BIT_OFFSET{ 0.31f, -15.19f, 0.0f };
// 触腕の「つかむ」「投げる」と同じ強さ・秒数。
static constexpr float TICK_RUMBLE_SMALL = 0.4f;
static constexpr float TICK_RUMBLE_LARGE = 0.0f;
static constexpr float TICK_RUMBLE_SECONDS = 0.1f;
static constexpr float PUSH_RUMBLE_SMALL = 0.6f;
static constexpr float PUSH_RUMBLE_LARGE = 0.6f;
static constexpr float PUSH_RUMBLE_SECONDS = 0.15f;

static DRILL g_Arms[2];
static float g_Straighten[2] = {}; // 描画だけのまっすぐ度（0：待機、1：水平の突き）。
static float g_Spin[2] = {};
static int g_BaseModel[2] = { -1, -1 };
static int g_BitModel[2] = { -1, -1 };

static void ResetHand(int hand)
{
	DrillCalcReset(g_Arms[hand]);
	g_Straighten[hand] = 0.0f;
	g_Spin[hand] = 0.0f;
}

static void Initialize(void)
{
	for (int hand = 0; hand < 2; ++hand) {
		ResetHand(hand);
		g_BaseModel[hand] = ArmLoadModel("assets/models/arm_drill_base.obj", hand);
		g_BitModel[hand] = ArmLoadModel("assets/models/arm_drill_bit.obj", hand);
	}
}

static void Finalize(void)
{
	for (int hand = 0; hand < 2; ++hand) ResetHand(hand);
}

static bool IsThrusting(int hand)
{
	return g_Arms[hand].State == DRILL_STRIKING || g_Arms[hand].State == DRILL_BORING;
}

static void UpdateHand(int hand, bool trigger)
{
	DRILL& arm = g_Arms[hand];
	const VEC3 origin = ArmGetShoulder(hand);
	const float yaw = CameraGetYaw();
	const VEC3 forward{ sinf(yaw), 0.0f, cosf(yaw) };
	TENTACLE_TARGET targets[DUMMY_COUNT];
	for (int i = 0; i < DUMMY_COUNT; ++i) targets[i] = DummyGetTarget(i);
	const int target = CameraGetTarget();
	const bool hasTarget = target >= 0 && target < DUMMY_COUNT;
	const VEC3 center = hasTarget
		? Vec3Scale(Vec3Add(targets[target].Body.Start, targets[target].Body.End), 0.5f) : VEC3{};
	const VEC3 aim = TentacleCalcAim(origin, center, hasTarget, forward);
	const float dt = fmaxf(0.0f, GameTimeGetDeltaTime());
	const DRILL_EVENT event = DrillCalcStep(arm, origin, aim, trigger, PlayerDidRespawn(),
		dt, PARAMS, targets, DUMMY_COUNT);
	for (int tick = 0; tick < event.DamageTicks; ++tick) {
		DummyAddDamage(event.Target, TICK_DAMAGE);
		PadRumble(TICK_RUMBLE_SMALL, TICK_RUMBLE_LARGE, TICK_RUMBLE_SECONDS);
	}
	if (event.Push) {
		DummyLaunch(event.Target, DrillCalcPushDirection(arm.Direction, PUSH_UP), 0.0f, PUSH_POWER);
		PadRumble(PUSH_RUMBLE_SMALL, PUSH_RUMBLE_LARGE, PUSH_RUMBLE_SECONDS);
	}
	const float step = dt / DRILL_AIM_SECONDS;
	g_Straighten[hand] = IsThrusting(hand)
		? fminf(1.0f, g_Straighten[hand] + step)
		: fmaxf(0.0f, g_Straighten[hand] - step);
	if (IsThrusting(hand)) g_Spin[hand] = fmodf(g_Spin[hand] + SPIN_SPEED * dt, XM_2PI);
}

static void DrawHand(int hand)
{
	const DRILL& arm = g_Arms[hand];
	const VEC3 origin = ArmGetShoulder(hand);
	const float yaw = PlayerGetYaw();
	const VEC3 forward{ sinf(yaw), 0.0f, cosf(yaw) };
	const VEC3 direction = ArmCalcBlendDirection(
		ArmCalcPitchDirection(forward, ArmGetRestPitch()), arm.Direction, g_Straighten[hand]);
	const XMFLOAT4 color = ArmGetColor(hand, arm.State == DRILL_BORING);
	// 付け根もビットも、常に同じ長さ・倍率で描く。間の円柱や伸縮はない。
	const VEC3 end = Vec3Add(origin, Vec3Scale(direction, BASE_LENGTH * MODEL_SCALE));
	ArmDrawPiece(g_BaseModel[hand], origin, end, BASE_LENGTH, MODEL_SCALE, color);
	const TENTACLE_PIECE_AXES axes = TentacleCalcPieceAxes(origin, end, { 0, -1, 0 });
	const VEC3 x = Vec3Scale(axes.X, (hand == 1 ? -BIT_OFFSET.X : BIT_OFFSET.X) * MODEL_SCALE);
	const VEC3 y = Vec3Scale(axes.Y, BIT_OFFSET.Y * MODEL_SCALE);
	const VEC3 z = Vec3Scale(axes.Z, BIT_OFFSET.Z * MODEL_SCALE);
	const VEC3 bit = Vec3Add(origin, Vec3Add(x, Vec3Add(y, z)));
	const VEC3 tip = Vec3Add(bit, Vec3Scale(direction, BIT_LENGTH * MODEL_SCALE));
	ArmDrawPiece(g_BitModel[hand], bit, tip, BIT_LENGTH, MODEL_SCALE, color, g_Spin[hand]);
}

static bool IsActiveHand(int hand)
{
	return g_Arms[hand].State != DRILL_IDLE;
}

static bool IsHoldingHand(int)
{
	// くい込み中も敵をつかまない。カメラはロックオンを続ける。
	return false;
}

const ARM_FUNCTIONS ARM_DRILL_FUNCTIONS{
	Initialize, Finalize, ResetHand, UpdateHand, DrawHand, IsActiveHand, IsHoldingHand
};
