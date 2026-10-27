#include "game/arm.h"
#include "game/camera.h"
#include "game/dummy.h"
#include "game/player.h"
#include "game/stage.h"
#include "core/gametime.h"
#include "input/pad.h"
#include "physics/armcalc.h"
#include "physics/grapplecalc.h"
#include "render/model.h"
#include "render/shape.h"

// 調整用の値
static constexpr GRAPPLE_PARAMS PARAMS{ 40.0f, 16.0f, 0.3f, 20.0f, 1.2f, 0.8f, 1.5f };
static constexpr float HIT_DAMAGE = 5.0f;
static constexpr float HIT_POWER = 1.0f;
static constexpr float LANDING_INSET = 0.8f;
static constexpr float MODEL_SCALE = 0.08f;
static constexpr float WIRE_RADIUS = 0.03f;
static constexpr float GRAPPLE_ROLL = XMConvertToRadians(90.0f);
static constexpr bool GRAPPLE_MIRROR_LEFT = true;
static constexpr float UPPER_LENGTH = 11.352817f;
static constexpr float LOWER_LENGTH = 11.820275f;
static constexpr float HOOK_LENGTH = 6.11f;
// Maya の肩→ひじ→各取付点（cm）。Z は OBJ 読み込み後の向き。
static constexpr VEC3 UPPER_ELBOW_OFFSET{ -1.614f, -10.900f, 0.241f };
static constexpr VEC3 ELBOW_WIRE_OFFSET{ 0.0f, -11.820f, 0.0f };
static constexpr VEC3 ELBOW_HOOK_OFFSET{ 0.0f, -14.222f, 0.0f };
static constexpr float GRAPPLE_IDLE_ELBOW = XMConvertToRadians(45.0f);
static constexpr float GRAPPLE_STRAIGHTEN_SECONDS = 0.1f;
// 触腕の「つかむ」「投げる」と同じ強さ・秒数。
static constexpr float ATTACH_RUMBLE_SMALL = 0.4f;
static constexpr float ATTACH_RUMBLE_LARGE = 0.0f;
static constexpr float ATTACH_RUMBLE_SECONDS = 0.1f;
static constexpr float HIT_RUMBLE_SMALL = 0.6f;
static constexpr float HIT_RUMBLE_LARGE = 0.6f;
static constexpr float HIT_RUMBLE_SECONDS = 0.15f;

static GrappleCalc g_Arms[2];
static float g_Straighten[2] = {}; // 描画だけのまっすぐ度（0：待機の構え、1：フックの向き）。
static int g_UpperModel[2] = { -1, -1 };
static int g_LowerModel[2] = { -1, -1 };
static int g_HookModel[2] = { -1, -1 };
static int g_PullingHand = -1;

static bool MirrorHand(int hand)
{
	return (hand == 0) == GRAPPLE_MIRROR_LEFT;
}

static void ResetHand(int hand)
{
	g_Arms[hand].Reset();
	g_Straighten[hand] = 0.0f;
	if (g_PullingHand == hand) {
		g_PullingHand = -1;
		PlayerSetPulled(false);
	}
}

static void Initialize(void)
{
	g_PullingHand = -1;
	for (int hand = 0; hand < 2; ++hand) {
		g_Arms[hand].Reset();
		g_Straighten[hand] = 0.0f;
		g_UpperModel[hand] = ModelLoad("assets/models/arm_grapple_upper.obj", MirrorHand(hand));
		g_LowerModel[hand] = ModelLoad("assets/models/arm_grapple_lower.obj", MirrorHand(hand));
		g_HookModel[hand] = ModelLoad("assets/models/arm_grapple_hook.obj", MirrorHand(hand));
	}
}

static void Finalize(void)
{
	for (int hand = 0; hand < 2; ++hand) ResetHand(hand);
}

