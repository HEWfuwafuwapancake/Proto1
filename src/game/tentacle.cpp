#include "game/tentacle.h"
#include "game/camera.h"
#include "game/fightermanager.h"
#include "game/fighter.h"
#include "game/stage.h"
#include "core/gametime.h"
#include "core/mathutil.h"
#include "game/arm.h"
#include "input/pad.h"
#include "physics/areacalc.h"
#include "physics/armcalc.h"
#include "physics/grapplecalc.h"
#include "render/shape.h"
#include <algorithm>

// 調整用の値
// 耐久の減る量は physics/armcostcalc の表の触手の行（弱：つかんだ・足場に付いた、強：回転）。
static constexpr ARM_COST_ROW COST_ROW = ARM_COST_ROW_TENTACLE;
static constexpr TENTACLE_PARAMS PARAMS{ 8.0f, 24.0f, 30.0f, 3.0f, 0.18f, 0.35f };
static constexpr float GRAB_DAMAGE = 4.0f; // つかんだときに足す %
static constexpr float THROW_DAMAGE = 4.0f;
static constexpr float THROW_POWER = 1.5f;
static constexpr float GRIP_RANGE = 16.0f; // 足場のふちへ伸ばせる距離（上下も含む）
static constexpr float GRIP_HALF_ANGLE = XMConvertToRadians(40.0f); // 足場のふちを探す画面の正面の開き（見上げているときは上下も含む）
static constexpr float LOOK_UP_SINE = 0.09f; // カメラの向きの Y がこれより大きいと「見上げている」（約 5°）
static constexpr TENTACLE_PULL_PARAMS PULL_PARAMS{ 20.0f, 0.8f, 1.5f };
static constexpr float LANDING_INSET = 0.8f; // 足場のふちから内側へ寄せて着地する距離
static constexpr float SPIN_PHASE_SECONDS[] = { 0.25f, 0.4f, 0.2f }; // 溜め・回転・休み
static constexpr float SPIN_RADIUS = 3.5f;
static constexpr float SPIN_DAMAGE = 10.0f;
static constexpr float SPIN_POWER = 2.0f;
static constexpr float SPIN_UP = 0.6f; // 外への水平の単位方向に足す上向きの量
static constexpr float SPIN_HALF_HEIGHT = 1.5f; // 肩の高さから上下それぞれの当たる幅
static constexpr float SPIN_MARGIN = XMConvertToRadians(2.0f); // 1フレーム分の扇に足す角度（つなぎ目の取りこぼし防止）
static constexpr float SPIN_FULL_TURN = XMConvertToRadians(360.0f);
static constexpr float SPIN_START_ANGLE = XMConvertToRadians(45.0f); // 回し始めの、正面から腕の側への角度（左腕なら左前から右へ振る）
static constexpr float SPIN_RUMBLE_SMALL = 0.6f;
static constexpr float SPIN_RUMBLE_LARGE = 0.6f;
static constexpr float SPIN_RUMBLE_SECONDS = 0.15f;
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
// 待機中の節の鎖の物理（描画だけ）。付け根の箱は真ん中の肩を支点に振れ、その先に節がぶら下がる
static constexpr float ROPE_GRAVITY = 14.0f; // 鎖にかかる重力（大きいほど重く速く振れる）
static constexpr float ROPE_DAMPING = 3.5f; // 体に対する揺れが1秒で減る割合（小さいほど長く振れる）
static constexpr float ROPE_AIR_DRAG = 1.0f; // 空気抵抗（大きいほど走ったとき後ろへなびく。全速の前傾 10° より大きく流す）
static constexpr float ROPE_BODY_FOLLOW = 0.7f; // 走り出し・急停止の速さの変化を腕も一緒に受ける割合（大きいほど振られず、なめらか）
static constexpr float ROPE_SHOULDER_FORWARD = XMConvertToRadians(180.0f); // 肩の可動域：前へ（人の屈曲）
static constexpr float ROPE_SHOULDER_BACK = XMConvertToRadians(50.0f); // 肩の可動域：後ろへ（人の伸展）
static constexpr float ROPE_SHOULDER_OUTWARD = XMConvertToRadians(180.0f); // 肩の可動域：体の外へ（人の外転）
static constexpr float ROPE_SHOULDER_INWARD = XMConvertToRadians(15.0f); // 肩の可動域：体の内へ（人の内転。体に当たるので狭い）
static constexpr float ROPE_MAX_BEND = XMConvertToRadians(14.0f); // 途中の節のつなぎ目の可動域
static constexpr float ROPE_MAX_ELBOW_BEND = XMConvertToRadians(60.0f); // 付け根の箱→最初の節の可動域（広め）
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

