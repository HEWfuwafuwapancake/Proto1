#include "game/tentacle.h"
#include "game/camera.h"
#include "game/fightermanager.h"
#include "game/fighter.h"
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
static constexpr float MODEL_SCALE = 0.15f * Arm::MODEL_SIZE;
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

VEC3 TentacleArm::HoldPosition(float side, float forward, float height) const
{
	const XMFLOAT3 p = GetPlayer().GetPosition();
	const float yaw = GetPlayer().GetYaw();
	return { p.x + cosf(yaw) * side + sinf(yaw) * forward, height,
		p.z - sinf(yaw) * side + cosf(yaw) * forward };
}

TentacleArm::TentacleArm(int hand, const ARM_LINKS& links) : Arm(hand, links)
{
	// ゲームの中ではモデルのままが左手に合うので、右手は X を鏡写しにして読む
	m_BaseModel = LoadModel("assets/models/tentacle_base.obj");
	m_SegmentModel = LoadModel("assets/models/tentacle_segment.obj");
	m_TipModel = LoadModel("assets/models/tentacle_tip.obj");
	m_Arm.Reset();
}

void TentacleArm::Reset()
{
	TentacleCalc& arm = m_Arm;
	GetFighters().Release(arm.GetTarget(), GetSlot(), false, {}, 0.0f, 0.0f);
	arm.Reset();
}

TentacleArm::~TentacleArm()
{
	Reset();
}

void TentacleArm::Update(bool trigger)
{
	const int hand = GetHand();
	const float yaw = GetCamera().GetYaw();
	const VEC3 cameraForward{ sinf(yaw), 0.0f, cosf(yaw) };
	TentacleCalc& arm = m_Arm;
	// 左腕がつかんだ状態を右腕へ反映するため、候補は各腕の直前に取得する。
	TENTACLE_TARGET targets[FIGHTER_COUNT];
	for (int i = 0; i < FIGHTER_COUNT; ++i) targets[i] = GetFighters().GetTarget(i, GetSlot());
	const float side = hand == 0 ? -1.0f : 1.0f;
	const VEC3 origin = GetShoulder();
	// 肩と同じ高さ（従来の p.y + HAND_HEIGHT）。XZ の計算順は従来の Position と同じ。
	const VEC3 hold = HoldPosition(side * HOLD_OFFSET, HOLD_FORWARD, origin.Y);
	VEC3 aim = cameraForward;
	if (trigger && arm.GetState() == TENTACLE_IDLE) {
		const int target = GetCamera().GetTarget();
		const bool hasTarget = target >= 0 && target < FIGHTER_COUNT;
		const VEC3 center = hasTarget
			? Vec3Scale(Vec3Add(targets[target].Body.Start, targets[target].Body.End), 0.5f) : VEC3{};
		// 各肩からの向きを開始時だけ決める。伸長中は追いかけず、投げる向きとも分ける。
		aim = TentacleCalcAim(origin, center, hasTarget, cameraForward);
	}
	const bool cancel = GetPlayer().DidRespawn() || (arm.GetTarget() >= 0 && !GetFighters().IsHeldBy(arm.GetTarget(), GetSlot()));
	const TENTACLE_EVENT event = arm.Step(origin, aim, hold,
		trigger, cancel,
		GameTimeGetDeltaTime(), PARAMS, targets, FIGHTER_COUNT, GetStage().GetStage());
	if (event.Grabbed >= 0) {
		if (GetFighters().Grab(event.Grabbed, GetSlot())) Rumble(GRAB_RUMBLE_SMALL, GRAB_RUMBLE_LARGE, GRAB_RUMBLE_SECONDS);
		else arm.Reset();
	}
	if (event.Released >= 0) {
		GetFighters().Release(event.Released, GetSlot(), event.Throw, cameraForward, THROW_DAMAGE, THROW_POWER);
		if (event.Throw) Rumble(THROW_RUMBLE_SMALL, THROW_RUMBLE_LARGE, THROW_RUMBLE_SECONDS);
	}
	if (arm.GetTarget() >= 0) GetFighters().MoveHeld(arm.GetTarget(), GetSlot(), arm.GetTip());
	if (arm.GetState() == TENTACLE_EXTENDING) {
		// 伸びるほど強くする（PadRumble は強いほうを残すので、毎フレーム足すと右肩上がりになる）
		const float strength = Lerp(EXTEND_RUMBLE_START, EXTEND_RUMBLE_END, arm.GetExtendRatio(PARAMS));
		Rumble(strength, strength, EXTEND_RUMBLE_SECONDS);
	}
}

