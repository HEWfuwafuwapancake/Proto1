#include "game/tentacle.h"
#include "game/camera.h"
#include "game/dummy.h"
#include "game/player.h"
#include "game/stage.h"
#include "core/gametime.h"
#include "input/action.h"
#include "render/shape.h"

// 調整用の値
static constexpr TENTACLE_PARAMS PARAMS{ 8.0f, 24.0f, 30.0f, 3.0f, 0.18f };
static constexpr float THROW_DAMAGE = 12.0f;
static constexpr float THROW_POWER = 2.0f;
static constexpr float SHOULDER_OFFSET = 0.65f;
static constexpr float HOLD_OFFSET = 1.0f;
static constexpr float HOLD_FORWARD = 2.0f;
static constexpr float HAND_HEIGHT = 1.35f;
static constexpr float SPHERE_SPACING = 0.25f;
static constexpr XMFLOAT4 LEFT_COLOR{ 0.8f, 0.3f, 0.65f, 1.0f };
static constexpr XMFLOAT4 RIGHT_COLOR{ 0.35f, 0.8f, 0.6f, 1.0f };
static constexpr XMFLOAT4 HOLD_COLOR{ 1.0f, 0.7f, 0.2f, 1.0f };

static TENTACLE g_Arms[2];

static VEC3 Position(float side, float forward)
{
	const XMFLOAT3 p = PlayerGetPosition();
	const float yaw = CameraGetYaw();
	return { p.x + cosf(yaw) * side + sinf(yaw) * forward, p.y + HAND_HEIGHT,
		p.z - sinf(yaw) * side + cosf(yaw) * forward };
}

void TentacleInitialize(void)
{
	for (TENTACLE& arm : g_Arms) TentacleCalcReset(arm);
}

void TentacleReset(void)
{
	for (TENTACLE& arm : g_Arms) {
		DummyRelease(arm.Target, false, {}, 0.0f, 0.0f);
		TentacleCalcReset(arm);
	}
}

void TentacleFinalize(void)
{
	TentacleReset();
}

void TentacleUpdate(void)
{
	const float yaw = CameraGetYaw();
	const VEC3 aim{ sinf(yaw), 0.0f, cosf(yaw) };
	for (int hand = 0; hand < 2; ++hand) {
		TENTACLE& arm = g_Arms[hand];
		// 左腕がつかんだ状態を右腕へ反映するため、候補は各腕の直前に取得する。
		TENTACLE_TARGET targets[DUMMY_COUNT];
		for (int i = 0; i < DUMMY_COUNT; ++i) targets[i] = DummyGetTarget(i);
		const float side = hand == 0 ? -1.0f : 1.0f;
		const VEC3 origin = Position(side * SHOULDER_OFFSET, 0.0f);
		const VEC3 hold = Position(side * HOLD_OFFSET, HOLD_FORWARD);
		const bool cancel = PlayerDidRespawn() || (arm.Target >= 0 && !DummyIsActive(arm.Target));
		const TENTACLE_EVENT event = TentacleCalcStep(arm, origin, aim, hold,
			ActionIsTrigger(hand == 0 ? ACTION_ARM_LEFT : ACTION_ARM_RIGHT), cancel,
			GameTimeGetDeltaTime(), PARAMS, targets, DUMMY_COUNT, StageGet());
		if (event.Grabbed >= 0 && !DummyGrab(event.Grabbed)) TentacleCalcReset(arm);
		if (event.Released >= 0) DummyRelease(event.Released, event.Throw, aim, THROW_DAMAGE, THROW_POWER);
		if (arm.Target >= 0) DummyMoveHeld(arm.Target, arm.Tip);
	}
}

void TentacleDraw(void)
{
	for (int hand = 0; hand < 2; ++hand) {
		const TENTACLE& arm = g_Arms[hand];
		const VEC3 origin = Position(hand == 0 ? -SHOULDER_OFFSET : SHOULDER_OFFSET, 0.0f);
		const VEC3 tip = arm.State == TENTACLE_IDLE ? origin : arm.Tip;
		const VEC3 delta = Vec3Sub(tip, origin);
		const int steps = (int)ceilf(Vec3Length(delta) / SPHERE_SPACING) + 1;
		const XMFLOAT4 color = arm.Target >= 0 ? HOLD_COLOR : (hand == 0 ? LEFT_COLOR : RIGHT_COLOR);
		for (int i = 0; i <= steps; ++i) {
			const VEC3 p = Vec3Add(origin, Vec3Scale(delta, (float)i / steps));
			ShapeDrawSphere({ p.X, p.Y, p.Z }, PARAMS.Radius, color);
		}
	}
}

bool TentacleIsHolding(void)
{
	for (const TENTACLE& arm : g_Arms) {
		if (arm.State == TENTACLE_HOLDING) return true;
	}
	return false;
}