enum SPIN_PHASE { SPIN_CHARGE, SPIN_SWING, SPIN_RECOVER, SPIN_PHASE_COUNT };

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
	if (m_ReachPending) ConsumeDurability(GetCost(COST_ROW, ARM_COST_TAP_MISS));
	m_ReachPending = false;
	if (!m_Spin.IsIdle() && !m_SpinConsumed) ConsumeDurability(GetCost(COST_ROW, ARM_COST_HOLD_MISS));
	m_SpinConsumed = true;
	TentacleCalc& arm = m_Arm;
	GetFighters().Release(arm.GetTarget(), GetSlot(), false, {}, 0.0f, 0.0f);
	arm.Reset();
	m_Spin.Reset();
	StopPulling();
}

TentacleArm::~TentacleArm()
{
	Reset();
}

void TentacleArm::ConnectOtherArm(TentacleArm& other)
{
	m_OtherArm = &other;
	other.m_OtherArm = this;
}

void TentacleArm::StopPulling()
{
	if (!m_Pulling) return;
	m_Pulling = false;
	GetPlayer().SetPulled(false);
}

// 足場に付けるのは、フリーカメラの間と、浮島の外で宙にいる復帰の間だけ。
bool TentacleArm::CanAnchor() const
{
	const XMFLOAT3 p = GetPlayer().GetPosition();
	return TentacleCalcCanAnchor(GetCamera().IsFreeLook(), GetPlayer().IsOnGround(), GetStage().GetStage(), p.x, p.z);
}

// 伸ばす先を決める：届く距離のロックオン中の敵 → 画面の正面の足場のふち（CanAnchor のときだけ。復帰は敵より足場を優先） → 画面の正面。
// 見上げているときだけ上下も含めて狙う。
void TentacleArm::StartReach(const TENTACLE_TARGET* targets, const VEC3& origin, const VEC3& cameraForward,
	VEC3& aim, STAGE_GRIP& grip, bool& hasGrip) const
{
	const XMFLOAT3 view = GetCamera().GetDirection();
	const bool lookingUp = view.y > LOOK_UP_SINE;
	const VEC3 viewForward = lookingUp ? VEC3{ view.x, view.y, view.z } : cameraForward;
	// ロックオン中は、見上げていても敵へ伸ばす（足場のふちは場外から戻るときか、敵が届かないときだけ）。
	const int target = GetAimTarget();
	const VEC3 center = target >= 0
		? Vec3Scale(Vec3Add(targets[target].Body.Start, targets[target].Body.End), 0.5f) : VEC3{};
	const XMFLOAT3 p = GetPlayer().GetPosition();
	// ロックオン中は敵をつかむ操作に絞る。場外から戻るときだけは敵より足場を優先する。
	const bool recovering = !GetCamera().IsFreeLook() && CanAnchor();
	hasGrip = CanAnchor() && (recovering || target < 0 || Vec3Length(Vec3Sub(center, origin)) > PARAMS.MaxLength)
		&& StageCalcFindGrip(GetStage().GetStage(), origin, { p.x, p.y, p.z }, viewForward, GRIP_RANGE, GRIP_HALF_ANGLE, grip);
	// 各肩からの向きを開始時だけ決める。伸長中は追いかけず、投げる向きとも分ける。
	aim = hasGrip ? Vec3Sub(grip.Point, origin) : (target >= 0 ? Vec3Sub(center, origin) : viewForward);
}

