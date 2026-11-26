#include "game/tentacle.h"
#include "audio/se.h"
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
// 単押しは3段：1段目でつかみ、2段目で足元へたたきつけ、3段目で投げる（押さなければ少し待って投げる）。
static constexpr float COMBO_COOLDOWN = 0.5f; // 投げてから、この腕で攻撃できない秒数。
static constexpr float GRAB_WINDOW = 1.2f; // つかんでから2段目の単押しを待つ秒数（過ぎたら投げる）。
static constexpr float SLAM_LIFT_SECONDS = 0.1f; // たたきつける前に持ち上げる秒数。
static constexpr float SLAM_IMPACT_SECONDS = 0.2f; // 2段目を始めてから地面に当たる秒数。
static constexpr float SLAM_SECONDS = 0.3f; // たたきつけの動きの秒数（この間の単押しは覚えておき、終わってから投げる）。
static constexpr float SLAM_WINDOW = 1.0f; // たたきつけ終わってから3段目の単押しを待つ秒数（過ぎたら投げる）。
// 投げは、いったん腕の側の後ろへ大きく引いて勢いをつけ、加速しながら前の斜め上へ振り抜いたところで離す。
static constexpr float WINDUP_SECONDS = 0.2f; // 後ろへ引く秒数。
static constexpr float WINDUP_ANGLE = XMConvertToRadians(160.0f); // 引ききった所の、正面から腕の側への角度。
static constexpr float WINDUP_DROP = 0.3f; // 引くときに肩の高さから下げる量。
static constexpr float SWING_SECONDS = 0.2f; // 引ききってから振り抜くまでの秒数（だんだん速くなる）。
static constexpr float SWING_END_ANGLE = XMConvertToRadians(-10.0f); // 振り抜いて離す所の角度（正面を少し越える）。
static constexpr float SWING_RADIUS = 1.5f; // 振り回す弧の、体の中心からの半径。
static constexpr float SWING_RISE = 1.0f; // 振り抜くまでに肩の高さから上げる量。
static constexpr float SWING_TIP_SPEED = 80.0f; // 振り回す間に、つかんだ相手が持つ位置へ追いつく速さ（遅れて弧が小さくならないように）。
static constexpr float THROW_UP = 1.2f; // 投げる向きの水平の単位方向に足す上向きの量（ナックルのアッパーのように斜め上へ）。
static constexpr float SLAM_LIFT = 1.3f; // 持ち上げる、肩からの高さ。
static constexpr float SLAM_FORWARD = 1.6f; // たたきつける所の、体からの前への距離。
static constexpr float SLAM_DAMAGE = 8.0f;
static constexpr float SLAM_HOLD_SECONDS = 10.0f; // 段階攻撃の間は計算の自動の解放を使わない（この秒数は届かない長さ）。
static constexpr float FINISH_THROW_DAMAGE = 6.0f; // たたきつけたあとの投げ。
static constexpr float FINISH_THROW_POWER = 2.0f;
static constexpr float SLAM_CLIP_SECONDS = 8.0f / 24.0f; // tentacle_slam の長さ（持ち上げ 0-3・たたきつけ 3-5・押さえる 5-8）
static constexpr float SLAM_RUMBLE_SMALL = 1.0f;
static constexpr float SLAM_RUMBLE_LARGE = 1.0f;
static constexpr float SLAM_RUMBLE_SECONDS = 0.3f;
static constexpr float GRIP_RANGE = 16.0f; // 足場のふちへ伸ばせる距離（上下も含む）
static constexpr float GRIP_HALF_ANGLE = XMConvertToRadians(40.0f); // 足場のふちを探す画面の正面の開き（見上げているときは上下も含む）
static constexpr float LOOK_UP_SINE = 0.09f; // カメラの向きの Y がこれより大きいと「見上げている」（約 5°）
static constexpr TENTACLE_PULL_PARAMS PULL_PARAMS{ 20.0f, 0.8f, 1.5f };
static constexpr float LANDING_INSET = 0.8f; // 足場のふちから内側へ寄せて着地する距離
static constexpr float SPIN_PHASE_SECONDS[] = { 0.19f, 0.4f, 0.15f }; // 溜め・回転・休み
static constexpr float SPIN_CLIP_SECONDS[] = { 6.0f / 24.0f, 10.0f / 24.0f, 5.0f / 24.0f };
static constexpr float REACH_CLIP_SECONDS = 6.0f / 24.0f;
static constexpr float THROW_CLIP_SECONDS[] = { 8.0f / 24.0f, 8.0f / 24.0f }; // 保持・投げ
static constexpr float THROW_ANIM_SECONDS = 0.45f; // 振り抜き始めてから投げ終わるまでの描画の長さ（振り抜く間も含む）
static constexpr float THROW_PHASE_SECONDS[] = { WINDUP_SECONDS, THROW_ANIM_SECONDS }; // 投げのクリップの、引く・振り抜く
static constexpr float SPIN_RADIUS = 3.5f;
static constexpr float SPIN_DAMAGE = 10.0f;
static constexpr float SPIN_POWER = 2.0f;
static constexpr float SPIN_UP = 0.6f; // 外への水平の単位方向に足す上向きの量
static constexpr float SPIN_HALF_HEIGHT = 1.5f; // 肩の高さから上下それぞれの当たる幅
static constexpr float SPIN_MARGIN = XMConvertToRadians(2.0f); // 1フレーム分の扇に足す角度（つなぎ目の取りこぼし防止）
static constexpr float SPIN_FULL_TURN = XMConvertToRadians(360.0f);
static constexpr float SPIN_START_ANGLE = XMConvertToRadians(45.0f); // 回し始めの、正面から腕の側への角度（左腕なら左前から右へ振る）
static constexpr float SPIN_RUMBLE_SMALL = 1.0f;
static constexpr float SPIN_RUMBLE_LARGE = 1.0f;
static constexpr float SPIN_RUMBLE_SECONDS = 0.25f;
static constexpr float GRAB_RUMBLE_SMALL = 0.8f;
static constexpr float GRAB_RUMBLE_LARGE = 0.6f;
static constexpr float GRAB_RUMBLE_SECONDS = 0.2f;
static constexpr float THROW_RUMBLE_SMALL = 1.0f;
static constexpr float THROW_RUMBLE_LARGE = 1.0f;
static constexpr float THROW_RUMBLE_SECONDS = 0.3f;
static constexpr float EXTEND_RUMBLE_START = 1.0f;	// 腕が伸び始めたときの振動の強さ（0〜1。両方のモーター）
static constexpr float EXTEND_RUMBLE_END = 1.0f;	// 腕が最大まで伸びたときの振動の強さ（0〜1。両方のモーター）
static constexpr float EXTEND_RUMBLE_SECONDS = 0.05f;	// 伸長中に毎フレーム足す振動の長さ（伸び終わるとこの秒数で止まる）
static constexpr float HOLD_OFFSET = 0.6f; // つかんだ相手を持つ、体の中心から腕の側への距離。
static constexpr float HOLD_FORWARD = 1.3f; // つかんだ相手を持つ、体の前への距離。
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
// しなり（描画だけ）：触手を、先端の動きに遅れてついてくる弓なりの曲線で描く。
static constexpr float BEND_LAG_SECONDS = 0.07f; // 先端の速さにこの秒数を掛けた分だけ、真ん中を動きと逆へ遅らせる。
static constexpr float BEND_SAG = 0.12f; // 長さに対する、真ん中の下へのたるみの割合。
static constexpr float BEND_OUTWARD = 0.1f; // 長さに対する、真ん中を腕の側へふくらませる割合。
static constexpr float BEND_MAX = 0.45f; // 長さに対する、真ん中をずらせる最大の割合。
static constexpr float BEND_RATE = 14.0f; // しなりが目標へ近づく速さ（1秒あたりの割合）。

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
	m_ClipState = TENTACLE_IDLE;
	m_StateClipElapsed = 0.0f;
	m_ThrowClipElapsed = -1.0f;
	m_GrabStep = 0;
	m_SwingFinish = false;
	m_ComboTime = 0.0f;
	m_Cooldown.Reset();
	m_Spin.Reset();
	StopPulling();
}

