#include "game/tentacle.h"
#include "game/camera.h"
#include "game/dummy.h"
#include "game/player.h"
#include "game/stage.h"
#include "core/gametime.h"
#include "core/mathutil.h"
#include "input/action.h"
#include "input/pad.h"
#include "render/shape.h"
#include "render/model.h"

// 調整用の値
static constexpr TENTACLE_PARAMS PARAMS{ 8.0f, 24.0f, 30.0f, 3.0f, 0.18f };
static constexpr float THROW_DAMAGE = 12.0f;
static constexpr float THROW_POWER = 2.0f;
static constexpr float GRAB_RUMBLE_SMALL = 0.4f;
static constexpr float GRAB_RUMBLE_LARGE = 0.0f;
static constexpr float GRAB_RUMBLE_SECONDS = 0.1f;
static constexpr float THROW_RUMBLE_SMALL = 0.6f;
static constexpr float THROW_RUMBLE_LARGE = 0.6f;
static constexpr float THROW_RUMBLE_SECONDS = 0.15f;
static constexpr float EXTEND_RUMBLE_START = 1.0f;	// 腕が伸び始めたときの振動の強さ（0〜1。両方のモーター）
static constexpr float EXTEND_RUMBLE_END = 1.0f;	// 腕が最大まで伸びたときの振動の強さ（0〜1。両方のモーター）
static constexpr float EXTEND_RUMBLE_SECONDS = 0.05f;	// 伸長中に毎フレーム足す振動の長さ（伸び終わるとこの秒数で止まる）
static constexpr float SHOULDER_OFFSET = 0.65f;
static constexpr float HOLD_OFFSET = 1.0f;
static constexpr float HOLD_FORWARD = 2.0f;
static constexpr float HAND_HEIGHT = 1.35f;
static constexpr float SPHERE_SPACING = 0.25f;
static constexpr float MODEL_SCALE = 0.15f;
static constexpr float BASE_LENGTH = 10.218f; // OBJ のつなぎ目間の長さ（cm）
static constexpr float SEGMENT_LENGTH = 3.207f;
static constexpr float TIP_LENGTH = 4.434f;
static constexpr float MIN_STRETCH = 0.2f;
static constexpr float MODEL_MIN_LENGTH = 0.00001f;
static constexpr float WIGGLE_AMPLITUDE = 0.25f;
static constexpr float WIGGLE_WAVES = 1.5f;
static constexpr float WIGGLE_SPEED = 8.0f;
static constexpr float WIGGLE_ARM_PHASE = 1.6f;
static constexpr float WIGGLE_FULL_LENGTH = 3.0f;
static constexpr float WIGGLE_HOLD_SCALE = 0.3f;
static constexpr XMFLOAT4 LEFT_COLOR{ 0.8f, 0.3f, 0.65f, 1.0f };
static constexpr XMFLOAT4 RIGHT_COLOR{ 0.35f, 0.8f, 0.6f, 1.0f };
static constexpr XMFLOAT4 HOLD_COLOR{ 1.0f, 0.7f, 0.2f, 1.0f };

static TENTACLE g_Arms[2];
// モデル番号（[0]：左手、[1]：右手（左手用のモデルを鏡写しにしたもの））
static int g_BaseModel[2] = { -1, -1 };
static int g_SegmentModel[2] = { -1, -1 };
static int g_TipModel[2] = { -1, -1 };

static VEC3 Position(float side, float forward)
{
	const XMFLOAT3 p = PlayerGetPosition();
	const float yaw = PlayerGetYaw();
	return { p.x + cosf(yaw) * side + sinf(yaw) * forward, p.y + HAND_HEIGHT,
		p.z - sinf(yaw) * side + cosf(yaw) * forward };
}