void TentacleArm::StepPull(const TENTACLE_EVENT& event, float dt)
{
	if (event.Anchored) {
		ConsumeDurability(GetCost(COST_ROW, ARM_COST_TAP_HIT));
		m_ReachPending = false;
		// 新しく付いた手へ受け持ちを移す。先の手は腕を戻す。
		if (m_OtherArm != nullptr && m_OtherArm->m_Pulling) {
			m_OtherArm->m_Arm.ReleasePull();
			m_OtherArm->m_Pulling = false;
		}
		m_Pulling = true;
		GetPlayer().SetPulled(true);
		Rumble(GRAB_RUMBLE_SMALL, GRAB_RUMBLE_LARGE, GRAB_RUMBLE_SECONDS);
	}
	if (event.PullEnded) StopPulling();
	if (!m_Pulling) return;
	const XMFLOAT3 p = GetPlayer().GetPosition();
	const float halfHeight = GetPlayer().GetHeight() * 0.5f;
	VEC3 center{ p.x, p.y + halfHeight, p.z };
	const TENTACLE_PULL pull = m_Arm.StepPull(center, PULL_PARAMS, dt);
	if (pull.Arrived) {
		const VEC3 landing = GrappleCalcLanding(m_Arm.GetTip(), m_Arm.GetSurface(), LANDING_INSET);
		GetPlayer().SetPosition({ landing.X, landing.Y, landing.Z });
	}
	else {
		GetPlayer().SetPosition({ center.X, center.Y - halfHeight, center.Z });
	}
	if (pull.Ended) StopPulling();
}

VEC3 TentacleArm::GetSpinTip() const
{
	const float side = GetHand() == 0 ? -1.0f : 1.0f;
	// 腕の側の斜め前から、反対の側へ向かって回す。
	return Vec3Add(GetShoulder(), Vec3Scale(AreaCalcRotateYaw(m_SpinForward, side * (SPIN_START_ANGLE - m_SpinAngle)), SPIN_RADIUS));
}

void TentacleArm::StepSpin(const ARM_INPUT& input, float dt)
{
	if (m_Spin.IsIdle() && input.HoldStarted && m_Arm.GetState() == TENTACLE_IDLE) {
		m_SpinForward = GetAim(false);
		m_SpinAngle = 0.0f;
		m_SpinHits.Clear();
		m_SpinConsumed = false;
		m_Spin.Start(SPIN_CHARGE);
	}
	const ARM_PHASE_STEP step = m_Spin.Step(dt, SPIN_PHASE_SECONDS, SPIN_PHASE_COUNT);
	if (step.Finished && !m_SpinConsumed) {
		ConsumeDurability(GetCost(COST_ROW, ARM_COST_HOLD_MISS));
		m_SpinConsumed = true;
	}
	if (m_Spin.GetPhase() != SPIN_SWING && step.Entered != SPIN_RECOVER) return;
	// 前のフレームから今までに回った扇だけを調べる（段階の終わりは必ず一周まで）。
	const float angle = step.Entered == SPIN_RECOVER ? SPIN_FULL_TURN
		: SPIN_FULL_TURN * fminf(1.0f, m_Spin.GetTime() / SPIN_PHASE_SECONDS[SPIN_SWING]);
	const float side = GetHand() == 0 ? -1.0f : 1.0f;
	const VEC3 middle = AreaCalcRotateYaw(m_SpinForward, side * (SPIN_START_ANGLE - (m_SpinAngle + angle) * 0.5f));
	const float halfAngle = (angle - m_SpinAngle) * 0.5f + SPIN_MARGIN;
	const XMFLOAT3 p = GetPlayer().GetPosition();
	const VEC3 center{ p.x, GetShoulder().Y, p.z };
	bool hit = false;
	for (int i = 0; i < FIGHTER_COUNT; ++i) {
		const TENTACLE_TARGET target = GetFighters().GetTarget(i, GetSlot());
		if (!target.Available || m_SpinHits.Contains(i)
			|| !AreaCalcInSector(center, middle, 0.0f, SPIN_RADIUS, halfAngle, SPIN_HALF_HEIGHT, target.Body)) continue;
		m_SpinHits.TryAdd(i);
		const bool applied = GetFighters().Launch(i, GetSlot(), AreaCalcRadialDirection(center, target.Body, SPIN_UP, middle), SPIN_DAMAGE, SPIN_POWER);
		if (applied && !m_SpinConsumed) {
			ConsumeDurability(GetCost(COST_ROW, ARM_COST_HOLD_HIT));
			m_SpinConsumed = true;
		}
		hit = true;
	}
	m_SpinAngle = angle;
	if (hit) Rumble(SPIN_RUMBLE_SMALL, SPIN_RUMBLE_LARGE, SPIN_RUMBLE_SECONDS);
}

