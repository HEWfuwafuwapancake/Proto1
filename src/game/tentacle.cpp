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
static constexpr VEC3 MOUNT_OFFSET{ 0.0f, 0.24f, -0.06f }; // 取付位置のずらし（外・上・前 m）。付け根の箱の中央を取付口へ
static constexpr float BASE_LENGTH = 10.218f; // OBJ のつなぎ目間の長さ（cm）
static constexpr float SEGMENT_LENGTH = 3.207f;
static constexpr float TIP_LENGTH = 4.434f;
static constexpr float MIN_STRETCH = 0.2f;
static constexpr float IDLE_LENGTH = 1.2f; // 待機中・短い伸縮中に描く長さ（判定には使わない）
static constexpr int BONE_IDLE_SEGMENTS = 5; // 腕の骨に付くキャラの待機中の節の数（元モデルの TAKO1〜5 と同じ1本分）
// 待機中は垂れ下がり、走ると後ろへなびく形（描画だけ）
static constexpr float DROOP_BEND = 0.3f; // 肩から最初に外へ出る割合（残りは下へ垂れる）
static constexpr float DROOP_OUT_DOWN = 0.8f; // 最初に出る向きの下向きの量（外向き 1 に対して）
static constexpr float TRAIL_ANGLE = XMConvertToRadians(65.0f); // 全速で走ったとき、垂れる向きを後ろへ倒す角度
static constexpr float TRAIL_FULL_SPEED = 6.0f; // なびきが最大になる横の速さ
static constexpr float TRAIL_RATE = 5.0f; // なびきが目標へ近づく速さ（1秒あたりの割合）
static constexpr float SWAY_IDLE_SPEED = 1.8f; // 待機中のゆっくりした揺れの速さ（ラジアン／秒）
static constexpr float SWAY_RUN_SPEED = 11.0f; // 走っているときの揺れの速さ
static constexpr float SWAY_IDLE_AMPLITUDE = 0.04f; // 待機中の先の横揺れ（m）
static constexpr float SWAY_RUN_AMPLITUDE = 0.12f; // 走っているときの先の横揺れ（m）
static constexpr float SWAY_IDLE_WAVES = 0.4f; // 待機中に触腕全体へ入れる波の数
static constexpr float SWAY_RUN_WAVES = 1.2f; // 走っているときの波の数
static constexpr float MODEL_MIN_LENGTH = 0.00001f;
static constexpr float WIGGLE_AMPLITUDE = 0.25f;
static constexpr float WIGGLE_WAVES = 1.5f;
static constexpr float WIGGLE_SPEED = 8.0f;
static constexpr float WIGGLE_ARM_PHASE = 1.6f;
static constexpr float WIGGLE_FULL_LENGTH = 3.0f;
static constexpr float WIGGLE_HOLD_SCALE = 0.3f;