void TentacleArm::Draw()
{
	const int hand = GetHand();
	const TentacleCalc& arm = m_Arm;
	const VEC3 origin = GetShoulder();
	VEC3 tip = arm.GetState() == TENTACLE_IDLE ? origin : arm.GetTip();
	if (arm.GetState() != TENTACLE_HOLDING) {
		// 描画する先端だけを変える。計算上の arm.GetTip()・長さ・判定はそのまま。
		const VEC3 extended = Vec3Sub(tip, origin);
		const float length = Vec3Length(extended);
		const VEC3 rest = GetRestDirection();
		const VEC3 aim = length < MODEL_MIN_LENGTH ? rest : Vec3Scale(extended, 1.0f / length);
		const VEC3 direction = ArmCalcBlendDirection(rest, aim, length / IDLE_LENGTH);
		tip = Vec3Add(origin, Vec3Scale(direction, fmaxf(length, IDLE_LENGTH)));
	}
	const VEC3 delta = Vec3Sub(tip, origin);
	const int steps = (int)ceilf(Vec3Length(delta) / SPHERE_SPACING) + 1;
	const XMFLOAT4 color = GetColor(arm.GetTarget() >= 0);
	if (m_BaseModel >= 0 && m_SegmentModel >= 0 && m_TipModel >= 0) {
		const float length = Vec3Length(delta);
		const float remaining = length / MODEL_SCALE - BASE_LENGTH - TIP_LENGTH;
		const int segments = remaining > 0.0f ? (int)floorf(remaining / SEGMENT_LENGTH + 0.5f) : 0;
		const float total = BASE_LENGTH + segments * SEGMENT_LENGTH + TIP_LENGTH;
		// 短すぎる腕は見た目だけ最低長まで伸ばす。判定や arm.GetTip() は変えない。
		const float drawLength = fmaxf(length, total * MODEL_SCALE * MIN_STRETCH);
		const float yaw = GetPlayer().GetYaw();
		const VEC3 direction = length < MODEL_MIN_LENGTH ? VEC3{ sinf(yaw), 0, cosf(yaw) }
			: Vec3Scale(delta, 1.0f / length);
		const VEC3 drawTip = Vec3Add(origin, Vec3Scale(direction, drawLength));
		const float phase = GameTimeGetElapsed() * WIGGLE_SPEED + hand * WIGGLE_ARM_PHASE;
		const float amplitude = WIGGLE_AMPLITUDE * fminf(1.0f, length / WIGGLE_FULL_LENGTH)
			* (arm.GetState() == TENTACLE_HOLDING ? WIGGLE_HOLD_SCALE : 1.0f);
		float distance = 0.0f;
		VEC3 start = origin;
		for (int piece = 0; piece < segments + 2; ++piece) {
			const bool base = piece == 0;
			const bool last = piece == segments + 1;
			const float sourceLength = base ? BASE_LENGTH : (last ? TIP_LENGTH : SEGMENT_LENGTH);
			distance += sourceLength;
			const VEC3 end = TentacleCalcWigglePoint(origin, drawTip, last ? 1.0f : distance / total,
				phase, amplitude, WIGGLE_WAVES);
			DrawPiece(base ? m_BaseModel : (last ? m_TipModel : m_SegmentModel),
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

bool TentacleArm::IsHolding() const
{
	return m_Arm.GetState() == TENTACLE_HOLDING;
}

bool TentacleArm::IsActive() const
{
	return m_Arm.GetState() != TENTACLE_IDLE;
}