void TentacleArm::Update(const ARM_INPUT& input)
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
	STAGE_GRIP grip{};
	bool hasGrip = false;
	// 回転の間は単押しで伸ばさない。
	const bool trigger = input.Tap && m_Spin.IsIdle();
	if (trigger && arm.GetState() == TENTACLE_IDLE) {
		m_ReachPending = true;
		StartReach(targets, origin, cameraForward, aim, grip, hasGrip);
	}
	const float dt = GameTimeGetDeltaTime();
	const bool cancel = GetPlayer().DidRespawn() || (arm.GetTarget() >= 0 && !GetFighters().IsHeldBy(arm.GetTarget(), GetSlot()));
	const TENTACLE_EVENT event = arm.Step(origin, aim, hold, trigger, cancel,
		dt, PARAMS, targets, FIGHTER_COUNT, GetStage().GetStage(), hasGrip ? &grip : nullptr, CanAnchor());
	if (event.Grabbed >= 0) {
		if (GetFighters().Grab(event.Grabbed, GetSlot())) {
			ConsumeDurability(GetCost(COST_ROW, ARM_COST_TAP_HIT));
			m_ReachPending = false;
			GetFighters().AddDamage(event.Grabbed, GetSlot(), GRAB_DAMAGE);
			Rumble(GRAB_RUMBLE_SMALL, GRAB_RUMBLE_LARGE, GRAB_RUMBLE_SECONDS);
		}
		else arm.Reset();
	}
	if (m_ReachPending && arm.GetState() == TENTACLE_IDLE) {
		ConsumeDurability(GetCost(COST_ROW, ARM_COST_TAP_MISS));
		m_ReachPending = false;
	}
	StepPull(event, dt);
	StepSpin(input, dt);
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
	// 根元は箱の真ん中（肩）。最初の節が箱の下半分で、これが上腕として振れる。
	const float halfBase = BASE_LENGTH * modelScale * 0.5f;
	const VEC3 anchor = Vec3Add(origin, { 0.0f, -halfBase, 0.0f });
	const int segments = std::max(0, std::min(TentacleRopeCalc::MAX_POINTS - 3,
		(int)floorf((idleLength / modelScale - BASE_LENGTH - TIP_LENGTH) / SEGMENT_LENGTH + 0.5f)));
	float lengths[TentacleRopeCalc::MAX_POINTS] = {};
	lengths[0] = halfBase;
	for (int i = 0; i < segments; ++i) lengths[i + 1] = SEGMENT_LENGTH * modelScale;
	lengths[segments + 1] = TIP_LENGTH * modelScale;
	const bool changed = m_Rope.GetPointCount() != segments + 3 || m_RopeScale != modelScale;
	if (changed || dt <= 0.0f || Vec3Length(Vec3Sub(anchor, m_Rope.GetPoint(0))) > ROPE_RESET_DISTANCE) {
		const float yaw = GetPlayer().GetYaw();
		const float side = GetHand() == 0 ? -1.0f : 1.0f;
		const VEC3 outward{ cosf(yaw) * side * ROPE_RESET_OUTWARD, -1.0f, -sinf(yaw) * side * ROPE_RESET_OUTWARD };
		m_Rope.Reset(anchor, Vec3Scale(outward, 1.0f / Vec3Length(outward)), lengths, segments + 2);
		m_RopeScale = modelScale;
		return;
	}
	TENTACLE_ROPE_PARAMS params;
	params.Gravity = ROPE_GRAVITY;
	params.Damping = ROPE_DAMPING;
	params.AirDrag = ROPE_AIR_DRAG;
	params.BodyFollow = ROPE_BODY_FOLLOW;
	params.MaxBend = ROPE_MAX_BEND;
	params.MaxElbowBend = ROPE_MAX_ELBOW_BEND;
	params.MaxTipBend = ROPE_MAX_TIP_BEND;
	params.ShoulderForward = ROPE_SHOULDER_FORWARD;
	params.ShoulderBack = ROPE_SHOULDER_BACK;
	params.ShoulderOutward = ROPE_SHOULDER_OUTWARD;
	params.ShoulderInward = ROPE_SHOULDER_INWARD;
	VEC3 bodyRight;
	GetPlayer().GetUpperBodyAxes(params.BodyUp, params.BodyForward, bodyRight);
	params.BodyOutward = Vec3Scale(bodyRight, GetHand() == 0 ? -1.0f : 1.0f);
	params.Substeps = ROPE_SUBSTEPS;
	params.Iterations = ROPE_ITERATIONS;
	const XMFLOAT3 p = GetPlayer().GetPosition();
	const VEC3 foot{ p.x, p.y, p.z };
	const VEC3 toShoulder = Vec3Sub(origin, foot);
	// 体は足元から上半身の傾きに沿って肩の高さまで伸びる円柱。半径は肩の「横」の距離だけで決める
	// （前傾で肩が前へ出た分を入れると、後ろへ流れた腕を前へ押し戻してしまう）。
	params.BodyBottom = foot;
	params.BodyTop = Vec3Add(foot, Vec3Scale(params.BodyUp, Vec3Dot(toShoulder, params.BodyUp)));
	params.BodyRadius = fabsf(Vec3Dot(toShoulder, bodyRight)) * ROPE_BODY_RATE + ROPE_THICKNESS * modelScale;
	m_Rope.Step(anchor, params, dt);
}

