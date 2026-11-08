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
#include <algorithm>

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
static constexpr int BONE_IDLE_SEGMENTS = 3; // 腕の骨に付くキャラの待機中の節の数（元モデルの TAKO1〜5 と同じ1本分）
// 待機中の節の鎖の物理（描画だけ）。付け根の箱は体に固定し、その下に鎖がぶら下がる
static constexpr float ROPE_GRAVITY = 14.0f; // 鎖にかかる重力（大きいほど重く速く振れる）
static constexpr float ROPE_DAMPING = 2.5f; // 体に対する揺れが1秒で減る割合（小さいほど長く振れる）
static constexpr float ROPE_AIR_DRAG = 0.3f; // 空気抵抗（大きいほど走ったとき後ろへなびく）
static constexpr float ROPE_MAX_BEND = XMConvertToRadians(14.0f); // 途中の節のつなぎ目の可動域
static constexpr float ROPE_MAX_ROOT_BEND = XMConvertToRadians(60.0f); // 肩（付け根の箱→最初の節）の可動域（広め）
static constexpr float ROPE_MAX_TIP_BEND = XMConvertToRadians(20.0f); // 先っぽのつなぎ目の可動域
static constexpr int ROPE_SUBSTEPS = 4; // 1フレームを分ける数
static constexpr int ROPE_ITERATIONS = 3; // 1回あたりに節の長さ・曲がりを合わせる回数
static constexpr float ROPE_BODY_RATE = 1.1f; // 体（腰・脚）の半径＝肩の横の距離 × これ
static constexpr float ROPE_THICKNESS = 3.0f; // 節の中心から表面までの太さ（OBJ の cm。体の半径に足す）
static constexpr float ROPE_RESET_DISTANCE = 1.5f; // 1フレームでこれ以上動いたら（ワープ・復活）並べ直す
static constexpr float ROPE_RESET_OUTWARD = 0.3f; // 並べ直すとき、真下に対して外へ開く量
static constexpr float PIECE_DOWN_MIN = 0.3f; // 吸盤側の向きを横の軸から決められる最小の長さ
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

// 節の向き：モデルの X をキャラの左右の軸に合わせる（付け根の平たい面が体に付く）。
// 吸盤側（-Z）は「右 × 腕の向き」（前へ出した腕なら下、垂れた腕なら後ろ）へ向ける。
static VEC3 PieceDown(const VEC3& start, const VEC3& end, const VEC3& right)
{
	const VEC3 d = Vec3Sub(end, start);
	const float length = Vec3Length(d);
	if (length < MODEL_MIN_LENGTH) return { 0.0f, -1.0f, 0.0f };
	const VEC3 dir = Vec3Scale(d, 1.0f / length);
	const VEC3 down{ right.Y * dir.Z - right.Z * dir.Y, right.Z * dir.X - right.X * dir.Z, right.X * dir.Y - right.Y * dir.X };
	return Vec3Length(down) < PIECE_DOWN_MIN ? VEC3{ 0.0f, -1.0f, 0.0f } : down;
}