static void UpdateHand(int hand, bool trigger)
{
	GrappleCalc& arm = g_Arms[hand];
	const VEC3 origin = ArmGetShoulder(hand);
	const float yaw = CameraGetYaw();
	const VEC3 forward{ sinf(yaw), 0.0f, cosf(yaw) };
	TENTACLE_TARGET targets[DUMMY_COUNT];
	for (int i = 0; i < DUMMY_COUNT; ++i) targets[i] = DummyGetTarget(i);
	const int target = CameraGetTarget();
	const bool hasTarget = target >= 0 && target < DUMMY_COUNT;
	const VEC3 targetCenter = hasTarget
		? Vec3Scale(Vec3Add(targets[target].Body.Start, targets[target].Body.End), 0.5f) : VEC3{};
	const VEC3 aim = GrappleCalcAim(origin, targetCenter, hasTarget, forward);
	const XMFLOAT3 position = PlayerGetPosition();
	const float halfHeight = PlayerGetHeight() * 0.5f;
	VEC3 center{ position.x, position.y + halfHeight, position.z };
	const GRAPPLE_EVENT event = arm.Step(origin, aim, center,
		trigger || (arm.GetTarget() >= 0 && !DummyIsActive(arm.GetTarget())), PlayerDidRespawn(),
		GameTimeGetDeltaTime(), PARAMS, targets, DUMMY_COUNT, StageGet());
	if (event.Attached) {
		// 新しく刺さった手へ所有権を移す。先の手は空のフックを回収する。
		if (g_PullingHand >= 0 && g_PullingHand != hand) {
			VEC3 unused = center;
			g_Arms[g_PullingHand].Step(ArmGetShoulder(g_PullingHand), {}, unused,
				true, false, GameTimeGetDeltaTime(), PARAMS, targets, DUMMY_COUNT, StageGet());
		}
		g_PullingHand = hand;
		PlayerSetPulled(true);
		PadRumble(ATTACH_RUMBLE_SMALL, ATTACH_RUMBLE_LARGE, ATTACH_RUMBLE_SECONDS);
	}
	if (g_PullingHand == hand) {
		if (event.Arrived && event.OnStage) {
			const VEC3 landing = GrappleCalcLanding(arm.GetAnchor(), StageGet(), LANDING_INSET);
			PlayerSetPosition({ landing.X, landing.Y, landing.Z });
		}
		else {
			PlayerSetPosition({ center.X, center.Y - halfHeight, center.Z });
		}
		if (event.Arrived && event.Target >= 0) {
			DummyHit(event.Target, { event.Direction.X, event.Direction.Y, event.Direction.Z }, HIT_DAMAGE, HIT_POWER);
			PadRumble(HIT_RUMBLE_SMALL, HIT_RUMBLE_LARGE, HIT_RUMBLE_SECONDS);
		}
		if (event.Released) {
			g_PullingHand = -1;
			PlayerSetPulled(false);
		}
	}
	// 動きの状態には触れず、描画の構えだけを一定の速さで変える。
	const float step = fmaxf(0.0f, GameTimeGetDeltaTime()) / GRAPPLE_STRAIGHTEN_SECONDS;
	g_Straighten[hand] = arm.GetState() == GRAPPLE_IDLE
		? fmaxf(0.0f, g_Straighten[hand] - step)
		: fminf(1.0f, g_Straighten[hand] + step);
}

static void DrawHand(int hand)
{
	const GrappleCalc& arm = g_Arms[hand];
	const VEC3 origin = ArmGetShoulder(hand);
	const float yaw = PlayerGetYaw();
	const VEC3 forward{ sinf(yaw), 0.0f, cosf(yaw) };
	// 距離によってまぜ方が変わらないよう、フックへの向きも先に長さ 1 にする。
	const VEC3 hookDirection = arm.GetState() == GRAPPLE_IDLE ? forward
		: ArmCalcBlendDirection(forward, Vec3Sub(arm.GetHook(), origin), 1.0f);
	const VEC3 upperDirection = ArmCalcBlendDirection(
		ArmGetRestDirection(hand), hookDirection, g_Straighten[hand]);
	const VEC3 lowerDirection = ArmCalcBlendDirection(
		ArmGetRestDirection(hand, GRAPPLE_IDLE_ELBOW), hookDirection, g_Straighten[hand]);
	const bool mirror = MirrorHand(hand);
	const float roll = mirror ? -GRAPPLE_ROLL : GRAPPLE_ROLL;
	const XMFLOAT4 color = ArmGetColor(hand, arm.GetState() == GRAPPLE_PULLING);
	// 各部品は伸縮させず、作者が置いたひじをつないで描く。
	const VEC3 upperFacing = Vec3Add(origin, upperDirection);
	const VEC3 upperEnd = GrappleCalcModelPoint({ 0, -UPPER_LENGTH, 0 }, origin, upperFacing, MODEL_SCALE, false, roll);
	ArmDrawPiece(g_UpperModel[hand], origin, upperEnd, UPPER_LENGTH, MODEL_SCALE, color, roll);
	const VEC3 elbow = GrappleCalcModelPoint(UPPER_ELBOW_OFFSET, origin, upperFacing, MODEL_SCALE, mirror, roll);
	const VEC3 lowerFacing = Vec3Add(elbow, lowerDirection);
	const VEC3 lowerEnd = GrappleCalcModelPoint({ 0, -LOWER_LENGTH, 0 }, elbow, lowerFacing, MODEL_SCALE, false, roll);
	ArmDrawPiece(g_LowerModel[hand], elbow, lowerEnd, LOWER_LENGTH, MODEL_SCALE, color, roll);
	const VEC3 wire = GrappleCalcModelPoint(ELBOW_WIRE_OFFSET, elbow, lowerFacing, MODEL_SCALE, mirror, roll);
	const VEC3 hook = arm.GetState() == GRAPPLE_IDLE
		? GrappleCalcModelPoint(ELBOW_HOOK_OFFSET, elbow, lowerFacing, MODEL_SCALE, mirror, roll) : arm.GetHook();
	// フックは前腕と同じ向き・ひねりで置く。
	const VEC3 hookEnd = Vec3Add(hook, Vec3Scale(lowerDirection, HOOK_LENGTH * MODEL_SCALE));
	ArmDrawPiece(g_HookModel[hand], hook, hookEnd, HOOK_LENGTH, MODEL_SCALE, color, roll);
	ShapeDrawCylinder({ wire.X, wire.Y, wire.Z }, { hook.X, hook.Y, hook.Z }, WIRE_RADIUS, color);
}

static bool IsActiveHand(int hand)
{
	return g_Arms[hand].GetState() != GRAPPLE_IDLE;
}

static bool IsHoldingHand(int)
{
	// グラップリングは敵の保持状態を取らない（カメラのロックオンを続ける）。
	return false;
}

const ARM_FUNCTIONS ARM_GRAPPLE_FUNCTIONS{
	Initialize, Finalize, ResetHand, UpdateHand, DrawHand, IsActiveHand, IsHoldingHand
};