VEC3 TentacleArm::GetMountOffset() const
{
	return MOUNT_OFFSET;
}

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
	const VEC3 origin = GetDrawShoulder();
	const VEC3 physicalOrigin = GetShoulder();
	VEC3 tip = arm.GetState() == TENTACLE_IDLE ? physicalOrigin : arm.GetTip();
	const float modelScale = MODEL_SCALE * GetDrawScale();
	// 腕の骨があるキャラは、待機の長さを元モデル1本分（付け根・節5つ・先っぽ）を今の大きさで描いた長さにする。
	const VEC3 rest = GetRestDirection();
	float idleLength = IDLE_LENGTH;
	VEC3 boneShoulder, boneDirection;
	float boneLength = 0.0f;
	if (GetPlayer().GetArmBone(hand, boneShoulder, boneDirection, boneLength)) {
		idleLength = (BASE_LENGTH + SEGMENT_LENGTH * BONE_IDLE_SEGMENTS + TIP_LENGTH) * modelScale;
	}

	// 待機中の形：肩から外へ少し出て下へ垂れ、走ると進む向きの反対へなびく（人の腕のようには振らない）。
	const float dt = GameTimeGetDeltaTime();
	const VEC3 velocity = GetPlayer().GetVelocity();
	const float speed = sqrtf(velocity.X * velocity.X + velocity.Z * velocity.Z);
	if (speed > MODEL_MIN_LENGTH) m_TrailDirection = { -velocity.X / speed, 0.0f, -velocity.Z / speed };
	const float trailGoal = fminf(1.0f, speed / TRAIL_FULL_SPEED);
	m_Trail += (trailGoal - m_Trail) * fminf(1.0f, TRAIL_RATE * dt);
	m_SwayPhase += (SWAY_IDLE_SPEED + (SWAY_RUN_SPEED - SWAY_IDLE_SPEED) * m_Trail) * dt;
	const float yaw = GetPlayer().GetYaw();
	const VEC3 side = Vec3Scale(VEC3{ cosf(yaw), 0.0f, -sinf(yaw) }, hand == 0 ? -1.0f : 1.0f);
	const VEC3 outwardRaw = Vec3Add(side, VEC3{ 0.0f, -DROOP_OUT_DOWN, 0.0f });
	const VEC3 outward = Vec3Scale(outwardRaw, 1.0f / Vec3Length(outwardRaw));
	const float trailAngle = TRAIL_ANGLE * m_Trail;
	const VEC3 hang = Vec3Add(VEC3{ 0.0f, -cosf(trailAngle), 0.0f }, Vec3Scale(m_TrailDirection, sinf(trailAngle)));
	const float swayAmplitude = SWAY_IDLE_AMPLITUDE + (SWAY_RUN_AMPLITUDE - SWAY_IDLE_AMPLITUDE) * m_Trail;
	const float swayWaves = SWAY_IDLE_WAVES + (SWAY_RUN_WAVES - SWAY_IDLE_WAVES) * m_Trail;
	const float swayPhase = m_SwayPhase + hand * WIGGLE_ARM_PHASE;
	// 伸ばし始めは垂れた形から少しずつまっすぐにする（つかんでいる間は使わない）。
	float droop = 0.0f;
	if (arm.GetState() == TENTACLE_IDLE) droop = 1.0f;
	else if (arm.GetState() != TENTACLE_HOLDING) {
		droop = fmaxf(0.0f, 1.0f - Vec3Length(Vec3Sub(arm.GetTip(), physicalOrigin)) / idleLength);
	}
	if (arm.GetState() != TENTACLE_HOLDING) {
		// 描画する先端だけを変える。計算上の arm.GetTip()・長さ・判定はそのまま。
		const VEC3 extended = Vec3Sub(tip, physicalOrigin);
		const float length = Vec3Length(extended);
		const VEC3 aim = length < MODEL_MIN_LENGTH ? rest : Vec3Scale(extended, 1.0f / length);
		const VEC3 direction = ArmCalcBlendDirection(rest, aim, length / idleLength);
		tip = Vec3Add(length < idleLength ? origin : physicalOrigin, Vec3Scale(direction, fmaxf(length, idleLength)));
	}
	const VEC3 delta = Vec3Sub(tip, origin);
	const int steps = (int)ceilf(Vec3Length(delta) / SPHERE_SPACING) + 1;
	const XMFLOAT4 color = GetColor(arm.GetTarget() >= 0);
	if (m_BaseModel >= 0 && m_SegmentModel >= 0 && m_TipModel >= 0) {
		const float length = Vec3Length(delta);
		const float remaining = length / modelScale - BASE_LENGTH - TIP_LENGTH;
		const int segments = remaining > 0.0f ? (int)floorf(remaining / SEGMENT_LENGTH + 0.5f) : 0;
		const float total = BASE_LENGTH + segments * SEGMENT_LENGTH + TIP_LENGTH;
		// 短すぎる腕は見た目だけ最低長まで伸ばす。判定や arm.GetTip() は変えない。
		const float drawLength = fmaxf(length, total * modelScale * MIN_STRETCH);
		const VEC3 direction = length < MODEL_MIN_LENGTH ? VEC3{ sinf(yaw), 0, cosf(yaw) }
			: Vec3Scale(delta, 1.0f / length);
		const VEC3 drawTip = Vec3Add(origin, Vec3Scale(direction, drawLength));
		const float phase = GameTimeGetElapsed() * WIGGLE_SPEED + hand * WIGGLE_ARM_PHASE;
		// 待機中は揺らさない（ブルブルして見えるため）。伸ばしている間とつかんでいる間だけ揺らす。
		const float amplitude = arm.GetState() == TENTACLE_IDLE ? 0.0f
			: WIGGLE_AMPLITUDE * fminf(1.0f, length / WIGGLE_FULL_LENGTH)
			* (arm.GetState() == TENTACLE_HOLDING ? WIGGLE_HOLD_SCALE : 1.0f);
		float distance = 0.0f;
		VEC3 start = origin;
		for (int piece = 0; piece < segments + 2; ++piece) {
			const bool base = piece == 0;
			const bool last = piece == segments + 1;
			const float sourceLength = base ? BASE_LENGTH : (last ? TIP_LENGTH : SEGMENT_LENGTH);
			distance += sourceLength;
			const float s = last ? 1.0f : distance / total;
			VEC3 end = TentacleCalcWigglePoint(origin, drawTip, s, phase, amplitude, WIGGLE_WAVES);
			if (droop > 0.0f) {
				const VEC3 hanging = TentacleCalcDroopPoint(origin, outward, hang, drawLength, DROOP_BEND,
					s, side, swayPhase, swayAmplitude, swayWaves);
				end = Vec3Add(Vec3Scale(end, 1.0f - droop), Vec3Scale(hanging, droop));
			}
			DrawPiece(base ? m_BaseModel : (last ? m_TipModel : m_SegmentModel),
				start, end, sourceLength, modelScale, color);
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
