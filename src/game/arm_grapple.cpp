#include "game/arm.h"
#include "game/camera.h"
#include "game/dummy.h"
#include "game/player.h"
#include "game/stage.h"
#include "core/gametime.h"
#include "input/pad.h"
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
static constexpr float BASE_LENGTH = 22.72f;
static constexpr float HOOK_LENGTH = 6.11f;
// No.34 の原点の差。Maya の Z = -0.241 は OBJ 読み込み後の +0.241 に直して使う。
static constexpr VEC3 HOOK_OFFSET{ -1.614f, -25.122f, 0.241f };
static constexpr VEC3 WIRE_OFFSET{ -1.614f, -22.720f, 0.241f };
// 触腕の「つかむ」「投げる」と同じ強さ・秒数。
static constexpr float ATTACH_RUMBLE_SMALL = 0.4f;
static constexpr float ATTACH_RUMBLE_LARGE = 0.0f;
static constexpr float ATTACH_RUMBLE_SECONDS = 0.1f;
static constexpr float HIT_RUMBLE_SMALL = 0.6f;
static constexpr float HIT_RUMBLE_LARGE = 0.6f;
static constexpr float HIT_RUMBLE_SECONDS = 0.15f;

static GRAPPLE g_Arms[2];
static int g_BaseModel[2] = { -1, -1 };
static int g_HookModel[2] = { -1, -1 };
static int g_PullingHand = -1;

static bool MirrorHand(int hand)
{
	return (hand == 0) == GRAPPLE_MIRROR_LEFT;
}

static void ResetHand(int hand)
{
	GrappleCalcReset(g_Arms[hand]);
	if (g_PullingHand == hand) {
		g_PullingHand = -1;
		PlayerSetPulled(false);
	}
}

static void Initialize(void)
{
	g_PullingHand = -1;
	for (int hand = 0; hand < 2; ++hand) {
		GrappleCalcReset(g_Arms[hand]);
		g_BaseModel[hand] = ModelLoad("assets/models/arm_grapple_base.obj", MirrorHand(hand));
		g_HookModel[hand] = ModelLoad("assets/models/arm_grapple_hook.obj", MirrorHand(hand));
	}
}

static void Finalize(void)
{
	for (int hand = 0; hand < 2; ++hand) ResetHand(hand);
}

static void UpdateHand(int hand, bool trigger)
{
	GRAPPLE& arm = g_Arms[hand];
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
	const GRAPPLE_EVENT event = GrappleCalcStep(arm, origin, aim, center,
		trigger || (arm.Target >= 0 && !DummyIsActive(arm.Target)), PlayerDidRespawn(),
		GameTimeGetDeltaTime(), PARAMS, targets, DUMMY_COUNT, StageGet());
	if (event.Attached) {
		// 新しく刺さった手へ所有権を移す。先の手は空のフックを回収する。
		if (g_PullingHand >= 0 && g_PullingHand != hand) {
			VEC3 unused = center;
			GrappleCalcStep(g_Arms[g_PullingHand], ArmGetShoulder(g_PullingHand), {}, unused,
				true, false, GameTimeGetDeltaTime(), PARAMS, targets, DUMMY_COUNT, StageGet());
		}
		g_PullingHand = hand;
		PlayerSetPulled(true);
		PadRumble(ATTACH_RUMBLE_SMALL, ATTACH_RUMBLE_LARGE, ATTACH_RUMBLE_SECONDS);
	}
	if (g_PullingHand == hand) {
		if (event.Arrived && event.OnStage) {
			const VEC3 landing = GrappleCalcLanding(arm.Anchor, StageGet(), LANDING_INSET);
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
}

static void DrawHand(int hand)
{
	const GRAPPLE& arm = g_Arms[hand];
	const VEC3 origin = ArmGetShoulder(hand);
	const float yaw = PlayerGetYaw();
	const VEC3 forward{ sinf(yaw), 0.0f, cosf(yaw) };
	const VEC3 facing = arm.State == GRAPPLE_IDLE ? Vec3Add(origin, forward) : arm.Hook;
	const bool mirror = MirrorHand(hand);
	const float roll = mirror ? -GRAPPLE_ROLL : GRAPPLE_ROLL;
	const XMFLOAT4 color = ArmGetColor(hand, arm.State == GRAPPLE_PULLING);
	// 付け根は伸縮させず、フックへの向きだけを使う。
	const VEC3 baseEnd = GrappleCalcModelPoint({ 0, -BASE_LENGTH, 0 }, origin, facing, MODEL_SCALE, false, roll);
	ArmDrawPiece(g_BaseModel[hand], origin, baseEnd, BASE_LENGTH, MODEL_SCALE, color, roll);
	const VEC3 wire = GrappleCalcModelPoint(WIRE_OFFSET, origin, facing, MODEL_SCALE, mirror, roll);
	const VEC3 hook = arm.State == GRAPPLE_IDLE
		? GrappleCalcModelPoint(HOOK_OFFSET, origin, facing, MODEL_SCALE, mirror, roll) : arm.Hook;
	// フックも付け根と同じ進む向き・ひねりで置く。
	const VEC3 hookEnd = Vec3Add(hook, Vec3Scale(Vec3Sub(baseEnd, origin), HOOK_LENGTH / BASE_LENGTH));
	ArmDrawPiece(g_HookModel[hand], hook, hookEnd, HOOK_LENGTH, MODEL_SCALE, color, roll);
	ShapeDrawCylinder({ wire.X, wire.Y, wire.Z }, { hook.X, hook.Y, hook.Z }, WIRE_RADIUS, color);
}

static bool IsActiveHand(int hand)
{
	return g_Arms[hand].State != GRAPPLE_IDLE;
}

static bool IsHoldingHand(int)
{
	// グラップリングは敵の保持状態を取らない（カメラのロックオンを続ける）。
	return false;
}

const ARM_FUNCTIONS ARM_GRAPPLE_FUNCTIONS{
	Initialize, Finalize, ResetHand, UpdateHand, DrawHand, IsActiveHand, IsHoldingHand
};
