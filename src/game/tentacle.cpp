#include "game/tentacle.h"
#include "game/camera.h"
#include "game/dummy.h"
#include "game/player.h"
#include "game/stage.h"
#include "core/gametime.h"
#include "input/action.h"
#include "physics/slapcalc.h"
#include "render/shape.h"

// 調整用の値
static constexpr TENTACLE_PARAMS PARAMS{ 8.0f, 24.0f, 30.0f, 3.0f, 0.18f };
static constexpr float THROW_DAMAGE = 12.0f;
static constexpr float THROW_POWER = 2.0f;
static constexpr SLAP_PARAMS SLAP_SETTINGS{ 3.0f, 0.3f, XMConvertToRadians(60.0f), 0.2f, 0.3f };
static constexpr float SLAP_DAMAGE = 6.0f;
static constexpr float SLAP_POWER = 1.0f;
static constexpr float SHOULDER_OFFSET = 0.65f;
static constexpr float HOLD_OFFSET = 1.0f;
static constexpr float HOLD_FORWARD = 2.0f;
static constexpr float HAND_HEIGHT = 1.35f;
static constexpr float SPHERE_SPACING = 0.25f;
static constexpr XMFLOAT4 LEFT_COLOR{ 0.8f, 0.3f, 0.65f, 1.0f };
static constexpr XMFLOAT4 RIGHT_COLOR{ 0.35f, 0.8f, 0.6f, 1.0f };
static constexpr XMFLOAT4 HOLD_COLOR{ 1.0f, 0.7f, 0.2f, 1.0f };

static TENTACLE g_Arms[2];
static SLAP g_Slaps[2];

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
	for (SLAP& slap : g_Slaps) SlapCalcReset(slap);
}

void TentacleReset(void)
{
	for (SLAP& slap : g_Slaps) SlapCalcReset(slap);
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
	const XMFLOAT3 direction = CameraGetDirection();
	const VEC3 aim{ direction.x, direction.y, direction.z };
	const float dt = GameTimeGetDeltaTime();
	const VEC3 horizontal{ sinf(CameraGetYaw()), 0.0f, cosf(CameraGetYaw()) };
	for (SLAP& slap : g_Slaps) {
		if (PlayerDidRespawn()) SlapCalcReset(slap);
		else SlapCalcStep(slap, dt, SLAP_SETTINGS);
	}
	if (dt > 0.0f && !PlayerDidRespawn() && ActionIsTrigger(ACTION_SLAP)) {
		// 右腕を優先し、待ち時間も含めて使えないときだけ左腕を選ぶ。
		for (int hand = 1; hand >= 0; --hand) {
			if (g_Arms[hand].State == TENTACLE_IDLE && SlapCalcStart(g_Slaps[hand])) break;
		}
	}
	for (int hand = 0; hand < 2; ++hand) {
		TENTACLE& arm = g_Arms[hand];
		SLAP& slap = g_Slaps[hand];
		// 左腕がつかんだ状態を右腕へ反映するため、候補は各腕の直前に取得する。
		TENTACLE_TARGET targets[DUMMY_COUNT];
		for (int i = 0; i < DUMMY_COUNT; ++i) targets[i] = DummyGetTarget(i);
		const float side = hand == 0 ? -1.0f : 1.0f;
		const VEC3 origin = Position(side * SHOULDER_OFFSET, 0.0f);
		const VEC3 hold = Position(side * HOLD_OFFSET, HOLD_FORWARD);
		const bool cancel = PlayerDidRespawn() || (arm.Target >= 0 && !DummyIsActive(arm.Target));
		const TENTACLE_EVENT event = TentacleCalcStep(arm, origin, aim, hold,
			SlapCalcCanStart(slap) && ActionIsTrigger(hand == 0 ? ACTION_ARM_LEFT : ACTION_ARM_RIGHT), cancel,
			GameTimeGetDeltaTime(), PARAMS, targets, DUMMY_COUNT, StageGet());
		if (event.Grabbed >= 0 && !DummyGrab(event.Grabbed)) TentacleCalcReset(arm);
		if (event.Released >= 0) DummyRelease(event.Released, event.Throw, aim, THROW_DAMAGE, THROW_POWER);
		if (arm.Target >= 0) DummyMoveHeld(arm.Target, arm.Tip);
		if (slap.Active && dt > 0.0f) {
			const VEC3 tip = Vec3Add(origin, Vec3Scale(SlapCalcDirection(slap, horizontal, side, SLAP_SETTINGS), SLAP_SETTINGS.Length));
			const CAPSULE reach{ origin, tip, SLAP_SETTINGS.Radius };
			int hit;
			while ((hit = SlapCalcHit(slap, reach, targets, DUMMY_COUNT)) >= 0) {
				DummyHit(hit, { horizontal.X, 0.0f, horizontal.Z }, SLAP_DAMAGE, SLAP_POWER);
			}
		}
	}
}

void TentacleDraw(void)
{
	for (int hand = 0; hand < 2; ++hand) {
		const TENTACLE& arm = g_Arms[hand];
		const VEC3 origin = Position(hand == 0 ? -SHOULDER_OFFSET : SHOULDER_OFFSET, 0.0f);
		const SLAP& slap = g_Slaps[hand];
		const VEC3 horizontal{ sinf(CameraGetYaw()), 0.0f, cosf(CameraGetYaw()) };
		const VEC3 tip = slap.Active
			? Vec3Add(origin, Vec3Scale(SlapCalcDirection(slap, horizontal, hand == 0 ? -1.0f : 1.0f, SLAP_SETTINGS), SLAP_SETTINGS.Length))
			: (arm.State == TENTACLE_IDLE ? origin : arm.Tip);
		const VEC3 delta = Vec3Sub(tip, origin);
		const int steps = (int)ceilf(Vec3Length(delta) / SPHERE_SPACING) + 1;
		const XMFLOAT4 color = arm.Target >= 0 ? HOLD_COLOR : (hand == 0 ? LEFT_COLOR : RIGHT_COLOR);
		for (int i = 0; i <= steps; ++i) {
			const VEC3 p = Vec3Add(origin, Vec3Scale(delta, (float)i / steps));
			ShapeDrawSphere({ p.X, p.Y, p.Z }, slap.Active ? SLAP_SETTINGS.Radius : PARAMS.Radius, color);
		}
	}
}
