#include "game/tentacle.h"
#include "game/camera.h"
#include "game/dummy.h"
#include "game/player.h"
#include "game/stage.h"
#include "core/gametime.h"
#include "core/mathutil.h"
#include "game/arm.h"
#include "input/pad.h"
#include "physics/armcalc.h"
#include "render/shape.h"

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
static constexpr float HOLD_OFFSET = 1.0f;
static constexpr float HOLD_FORWARD = 2.0f;
static constexpr float SPHERE_SPACING = 0.25f;
static constexpr float MODEL_SCALE = 0.15f;
static constexpr float BASE_LENGTH = 10.218f; // OBJ のつなぎ目間の長さ（cm）
static constexpr float SEGMENT_LENGTH = 3.207f;
static constexpr float TIP_LENGTH = 4.434f;
static constexpr float MIN_STRETCH = 0.2f;
static constexpr float IDLE_LENGTH = 1.2f; // 待機中・短い伸縮中に描く長さ（判定には使わない）
static constexpr float MODEL_MIN_LENGTH = 0.00001f;
static constexpr float WIGGLE_AMPLITUDE = 0.25f;
static constexpr float WIGGLE_WAVES = 1.5f;
static constexpr float WIGGLE_SPEED = 8.0f;
static constexpr float WIGGLE_ARM_PHASE = 1.6f;
static constexpr float WIGGLE_FULL_LENGTH = 3.0f;
static constexpr float WIGGLE_HOLD_SCALE = 0.3f;

static TENTACLE g_Arms[2];
// モデル番号（[0]：左手、[1]：右手（左手用のモデルを鏡写しにしたもの））
static int g_BaseModel[2] = { -1, -1 };
static int g_SegmentModel[2] = { -1, -1 };
static int g_TipModel[2] = { -1, -1 };

static VEC3 HoldPosition(float side, float forward, float height)
{
	const XMFLOAT3 p = PlayerGetPosition();
	const float yaw = PlayerGetYaw();
	return { p.x + cosf(yaw) * side + sinf(yaw) * forward, height,
		p.z - sinf(yaw) * side + cosf(yaw) * forward };
}

void TentacleInitialize(void)
{
	// ゲームの中ではモデルのままが左手に合うので、右手は X を鏡写しにして読む
	for (int hand = 0; hand < 2; ++hand) {
		g_BaseModel[hand] = ArmLoadModel("assets/models/tentacle_base.obj", hand);
		g_SegmentModel[hand] = ArmLoadModel("assets/models/tentacle_segment.obj", hand);
		g_TipModel[hand] = ArmLoadModel("assets/models/tentacle_tip.obj", hand);
	}
	for (TENTACLE& arm : g_Arms) TentacleCalcReset(arm);
}

void TentacleResetHand(int hand)
{
	TENTACLE& arm = g_Arms[hand];
	DummyRelease(arm.Target, false, {}, 0.0f, 0.0f);
	TentacleCalcReset(arm);
}

void TentacleFinalize(void)
{
	for (int hand = 0; hand < 2; ++hand) TentacleResetHand(hand);
}

void TentacleUpdateHand(int hand, bool trigger)
{
	const float yaw = CameraGetYaw();
	const VEC3 cameraForward{ sinf(yaw), 0.0f, cosf(yaw) };
	TENTACLE& arm = g_Arms[hand];
	// 左腕がつかんだ状態を右腕へ反映するため、候補は各腕の直前に取得する。
	TENTACLE_TARGET targets[DUMMY_COUNT];
	for (int i = 0; i < DUMMY_COUNT; ++i) targets[i] = DummyGetTarget(i);
	const float side = hand == 0 ? -1.0f : 1.0f;
	const VEC3 origin = ArmGetShoulder(hand);
	// 肩と同じ高さ（従来の p.y + HAND_HEIGHT）。XZ の計算順は従来の Position と同じ。
	const VEC3 hold = HoldPosition(side * HOLD_OFFSET, HOLD_FORWARD, origin.Y);
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

void TentacleDrawHand(int hand)
{
	const TENTACLE& arm = g_Arms[hand];
	const VEC3 origin = ArmGetShoulder(hand);
	VEC3 tip = arm.State == TENTACLE_IDLE ? origin : arm.Tip;
	if (arm.State != TENTACLE_HOLDING) {
		// 描画する先端だけを変える。計算上の arm.Tip・長さ・判定はそのまま。
		const VEC3 extended = Vec3Sub(tip, origin);
		const float length = Vec3Length(extended);
		const VEC3 rest = ArmGetRestDirection(hand);
		const VEC3 aim = length < MODEL_MIN_LENGTH ? rest : Vec3Scale(extended, 1.0f / length);
		const VEC3 direction = ArmCalcBlendDirection(rest, aim, length / IDLE_LENGTH);
		tip = Vec3Add(origin, Vec3Scale(direction, fmaxf(length, IDLE_LENGTH)));
	}
	const VEC3 delta = Vec3Sub(tip, origin);
	const int steps = (int)ceilf(Vec3Length(delta) / SPHERE_SPACING) + 1;
	const XMFLOAT4 color = ArmGetColor(hand, arm.Target >= 0);
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
			ArmDrawPiece(base ? g_BaseModel[hand] : (last ? g_TipModel[hand] : g_SegmentModel[hand]),
				start, end, sourceLength, MODEL_SCALE, color);
			start = end;
		}
		return;
	}
	for (int i = 0; i <= steps; ++i) {
		const VEC3 p = Vec3Add(origin, Vec3Scale(delta, (float)i / steps));
		ShapeDrawSphere({ p.X, p.Y, p.Z }, PARAMS.Radius, color);
	}
}

bool TentacleIsHoldingHand(int hand)
{
	return g_Arms[hand].State == TENTACLE_HOLDING;
}

bool TentacleIsActiveHand(int hand)
{
	return g_Arms[hand].State != TENTACLE_IDLE;
}