void TentacleInitialize(void)
{
	// ゲームの中ではモデルのままが左手に合うので、右手は X を鏡写しにして読む
	for (int hand = 0; hand < 2; ++hand) {
		const bool mirror = hand == 1;
		g_BaseModel[hand] = ModelLoad("assets/models/tentacle_base.obj", mirror);
		g_SegmentModel[hand] = ModelLoad("assets/models/tentacle_segment.obj", mirror);
		g_TipModel[hand] = ModelLoad("assets/models/tentacle_tip.obj", mirror);
	}
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
	const VEC3 cameraForward{ sinf(yaw), 0.0f, cosf(yaw) };
	for (int hand = 0; hand < 2; ++hand) {
		TENTACLE& arm = g_Arms[hand];
		// 左腕がつかんだ状態を右腕へ反映するため、候補は各腕の直前に取得する。
		TENTACLE_TARGET targets[DUMMY_COUNT];
		for (int i = 0; i < DUMMY_COUNT; ++i) targets[i] = DummyGetTarget(i);
		const float side = hand == 0 ? -1.0f : 1.0f;
		const VEC3 origin = Position(side * SHOULDER_OFFSET, 0.0f);
		const VEC3 hold = Position(side * HOLD_OFFSET, HOLD_FORWARD);
		const bool trigger = ActionIsTrigger(hand == 0 ? ACTION_ARM_LEFT : ACTION_ARM_RIGHT);
		VEC3 aim = cameraForward;
		if (trigger && arm.State == TENTACLE_IDLE) {
			const int target = CameraGetTarget();
			const bool hasTarget = target >= 0 && target < DUMMY_COUNT;
			const VEC3 center = hasTarget
				? Vec3Scale(Vec3Add(targets[target].Body.Start, targets[target].Body.End), 0.5f) : VEC3{};
			// 各肩からの向きを開始時だけ決める。伸長中は追いかけず、投げる向きとも分ける。
			aim = TentacleCalcAim(origin, center, hasTarget, cameraForward);
		}
		const bool cancel = PlayerDidRespawn() || (arm.Target >= 0 && !DummyIsActive(arm.Target));
		const TENTACLE_EVENT event = TentacleCalcStep(arm, origin, aim, hold,
			trigger, cancel,
			GameTimeGetDeltaTime(), PARAMS, targets, DUMMY_COUNT, StageGet());
		if (event.Grabbed >= 0) {
			if (DummyGrab(event.Grabbed)) PadRumble(GRAB_RUMBLE_SMALL, GRAB_RUMBLE_LARGE, GRAB_RUMBLE_SECONDS);
			else TentacleCalcReset(arm);
		}
		if (event.Released >= 0) {
			DummyRelease(event.Released, event.Throw, cameraForward, THROW_DAMAGE, THROW_POWER);
			if (event.Throw) PadRumble(THROW_RUMBLE_SMALL, THROW_RUMBLE_LARGE, THROW_RUMBLE_SECONDS);
		}
		if (arm.Target >= 0) DummyMoveHeld(arm.Target, arm.Tip);
		if (arm.State == TENTACLE_EXTENDING) {
			// 伸びるほど強くする（PadRumble は強いほうを残すので、毎フレーム足すと右肩上がりになる）
			const float strength = Lerp(EXTEND_RUMBLE_START, EXTEND_RUMBLE_END, TentacleCalcExtendRatio(arm, PARAMS));
			PadRumble(strength, strength, EXTEND_RUMBLE_SECONDS);
		}
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
		if (g_BaseModel[hand] >= 0 && g_SegmentModel[hand] >= 0 && g_TipModel[hand] >= 0) {
			const float length = Vec3Length(delta);
			const float remaining = length / MODEL_SCALE - BASE_LENGTH - TIP_LENGTH;
			const int segments = remaining > 0.0f ? (int)floorf(remaining / SEGMENT_LENGTH + 0.5f) : 0;
			const float total = BASE_LENGTH + segments * SEGMENT_LENGTH + TIP_LENGTH;
			// 短すぎる腕は見た目だけ最低長まで伸ばす。判定や arm.Tip は変えない。
			const float drawLength = fmaxf(length, total * MODEL_SCALE * MIN_STRETCH);
			const float yaw = PlayerGetYaw();
			const VEC3 direction = length < MODEL_MIN_LENGTH ? VEC3{ sinf(yaw), 0, cosf(yaw) }
				: Vec3Scale(delta, 1.0f / length);
			const VEC3 drawTip = Vec3Add(origin, Vec3Scale(direction, drawLength));
			const float phase = GameTimeGetElapsed() * WIGGLE_SPEED + hand * WIGGLE_ARM_PHASE;
			const float amplitude = WIGGLE_AMPLITUDE * fminf(1.0f, length / WIGGLE_FULL_LENGTH)
				* (arm.State == TENTACLE_HOLDING ? WIGGLE_HOLD_SCALE : 1.0f);
			float distance = 0.0f;
			VEC3 start = origin;
			for (int piece = 0; piece < segments + 2; ++piece) {
				const bool base = piece == 0;
				const bool last = piece == segments + 1;
				const float sourceLength = base ? BASE_LENGTH : (last ? TIP_LENGTH : SEGMENT_LENGTH);
				distance += sourceLength;
				const VEC3 end = TentacleCalcWigglePoint(origin, drawTip, last ? 1.0f : distance / total,
					phase, amplitude, WIGGLE_WAVES);
				const TENTACLE_PIECE_AXES axes = TentacleCalcPieceAxes(start, end, { 0, -1, 0 });
				const VEC3 x = Vec3Scale(axes.X, MODEL_SCALE);
				const VEC3 y = Vec3Scale(axes.Y, Vec3Length(Vec3Sub(end, start)) / sourceLength);
				const VEC3 z = Vec3Scale(axes.Z, MODEL_SCALE);
				ModelDraw(base ? g_BaseModel[hand] : (last ? g_TipModel[hand] : g_SegmentModel[hand]),
					{ start.X, start.Y, start.Z }, { x.X, x.Y, x.Z }, { y.X, y.Y, y.Z }, { z.X, z.Y, z.Z }, color);
				start = end;
			}
			continue;
		}
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

bool TentacleIsActive(void)
{
	for (const TENTACLE& arm : g_Arms) {
		if (arm.State != TENTACLE_IDLE) return true;
	}
	return false;
}