void TentacleArm::StepRope(const VEC3& origin, float idleLength, float modelScale)
{
	// 画面分割で何度描いても、1フレームに1回だけ進める。
	const float now = GameTimeGetElapsed();
	if (now == m_RopeTime && m_Rope.GetPointCount() > 0) return;
	const float dt = now - m_RopeTime;
	m_RopeTime = now;
	const VEC3 anchor = Vec3Add(origin, { 0.0f, -BASE_LENGTH * modelScale, 0.0f });
	const int segments = std::max(0, std::min(TentacleRopeCalc::MAX_POINTS - 2,
		(int)floorf((idleLength / modelScale - BASE_LENGTH - TIP_LENGTH) / SEGMENT_LENGTH + 0.5f)));
	float lengths[TentacleRopeCalc::MAX_POINTS] = {};
	for (int i = 0; i < segments; ++i) lengths[i] = SEGMENT_LENGTH * modelScale;
	lengths[segments] = TIP_LENGTH * modelScale;
	const bool changed = m_Rope.GetPointCount() != segments + 2 || m_RopeScale != modelScale;
	if (changed || dt <= 0.0f || Vec3Length(Vec3Sub(anchor, m_Rope.GetPoint(0))) > ROPE_RESET_DISTANCE) {
		const float yaw = GetPlayer().GetYaw();
		const float side = GetHand() == 0 ? -1.0f : 1.0f;
		const VEC3 outward{ cosf(yaw) * side * ROPE_RESET_OUTWARD, -1.0f, -sinf(yaw) * side * ROPE_RESET_OUTWARD };
		m_Rope.Reset(anchor, Vec3Scale(outward, 1.0f / Vec3Length(outward)), lengths, segments + 1);
		m_RopeScale = modelScale;
		return;
	}
	TENTACLE_ROPE_PARAMS params;
	params.Gravity = ROPE_GRAVITY;
	params.Damping = ROPE_DAMPING;
	params.AirDrag = ROPE_AIR_DRAG;
	params.MaxBend = ROPE_MAX_BEND;
	params.MaxRootBend = ROPE_MAX_ROOT_BEND;
	params.MaxTipBend = ROPE_MAX_TIP_BEND;
	params.RootDirection = { 0.0f, -1.0f, 0.0f };
	params.Substeps = ROPE_SUBSTEPS;
	params.Iterations = ROPE_ITERATIONS;
	const XMFLOAT3 p = GetPlayer().GetPosition();
	params.BodyBottom = { p.x, p.y, p.z };
	params.BodyTop = { p.x, origin.Y, p.z };
	// 体は足元から肩までの円柱。腰・脚の幅として肩の横の距離より太くし、節の太さも足す（すり抜けないように）。
	params.BodyRadius = sqrtf((origin.X - p.x) * (origin.X - p.x) + (origin.Z - p.z) * (origin.Z - p.z)) * ROPE_BODY_RATE
		+ ROPE_THICKNESS * modelScale;
	m_Rope.Step(anchor, params, dt);
}