void TentacleArm::Draw()
{
	const int hand = GetHand();
	const TentacleCalc& arm = m_Arm;
	const VEC3 origin = GetDrawShoulder();
	const VEC3 physicalOrigin = GetShoulder();
	// 回転中は腕の先を回転の円の上に置いて、伸ばしている腕と同じように描く。
	const bool spinning = m_Spin.GetPhase() == SPIN_SWING;
	const bool idle = arm.GetState() == TENTACLE_IDLE && !spinning;
	VEC3 tip = spinning ? GetSpinTip() : (idle ? physicalOrigin : arm.GetTip());
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
	if (idle) hanging = 1.0f;
	else if (!spinning && arm.GetState() != TENTACLE_HOLDING) {
		hanging = fmaxf(0.0f, 1.0f - Vec3Length(Vec3Sub(arm.GetTip(), physicalOrigin)) / idleLength);
	}
	// 付け根の箱は待機中は鎖の最初の節の向き（肩の可動域の中で振れる）。伸ばすほど、箱の真ん中を支点に先端の方へ回す。
	const float baseLength = BASE_LENGTH * modelScale;
	const VEC3 baseCenter = Vec3Add(origin, { 0.0f, -baseLength * 0.5f, 0.0f });
	const VEC3 hangAnchor = m_Rope.GetPoint(1); // ぶら下げたときの箱の下端
	const VEC3 hangDirection = Vec3Scale(Vec3Sub(hangAnchor, m_Rope.GetPoint(0)), 2.0f / baseLength);
	const VEC3 baseDirection = ArmCalcBlendDirection(hangDirection, Vec3Sub(tip, baseCenter), 1.0f - hanging);
	const VEC3 baseTop = Vec3Sub(baseCenter, Vec3Scale(baseDirection, baseLength * 0.5f));
	const VEC3 anchor = Vec3Add(baseCenter, Vec3Scale(baseDirection, baseLength * 0.5f)); // 回した箱の下端
	const VEC3 ropeShift = Vec3Sub(anchor, hangAnchor); // 鎖の形を回した箱の下端へ寄せる
	const float ropeTotal = baseLength * 0.5f + (idleLength - baseLength); // 鎖の全長（箱の下半分＋節と先っぽ）
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
	const XMFLOAT4 color = GetColor(arm.GetTarget() >= 0 || m_Pulling || !m_Spin.IsIdle());
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
		const float amplitude = idle ? 0.0f
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
				if (hanging > 0.0f) {
					// s は箱の下端からの割合なので、鎖の根元（肩）からの割合に直す。
					const VEC3 hangPoint = m_Rope.GetPointAt((baseLength * 0.5f + s * (idleLength - baseLength)) / ropeTotal);
					end = Vec3Add(Vec3Scale(end, 1.0f - hanging), Vec3Scale(Vec3Add(hangPoint, ropeShift), hanging));
				}
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

bool TentacleArm::CanTakeTap() const
{
	return m_Spin.IsIdle() && m_Arm.GetState() != TENTACLE_RETURNING;
}

bool TentacleArm::CanTakeHold() const
{
	return m_Spin.IsIdle() && m_Arm.GetState() == TENTACLE_IDLE;
}

bool TentacleArm::IsHolding() const
{
	return m_Arm.GetState() == TENTACLE_HOLDING;
}

bool TentacleArm::IsActive() const
{
	return m_Arm.GetState() != TENTACLE_IDLE || !m_Spin.IsIdle();
}