// 段階攻撃でつかんでいる間に、敵を置く位置（2段目は持ち上げてから足元へたたきつける）。
VEC3 TentacleArm::GetComboHoldPosition(const VEC3& hold, const VEC3& origin, int target) const
{
	const float side = GetHand() == 0 ? -1.0f : 1.0f;
	if (m_GrabStep == 3) {
		// 持っている所から腕の側の後ろへ下げながら引き、そこから加速して前の斜め上へ振り抜く。
		const float holdAngle = atan2f(HOLD_OFFSET, HOLD_FORWARD);
		const float holdRadius = sqrtf(HOLD_OFFSET * HOLD_OFFSET + HOLD_FORWARD * HOLD_FORWARD);
		float angle, radius, height;
		if (m_ComboTime < WINDUP_SECONDS) {
			const float u = m_ComboTime / WINDUP_SECONDS;
			const float e = u * u * (3.0f - 2.0f * u);
			angle = Lerp(holdAngle, WINDUP_ANGLE, e);
			radius = Lerp(holdRadius, SWING_RADIUS, e);
			height = origin.Y - WINDUP_DROP * e;
		}
		else {
			const float u = fminf(1.0f, (m_ComboTime - WINDUP_SECONDS) / SWING_SECONDS);
			const float e = u * u;
			angle = Lerp(WINDUP_ANGLE, SWING_END_ANGLE, e);
			radius = SWING_RADIUS;
			height = origin.Y - WINDUP_DROP + (WINDUP_DROP + SWING_RISE) * e;
		}
		const VEC3 around = AreaCalcRotateYaw(GetForward(), side * angle);
		const XMFLOAT3 p = GetPlayer().GetPosition();
		return { p.x + around.X * radius, height, p.z + around.Z * radius };
	}
	if (m_GrabStep != 2) return hold;
	if (m_ComboTime < SLAM_LIFT_SECONDS) return HoldPosition(side * HOLD_OFFSET * 0.5f, HOLD_FORWARD * 0.5f, origin.Y + SLAM_LIFT);
	const CAPSULE body = GetFighters().GetTarget(target, GetSlot()).Body;
	const float halfHeight = Vec3Length(Vec3Sub(body.End, body.Start)) * 0.5f + body.Radius;
	return HoldPosition(side * HOLD_OFFSET * 0.5f, SLAM_FORWARD, GetPlayer().GetPosition().y + halfHeight);
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
	// 足場のふちはカメラで狙うが、フリーカメラで足場がなければ体の向き（スティックの向き）へ伸ばす。
	const VEC3 reachForward = GetCamera().IsFreeLook() ? GetAim(false) : viewForward;
	aim = hasGrip ? Vec3Sub(grip.Point, origin) : (target >= 0 ? Vec3Sub(center, origin) : reachForward);
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
	// 長押しは押している間ためた姿勢で待ち、離した瞬間に回す。
	if (m_Spin.IsIdle() && (input.HoldStarted || input.HoldReleased) && m_Arm.GetState() == TENTACLE_IDLE) {
		m_SpinForward = GetAim(false);
		m_SpinAngle = 0.0f;
		m_SpinHits.Clear();
		m_SpinConsumed = false;
		m_Spin.Start(SPIN_CHARGE);
		m_Spin.SetHeld(true);
		if (input.HoldReleased) m_Spin.Release();
		else PlaySe(SE_ARM_HOLD);
		m_ThrowClipElapsed = -1.0f;
	}
	else if (input.HoldReleased) m_Spin.Release();
	// ためている間にボタンを受け付けなくなった（ガードなど）ときは、出さずにやめる。
	else if (!input.Pressed && m_Spin.IsHeld()) m_Spin.Reset();
	// ためている間は狙いを追い、離した瞬間の向きから回す。
	if (m_Spin.GetPhase() == SPIN_CHARGE) m_SpinForward = GetAim(false);
	const ARM_PHASE_STEP step = m_Spin.Step(dt, SPIN_PHASE_SECONDS, SPIN_PHASE_COUNT);
	// 振り回しは、空振りでも当たっても「振り回したときのヒット音」を回し始めに1回（当たったときに重ねない。No.195）。
	if (step.Entered == SPIN_SWING) PlaySe(SE_TENTACLE_SWING_HIT);
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
	const auto inside = [&](const CAPSULE& body) {
		return AreaCalcInSector(center, middle, 0.0f, SPIN_RADIUS, halfAngle, SPIN_HALF_HEIGHT, body);
	};
	StrikePillarsOutward(inside, SPIN_DAMAGE, center);
	for (int i = 0; i < FIGHTER_COUNT; ++i) {
		const TENTACLE_TARGET target = GetFighters().GetMeleeTarget(i, GetSlot());
		if (!target.Available || m_SpinHits.Contains(i) || !inside(target.Body)) continue;
		m_SpinHits.TryAdd(i);
		const bool applied = GetFighters().Launch(i, GetSlot(), AreaCalcRadialDirection(center, target.Body, SPIN_UP, middle), SPIN_DAMAGE, SPIN_POWER);
		if (applied && !m_SpinConsumed) {
			ConsumeDurability(GetCost(COST_ROW, ARM_COST_HOLD_HIT));
			m_SpinConsumed = true;
		}
		hit = true;
	}
	m_SpinAngle = angle;
	if (hit) {
		Rumble(SPIN_RUMBLE_SMALL, SPIN_RUMBLE_LARGE, SPIN_RUMBLE_SECONDS);
	}
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
	const float dt = GameTimeGetDeltaTime();
	m_Cooldown.Step(dt);
	// 回転の間は単押しで伸ばさない。
	bool trigger = input.Tap && m_Spin.IsIdle();
	// つかんでいる間は段階攻撃：2段目の単押しでたたきつけ、3段目の単押しで投げる。押さなければ待ってから投げる。
	if (arm.GetState() == TENTACLE_HOLDING && m_GrabStep > 0) {
		m_ComboTime += fmaxf(0.0f, dt);
		bool swing = false;
		bool throwNow = false;
		if (m_GrabStep == 3) throwNow = m_ComboTime >= WINDUP_SECONDS + SWING_SECONDS; // 振り抜いたら離す。
		else if (m_GrabStep == 1) {
			if (trigger) {
				m_GrabStep = 2;
				m_ComboTime = 0.0f;
				m_SlamHit = false;
			}
			else swing = m_ComboTime >= GRAB_WINDOW;
		}
		else {
			if (!m_SlamHit && m_ComboTime >= SLAM_IMPACT_SECONDS) {
				m_SlamHit = true;
				GetFighters().AddDamage(arm.GetTarget(), GetSlot(), SLAM_DAMAGE);
				Rumble(SLAM_RUMBLE_SMALL, SLAM_RUMBLE_LARGE, SLAM_RUMBLE_SECONDS);
				PlayHitSe(SE_TENTACLE_SLAM, arm.GetTarget());
			}
			swing = (trigger && m_ComboTime >= SLAM_SECONDS) || m_ComboTime >= SLAM_SECONDS + SLAM_WINDOW;
			m_SwingFinish = swing;
		}
		// 投げると決めたら、すぐ離さずに振り回す。
		if (swing) {
			m_GrabStep = 3;
			m_ComboTime = 0.0f;
			PlaySe(SE_TENTACLE_SWING_HIT); // つかんだ相手を振り回すのも、回転と同じ音
		}
		// 離す瞬間は、体のクリップの振り抜きを途中から続ける。
		if (throwNow) m_ThrowClipElapsed = m_ComboTime - WINDUP_SECONDS;
		trigger = throwNow;
	}
	if (trigger && arm.GetState() == TENTACLE_IDLE) {
		m_ReachPending = true;
		m_ThrowClipElapsed = -1.0f;
		StartReach(targets, origin, cameraForward, aim, grip, hasGrip);
		PlaySe(SE_TENTACLE_SWING); // 伸ばすときは「振り回したとき」の音（No.195。伸ばしたとき.wav は表に残す）
	}
	const bool cancel = GetPlayer().DidRespawn() || (arm.GetTarget() >= 0 && !GetFighters().IsHeldBy(arm.GetTarget(), GetSlot()));
	// 段階攻撃は腕のほうで投げる時を決めるので、計算の自動の投げ・解放は使わない。
	TENTACLE_PARAMS params = PARAMS;
	params.ThrowSeconds = 0.0f;
	params.HoldSeconds = SLAM_HOLD_SECONDS;
	if (m_GrabStep == 3) params.ReturnSpeed = SWING_TIP_SPEED;
	const VEC3 comboHold = arm.GetTarget() >= 0 ? GetComboHoldPosition(hold, origin, arm.GetTarget()) : hold;
	const TENTACLE_EVENT event = arm.Step(origin, aim, comboHold, trigger, cancel,
		dt, params, targets, FIGHTER_COUNT, GetStage().GetStage(), hasGrip ? &grip : nullptr, CanAnchor());
	if (event.Grabbed >= 0) {
		if (GetFighters().Grab(event.Grabbed, GetSlot())) {
			m_GrabStep = 1;
			m_ComboTime = 0.0f;
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
		// フリーカメラでは、投げる瞬間のスティックの向き（なければ体の向き）へ投げる。
		const float throwYaw = GetCamera().IsFreeLook() ? GetPlayer().GetFreeAimYaw(yaw) : yaw;
		// 斜め上へ投げる（ナックルのアッパーと同じく、水平の向きに上向きを足す）。
		const VEC3 throwForward = AreaCalcLaunchDirection({ sinf(throwYaw), 0.0f, cosf(throwYaw) }, THROW_UP);
		GetFighters().Release(event.Released, GetSlot(), event.Throw, throwForward,
			m_SwingFinish ? FINISH_THROW_DAMAGE : THROW_DAMAGE, m_SwingFinish ? FINISH_THROW_POWER : THROW_POWER);
		if (event.Throw) Rumble(THROW_RUMBLE_SMALL, THROW_RUMBLE_LARGE, THROW_RUMBLE_SECONDS);
		// 投げたら（段階攻撃の終わり）、しばらくこの腕で攻撃できない（続けてつかみ続けられないように）。
		if (event.Throw) m_Cooldown.StartCooldown(COMBO_COOLDOWN);
	}
	// 判定の状態は変えず、描画用の経過秒だけを数える。投げたフレームは後半の先頭を見せる。
	const float clipDt = fmaxf(0.0f, dt);
	const TENTACLE_STATE state = arm.GetState();
	if (state != TENTACLE_HOLDING) m_GrabStep = 0;
	m_StateClipElapsed = state == m_ClipState ? m_StateClipElapsed + clipDt : 0.0f;
	m_ClipState = state;
	if (state != TENTACLE_HOLDING) m_SwingFinish = false;
	if (cancel) m_ThrowClipElapsed = -1.0f;
	else if (event.Throw && m_ThrowClipElapsed < 0.0f) m_ThrowClipElapsed = 0.0f;
	else if (m_ThrowClipElapsed >= 0.0f) {
		m_ThrowClipElapsed += clipDt;
		if (m_ThrowClipElapsed >= THROW_ANIM_SECONDS) m_ThrowClipElapsed = -1.0f;
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

// 触手の真ん中のずれ（しなり）を1フレームに1回だけ進めて返す。先端の動きと逆へ遅れ、少したるんで腕の側へふくらむ。
VEC3 TentacleArm::StepBend(const VEC3& anchor, const VEC3& tip, const VEC3& right)
{
	const float now = GameTimeGetElapsed();
	if (now == m_BendTime) return m_Bend;
	const float dt = m_BendTime < 0.0f ? 0.0f : now - m_BendTime;
	m_BendTime = now;
	const VEC3 tipFromAnchor = Vec3Sub(tip, anchor);
	const float length = Vec3Length(tipFromAnchor);
	if (dt > 0.0f && m_HasBendTip) {
		const VEC3 velocity = Vec3Scale(Vec3Sub(tipFromAnchor, m_BendTip), 1.0f / dt);
		const float side = GetHand() == 0 ? -1.0f : 1.0f;
		VEC3 target = Vec3Add(Vec3Scale(velocity, -BEND_LAG_SECONDS),
			Vec3Add(VEC3{ 0.0f, -BEND_SAG * length, 0.0f }, Vec3Scale(right, side * BEND_OUTWARD * length)));
		const float most = BEND_MAX * length;
		const float size = Vec3Length(target);
		if (size > most && size > MODEL_MIN_LENGTH) target = Vec3Scale(target, most / size);
		m_Bend = Vec3Add(m_Bend, Vec3Scale(Vec3Sub(target, m_Bend), fminf(1.0f, BEND_RATE * dt)));
	}
	m_BendTip = tipFromAnchor;
	m_HasBendTip = true;
	return m_Bend;
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
		const VEC3 bend = idle ? VEC3{} : StepBend(anchor, drawTip, right);
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
				// 弓なりの曲線（2次ベジェ）に、まっすぐな線からの揺れの分を足す。
				const VEC3 straight = Vec3Add(anchor, Vec3Scale(Vec3Sub(drawTip, anchor), s));
				const VEC3 wiggle = Vec3Sub(TentacleCalcWigglePoint(anchor, drawTip, s, phase, amplitude, WIGGLE_WAVES), straight);
				end = Vec3Add(Vec3Add(straight, Vec3Scale(bend, 4.0f * s * (1.0f - s))), wiggle);
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
	// たたきつけの動きの間の単押しは覚えておき、終わってから投げる。
	if ((m_GrabStep == 2 && m_ComboTime < SLAM_SECONDS) || m_GrabStep == 3) return false;
	// 投げたあとの攻撃できない間は、新しく伸ばさない（足場に付いて引っぱっている間の取りやめは受け付ける）。
	if (m_Cooldown.IsCoolingDown() && m_Arm.GetState() == TENTACLE_IDLE) return false;
	return m_Spin.IsIdle() && m_Arm.GetState() != TENTACLE_RETURNING;
}

bool TentacleArm::CanTakeHold() const
{
	return m_Spin.IsIdle() && m_Arm.GetState() == TENTACLE_IDLE && !m_Cooldown.IsCoolingDown();
}

ARM_BODY_CLIP TentacleArm::GetBodyClip() const
{
	if (!m_Spin.IsIdle()) {
		return { true, ATTACK_CLIP::TentacleSpin,
			ArmCalcClipTime(m_Spin.GetPhase(), m_Spin.GetTime(), SPIN_PHASE_SECONDS, SPIN_CLIP_SECONDS), true };
	}
	if (m_ThrowClipElapsed >= 0.0f) {
		return { true, ATTACK_CLIP::TentacleThrow,
			ArmCalcClipTime(1, m_ThrowClipElapsed, THROW_PHASE_SECONDS, THROW_CLIP_SECONDS), false };
	}
	if (m_Arm.GetState() == TENTACLE_HOLDING && m_GrabStep == 3) {
		// 後ろへ引く間はクリップの引き（0-8）、振り抜く間は振り抜き（8-16）。
		const bool windup = m_ComboTime < WINDUP_SECONDS;
		return { true, ATTACK_CLIP::TentacleThrow, ArmCalcClipTime(windup ? 0 : 1, windup ? m_ComboTime : m_ComboTime - WINDUP_SECONDS,
			THROW_PHASE_SECONDS, THROW_CLIP_SECONDS), false };
	}
	if (m_Arm.GetState() == TENTACLE_HOLDING && m_GrabStep == 2) {
		return { true, ATTACK_CLIP::TentacleSlam, fminf(m_ComboTime / SLAM_SECONDS, 1.0f) * SLAM_CLIP_SECONDS, false };
	}
	if (m_Arm.GetState() == TENTACLE_HOLDING) {
		// つかんでいる間は前へ差し出した形のまま（投げるときに後ろへ引く）。
		return { true, ATTACK_CLIP::TentacleThrow, 0.0f, false };
	}
	if (m_Arm.GetState() == TENTACLE_EXTENDING || m_Arm.GetState() == TENTACLE_PULLING) {
		return { true, ATTACK_CLIP::TentacleReach, fminf(m_StateClipElapsed, REACH_CLIP_SECONDS), false };
	}
	return {};
}

bool TentacleArm::IsHolding() const
{
	return m_Arm.GetState() == TENTACLE_HOLDING;
}

bool TentacleArm::IsActive() const
{
	return m_Arm.GetState() != TENTACLE_IDLE || !m_Spin.IsIdle();
}