void TentacleArm::Draw()
{
	const int hand = GetHand();
	const TentacleCalc& arm = m_Arm;
	const VEC3 origin = GetDrawShoulder();
	const VEC3 physicalOrigin = GetShoulder();
	VEC3 tip = arm.GetState() == TENTACLE_IDLE ? physicalOrigin : arm.GetTip();
	const float modelScale = MODEL_SCALE * GetDrawScale();
	const float yaw = GetPlayer().GetYaw();
	const VEC3 right{ cosf(yaw), 0.0f, -sinf(yaw) };
	// 腕の骨があるキャラは、待機の長さを元モデル1本分（付け根・節5つ・先っぽ）を今の大きさで描いた長さにする。
	const VEC3 rest = GetRestDirection();
	float idleLength = IDLE_LENGTH;
	VEC3 boneShoulder, boneDirection;
	float boneLength = 0.0f;
	if (GetPlayer().GetArmBone(hand, boneShoulder, boneDirection, boneLength)) {
		idleLength = (BASE_LENGTH + SEGMENT_LENGTH * BONE_IDLE_SEGMENTS + TIP_LENGTH) * modelScale;
	}
	// 待機中は物理の鎖の形。伸ばし始めは鎖の形から少しずつまっすぐにする（つかんでいる間は使わない）。
	StepRope(origin, idleLength, modelScale);
	float hanging = 0.0f;
	if (arm.GetState() == TENTACLE_IDLE) hanging = 1.0f;
	else if (arm.GetState() != TENTACLE_HOLDING) {
		hanging = fmaxf(0.0f, 1.0f - Vec3Length(Vec3Sub(arm.GetTip(), physicalOrigin)) / idleLength);
	}
	// 付け根の箱は待機中は真下。伸ばすほど、箱の真ん中を支点に先端の方へ回す（肩も一緒に回って見える）。
	const float baseLength = BASE_LENGTH * modelScale;
	const VEC3 baseCenter = Vec3Add(origin, { 0.0f, -baseLength * 0.5f, 0.0f });
	const VEC3 hangAnchor = Vec3Add(origin, { 0.0f, -baseLength, 0.0f }); // 真下に垂らしたときの箱の下端（鎖の根元）
	const VEC3 baseDirection = ArmCalcBlendDirection({ 0.0f, -1.0f, 0.0f }, Vec3Sub(tip, baseCenter), 1.0f - hanging);
	const VEC3 baseTop = Vec3Sub(baseCenter, Vec3Scale(baseDirection, baseLength * 0.5f));
	const VEC3 anchor = Vec3Add(baseCenter, Vec3Scale(baseDirection, baseLength * 0.5f)); // 回した箱の下端
	const VEC3 ropeShift = Vec3Sub(anchor, hangAnchor); // 鎖の形を回した箱の下端へ寄せる
	if (arm.GetState() != TENTACLE_HOLDING) {
		// 描画する先端だけを変える。計算上の arm.GetTip()・長さ・判定はそのまま。
		const VEC3 extended = Vec3Sub(tip, physicalOrigin);
		const float length = Vec3Length(extended);
		const VEC3 aim = length < MODEL_MIN_LENGTH ? rest : Vec3Scale(extended, 1.0f / length);
		const VEC3 direction = ArmCalcBlendDirection(rest, aim, length / idleLength);
		tip = length < idleLength ? Vec3Add(anchor, Vec3Scale(direction, idleLength - baseLength)) : tip;
	}
	// 節は箱の下端から先端へ伸ばす（肩から外れて見えないように）。
	const VEC3 delta = Vec3Sub(tip, baseTop);
	const int steps = (int)ceilf(Vec3Length(delta) / SPHERE_SPACING) + 1;
	const XMFLOAT4 color = GetColor(arm.GetTarget() >= 0);
	if (m_BaseModel >= 0 && m_SegmentModel >= 0 && m_TipModel >= 0) {
		const float length = Vec3Length(Vec3Sub(tip, anchor)) + baseLength; // 箱の長さ＋箱の下端から先端まで
		const float remaining = length / modelScale - BASE_LENGTH - TIP_LENGTH;
		const int segments = remaining > 0.0f ? (int)floorf(remaining / SEGMENT_LENGTH + 0.5f) : 0;
		const float total = BASE_LENGTH + segments * SEGMENT_LENGTH + TIP_LENGTH;
		// 短すぎる腕は見た目だけ最低長まで伸ばす。判定や arm.GetTip() は変えない。
		const float drawLength = fmaxf(length, total * modelScale * MIN_STRETCH);
		const VEC3 direction = length < MODEL_MIN_LENGTH ? VEC3{ sinf(yaw), 0, cosf(yaw) }
			: Vec3Scale(delta, 1.0f / length);
		const VEC3 chainDelta = Vec3Sub(tip, anchor);
		const float chainLength = Vec3Length(chainDelta);
		const VEC3 chainDirection = chainLength < MODEL_MIN_LENGTH ? direction : Vec3Scale(chainDelta, 1.0f / chainLength);
		const VEC3 drawTip = Vec3Add(anchor, Vec3Scale(chainDirection, fmaxf(chainLength, drawLength - baseLength)));
		const float phase = GameTimeGetElapsed() * WIGGLE_SPEED + hand * WIGGLE_ARM_PHASE;
		// 待機中は揺らさない（揺れは鎖の物理に任せる）。伸ばしている間とつかんでいる間だけ揺らす。
		const float amplitude = arm.GetState() == TENTACLE_IDLE ? 0.0f
			: WIGGLE_AMPLITUDE * fminf(1.0f, length / WIGGLE_FULL_LENGTH)
			* (arm.GetState() == TENTACLE_HOLDING ? WIGGLE_HOLD_SCALE : 1.0f);
		float distance = 0.0f;
		VEC3 start = baseTop;
		for (int piece = 0; piece < segments + 2; ++piece) {
			const bool base = piece == 0;
			const bool last = piece == segments + 1;
			const float sourceLength = base ? BASE_LENGTH : (last ? TIP_LENGTH : SEGMENT_LENGTH);
			distance += sourceLength;
			// 付け根の箱は肩で回した向きへ。その先は、伸ばした形（箱の下端→先端）と鎖の形を混ぜる。
			const float s = last ? 1.0f : (distance - BASE_LENGTH) / (total - BASE_LENGTH);
			VEC3 end = anchor;
			if (!base) {
				end = TentacleCalcWigglePoint(anchor, drawTip, s, phase, amplitude, WIGGLE_WAVES);
				if (hanging > 0.0f) end = Vec3Add(Vec3Scale(end, 1.0f - hanging), Vec3Scale(Vec3Add(m_Rope.GetPointAt(s), ropeShift), hanging));
			}
			DrawPiece(base ? m_BaseModel : (last ? m_TipModel : m_SegmentModel),
				start, end, sourceLength, modelScale, color, 0.0f, PieceDown(start, end, right));
			start = end;
		}
		return;
	}
	for (int i = 0; i <= steps; ++i) {
		const VEC3 p = Vec3Add(baseTop, Vec3Scale(delta, (float)i / steps));
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
