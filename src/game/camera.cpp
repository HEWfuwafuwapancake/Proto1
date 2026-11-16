#include "core/gametime.h"
#include "core/shader.h"
#include "game/camera.h"
#include "game/fighter.h"
#include "game/match.h"
#include "game/stage.h"
#include "game/fightermanager.h"
#include "game/arm.h"
#include <limits>
#include "input/action.h"
#include "physics/cameracalc.h"


//------------------------------------------------------------------------------
//	調整用の値
//------------------------------------------------------------------------------
// マウス感度・矢印キー・右スティックで回る速さは action.cpp の「調整用の値」にある
static constexpr float FOLLOW_DISTANCE = 4.5f;					// 見ている点からカメラまでの距離
static constexpr float TARGET_HEIGHT_RATE = 0.8f;					// 見ている点の高さ（プレイヤーの高さに対する割合。0.8 で頭のあたり）
static constexpr float YAW_SMOOTH_TIME = 0.12f;						// 左右の角度が目標へ追いつくまでのおよその秒数
static constexpr float PITCH_SMOOTH_TIME = 0.12f;					// 上下の角度が目標へ追いつくまでのおよその秒数
static constexpr float DISTANCE_SMOOTH_TIME = 0.3f;					// 障害物から離れたとき、元の距離へ戻るまでのおよその秒数
static constexpr float LOOK_SMOOTH_TIME = 0.15f;						// 見ている点の左右（XZ）が追いつくまでのおよその秒数
static constexpr float LOOK_HEIGHT_SMOOTH_TIME = 0.15f;				// 見ている点の高さが追いつくまでのおよその秒数
static constexpr float LOOK_HEIGHT_DEADZONE_UP = 0.6f;				// 空中で、見ている点の高さの目標より上にこれだけ離れたら追い始める（小さいジャンプでは揺らさない）
static constexpr float LOOK_HEIGHT_DEADZONE_DOWN = 0.3f;			// 空中で、見ている点の高さの目標より下にこれだけ離れたら追い始める
static constexpr float LOOK_DEADZONE = 0.3f;							// 見ている点の左右（XZ）の遊び。この距離を超えた分だけ目標を動かす
static constexpr float FOLLOW_TURN_SPEED = XMConvertToRadians(90.0f);	// 横移動について回る速さ（1秒に回る角度。入力の大きさに比例）
static constexpr float MANUAL_PAUSE = 0.5f;							// 自分でカメラを回したあと、横移動について回るのを止める秒数
static constexpr float STAGE_MARGIN = 0.3f;							// 浮島の面から、カメラをこれだけ離す（めり込んで見えないように）
static constexpr float PITCH_MIN = XMConvertToRadians(-35.0f);		// いちばん下を向いたとき（上から見下ろす）
static constexpr float PITCH_MAX = XMConvertToRadians(50.0f);		// いちばん上を向いたとき（見上げる。高台を狙える）
static constexpr float START_PITCH = XMConvertToRadians(-12.0f);	// 最初の上下の向き（少し見下ろす）
static constexpr float SWITCH_SMOOTH_TIME = 0.06f;					// 切り替え直後の角度のバネ
static constexpr float SWITCH_TIME = 0.3f;							// 角度のバネを速める秒数
static constexpr float TARGET_POINT_RATE = 0.25f;					// 見ている点を敵へずらす割合
static constexpr float TARGET_SCREEN_MARGIN = 0.15f;				// 画面の端に残す余白
static constexpr float RESPAWN_LOOK_SMOOTH_TIME = 0.35f;				// 復活を待つ間、見ている点が浮島の高さ（場外にした相手）へ戻るまでのおよその秒数
static constexpr float RESPAWN_DISTANCE = 7.0f;						// 復活を待つ間、見ている点からカメラまでの距離（少し引いて見渡す）
static constexpr float TARGET_FALL_MARGIN = 0.1f;					// 敵の足元が浮島の上面からこれだけ下へ落ちたら、ターゲットから外す（追いかけない）
static constexpr float SHOULDER_OFFSET = 0.9f;						// ロックオン中にカメラを右へ寄せる量（キャラは画面の左へ寄る。AC6 風の肩越し）
static constexpr float STRAFE_DRIFT = 1.2f;							// 左右へいっぱいに動いたとき、キャラを動く向きへ画面の端へ流す量
static constexpr float SIDE_SMOOTH_TIME = 0.35f;						// 横へのずれが目標へ追いつくまでのおよその秒数
static constexpr float SOFT_SCREEN_MARGIN = 0.05f;					// ソフトロック：画面の端からこの割合の内側に見えている相手だけを狙う
static constexpr float SOFT_MAX_DISTANCE = 40.0f;					// ソフトロック：これより遠い相手は狙わない
static constexpr float SOFT_KEEP_BONUS = 0.25f;						// ソフトロック：今狙っている相手を優先する量（画面の半分の幅を1とした距離。ちらつき防止）

static constexpr float FOV = XMConvertToRadians(60.0f);				// 視野角（縦）
static constexpr float NEAR_Z = 0.1f;								// これより近いものは描かない
static constexpr float FAR_Z = 1000.0f;								// これより遠いものは描かない


// プレイヤーの頭のあたり（見ている点の目標）
VEC3 Camera::PlayerHead(void) const
{
	const XMFLOAT3 feet = m_Player->GetPosition();
	return { feet.x, feet.y + m_Player->GetHeight() * TARGET_HEIGHT_RATE, feet.z };
}

// 生存・浮島より下への落下・保持状態を候補へ反映する。配列の添字とスロット番号は同じ（自分は候補外）。
// 浮島の上面より下へ落ちた敵は候補から外し、カメラが下まで追いかけないようにする。
void Camera::CollectTargets(CAMERA_FOCUS_TARGET* targets) const
{
	for (int i = 0; i < FIGHTER_COUNT; ++i) {
		const TENTACLE_TARGET dummy = m_Fighters->GetTarget(i, m_Slot);
		targets[i].Position = Vec3Scale(Vec3Add(dummy.Body.Start, dummy.Body.End), 0.5f);
		const float feetY = dummy.Body.Start.Y - dummy.Body.Radius;
		// スモークの中で隠れている相手もロックオンしない（No.75）。
		targets[i].Available = dummy.Available && feetY >= m_Stage->GetStage().TopY - TARGET_FALL_MARGIN && !m_Fighters->IsHidden(i);
	}
}

bool Camera::IsTargetAvailable(int target, const CAMERA_FOCUS_TARGET* targets) const
{
	return target >= 0 && target < FIGHTER_COUNT && targets[target].Available;
}

// 角度は全周・距離は無制限。現在の敵への優先ボーナスも付けない。
int Camera::PickFrontTarget(const CAMERA_FOCUS_TARGET* targets) const
{
	return CameraCalcPickFocus(m_Yaw, PlayerHead(), targets, FIGHTER_COUNT, -1,
		XM_PI, (std::numeric_limits<float>::infinity)(), 0.0f);
}

VEC3 Camera::LockFocusPoint(const VEC3& player, const CAMERA_FOCUS_TARGET* targets) const
{
	// 身長を半径にすれば、頭付近を中心に体全体を安全側で囲める。
	return CameraCalcFocusPoint(player, targets[m_LockTarget].Position, m_Yaw, m_Pitch,
		m_Distance, FOV, m_ViewWidth / m_ViewHeight,
		TARGET_SCREEN_MARGIN, m_Player->GetHeight(), TARGET_POINT_RATE);
}

// 画面に映っていて、浮島・高台にさえぎられていない相手のうち、画面の真ん中にいちばん近い相手。
int Camera::PickSoftTarget(const CAMERA_FOCUS_TARGET* targets) const
{
	CAMERA_FOCUS_TARGET visible[FIGHTER_COUNT];
	const VEC3 eye{ m_Position.x, m_Position.y, m_Position.z };
	for (int i = 0; i < FIGHTER_COUNT; ++i) {
		visible[i] = targets[i];
		if (visible[i].Available && StageCalcIsBlocked(m_Stage->GetStage(), eye, targets[i].Position)) visible[i].Available = false;
	}
	return CameraCalcPickVisible(eye, m_Yaw, m_Pitch, FOV, m_ViewWidth / m_ViewHeight, SOFT_SCREEN_MARGIN,
		SOFT_MAX_DISTANCE, visible, FIGHTER_COUNT, m_LockTarget, SOFT_KEEP_BONUS);
}

void Camera::UpdateLockTarget(const CAMERA_FOCUS_TARGET* targets)
{
	const bool holding = m_Arms->IsHolding();
	if (m_Player->IsTrigger(ACTION_CAMERA_TOGGLE)) {
		m_Mode = CameraCalcNextMode(m_Mode);
		m_ModeSeconds = 0.0f;
		m_SavedTarget = -1;
		// ハードロックをやめるときは、今の角度から自由に回せるようにする。戻すときは下の処理が正面の相手を選び直す。
		if (m_Mode != CAMERA_MODE_HARD) {
			m_LockTarget = -1;
			m_YawGoal = m_Yaw;
			m_SwitchRemaining = 0.0f;
		}
	}
	// ソフトロックは毎フレーム、画面に見えている相手を選び直す（カメラは動かさない）。フリーは狙わない。
	if (m_Mode != CAMERA_MODE_HARD) {
		m_LockTarget = (m_Mode == CAMERA_MODE_SOFT && !holding) ? PickSoftTarget(targets) : -1;
		m_WasHolding = holding;
		return;
	}
	if (holding) {
		if (!m_WasHolding) {
			m_SavedTarget = m_LockTarget;
			m_LockTarget = -1;
			m_YawGoal = m_Yaw;
			m_SwitchRemaining = 0.0f;
		}
	}
	else {
		int next = m_LockTarget;
		if (m_WasHolding) {
			next = IsTargetAvailable(m_SavedTarget, targets) ? m_SavedTarget : PickFrontTarget(targets);
			m_SavedTarget = -1;
		}
		else if (!IsTargetAvailable(next, targets)) {
			next = PickFrontTarget(targets);
		}
		else {
			const int direction = (m_Player->IsTrigger(ACTION_TARGET_RIGHT) ? 1 : 0)
				- (m_Player->IsTrigger(ACTION_TARGET_LEFT) ? 1 : 0);
			next = CameraCalcNextTarget(PlayerHead(), targets, FIGHTER_COUNT, next, direction);
		}
		if (next != m_LockTarget) {
			m_LockTarget = next;
			m_SwitchRemaining = next >= 0 ? SWITCH_TIME : 0.0f;
			// 自由カメラへ移るときも、角度の速度は持ち越す。
			if (next < 0) m_YawGoal = m_Yaw;
		}
	}
	m_WasHolding = holding;
}

// 復活を待つ間は、落ちた下から浮島の高さへ戻し、場外にした相手（いなければ浮島の中心）を映す。
void Camera::UpdateWaiting(float dt)
{
	const STAGE& stage = m_Stage->GetStage();
	VEC3 focus{ stage.CenterX, stage.TopY + m_Player->GetHeight() * TARGET_HEIGHT_RATE, stage.CenterZ };
	const int killer = m_Player->GetKoBy();
	if (killer >= 0 && m_Fighters->IsActive(killer)) {
		const XMFLOAT3 feet = m_Fighters->GetFighter(killer).GetPosition();
		focus = { feet.x, feet.y + m_Fighters->GetFighter(killer).GetHeight() * TARGET_HEIGHT_RATE, feet.z };
	}
	m_PitchGoal = START_PITCH;
	m_Pitch = CameraCalcSmoothDamp(m_Pitch, m_PitchGoal, m_PitchVelocity, PITCH_SMOOTH_TIME, dt);
	m_Target.X = CameraCalcSmoothDamp(m_Target.X, focus.X, m_TargetVelocity.X, RESPAWN_LOOK_SMOOTH_TIME, dt);
	m_Target.Y = CameraCalcSmoothDamp(m_Target.Y, focus.Y, m_TargetVelocity.Y, RESPAWN_LOOK_SMOOTH_TIME, dt);
	m_Target.Z = CameraCalcSmoothDamp(m_Target.Z, focus.Z, m_TargetVelocity.Z, RESPAWN_LOOK_SMOOTH_TIME, dt);
	m_TargetGoal = focus;
	m_Distance = CameraCalcSmoothDamp(m_Distance, RESPAWN_DISTANCE, m_DistanceVelocity, RESPAWN_LOOK_SMOOTH_TIME, dt);
	m_SideOffset = CameraCalcSmoothDamp(m_SideOffset, 0.0f, m_SideVelocity, SIDE_SMOOTH_TIME, dt);
	const VEC3 pos = CameraCalcOrbitPosition(CameraCalcSideShift(m_Target, m_Yaw, m_SideOffset), m_Yaw, m_Pitch, m_Distance);
	m_Position = { pos.X, pos.Y, pos.Z };
}

// 見ている点のまわりの、向きと距離で決まる位置に置き、浮島にめり込むなら手前まで近づける
// 見ている点ごとカメラを横へずらすので、向きは変えずにキャラが画面の横へ寄る。
void Camera::PlaceCamera(float dt)
{
	const VEC3 pivot = CameraCalcSideShift(m_Target, m_Yaw, m_SideOffset);
	const VEC3 orbit = CameraCalcOrbitPosition(pivot, m_Yaw, m_Pitch, FOLLOW_DISTANCE);
	const VEC3 clipped = CameraCalcClipToStage(pivot, orbit, m_Stage->GetStage(), STAGE_MARGIN);
	const float distanceGoal = Vec3Length(Vec3Sub(clipped, pivot));
	if (distanceGoal < m_Distance) {
		// 浮島にぶつかるときは即座に近づけ、離れる向きの速度も消す。
		m_Distance = distanceGoal;
		m_DistanceVelocity = 0.0f;
	}
	else {
		m_Distance = CameraCalcSmoothDamp(m_Distance, distanceGoal, m_DistanceVelocity, DISTANCE_SMOOTH_TIME, dt);
	}
	const VEC3 pos = CameraCalcOrbitPosition(pivot, m_Yaw, m_Pitch, m_Distance);
	m_Position = { pos.X, pos.Y, pos.Z };
}

// やり直し・復活では目標へ瞬時に合わせ、すべてのバネの速度を消す。
void Camera::SnapToPlayer(void)
{
	m_Yaw = m_YawGoal;
	m_Pitch = m_PitchGoal;
	CAMERA_FOCUS_TARGET targets[FIGHTER_COUNT];
	CollectTargets(targets);
	// ソフトロックの相手は次の Update で画面から選ぶ。
	m_LockTarget = m_Mode != CAMERA_MODE_HARD ? -1 : PickFrontTarget(targets);
	m_SavedTarget = -1;
	m_WasHolding = false;
	m_SwitchRemaining = 0.0f;
	if (m_LockTarget >= 0) {
		m_Yaw = m_YawGoal = CameraCalcYawTo(PlayerHead(), targets[m_LockTarget].Position, m_Yaw);
	}
	m_YawVelocity = 0.0f;
	m_PitchVelocity = 0.0f;
	m_PlayerGoal = PlayerHead();
	m_TargetVelocity = {};
	m_Distance = FOLLOW_DISTANCE;
	m_Target = m_TargetGoal = m_LockTarget >= 0 ? LockFocusPoint(m_PlayerGoal, targets) : m_PlayerGoal;
	m_DistanceVelocity = 0.0f;
	m_ManualPause = 0.0f;
	m_SideOffset = CameraCalcSideOffsetGoal(m_LockTarget >= 0, 0.0f, SHOULDER_OFFSET, STRAFE_DRIFT);
	m_SideVelocity = 0.0f;
	PlaceCamera(0.0f);
}

void Camera::Reset(void)
{
	m_YawGoal = 0.0f;
	m_PitchGoal = START_PITCH;
	SnapToPlayer();
}

void Camera::Finalize(void)
{

}

void Camera::Update(void)
{
	if (m_Player->DidRespawn()) {
		SnapToPlayer();
		return;
	}
	if (!m_Player->IsActive()) {
		UpdateWaiting(GameTimeGetDeltaTime());
		return;
	}
	const float dt = GameTimeGetDeltaTime();
	CAMERA_FOCUS_TARGET targets[FIGHTER_COUNT];
	CollectTargets(targets);
	m_ModeSeconds = fminf(MODE_SHOW_NEVER, m_ModeSeconds + dt);
	UpdateLockTarget(targets);
	// カメラが相手を向き続けるのはハードロックだけ（ソフトロックは狙うだけで、カメラは自由に回せる）。
	const bool hardLock = m_Mode == CAMERA_MODE_HARD && m_LockTarget >= 0;
	// 手動入力も横移動の sway も、現在角ではなく目標角へ足す。
	const ACTION_AXIS look = m_Player->GetLookInput();
	if (hardLock) {
		m_YawGoal = CameraCalcYawTo(PlayerHead(), targets[m_LockTarget].Position, m_YawGoal);
	}
	else {
		m_YawGoal += look.X;
	}
	m_PitchGoal += look.Y;
	if (m_PitchGoal > PITCH_MAX) m_PitchGoal = PITCH_MAX;
	if (m_PitchGoal < PITCH_MIN) m_PitchGoal = PITCH_MIN;
	if (look.X != 0.0f || look.Y != 0.0f) {
		m_ManualPause = MANUAL_PAUSE;
	}
	else if (m_ManualPause > 0.0f) {
		m_ManualPause = fmaxf(0.0f, m_ManualPause - dt);
	}
	else if (m_LockTarget < 0 && m_Mode == CAMERA_MODE_HARD && m_Match->IsPlaying()) {
		// 腕で体が動けない間（ガンランスで突き刺している間など）は、移動の入力で視界を回さない。
		m_YawGoal += m_Player->GetMoveInput().X * m_Arms->GetMoveScale() * FOLLOW_TURN_SPEED * dt;
	}
	// 長く回し続けても、角度が大きくなり続けないようにする。
	m_YawGoal = atan2f(sinf(m_YawGoal), cosf(m_YawGoal));
	const bool switching = m_SwitchRemaining > 0.0f;
	m_Yaw = CameraCalcSmoothDampAngle(m_Yaw, m_YawGoal, m_YawVelocity,
		switching ? SWITCH_SMOOTH_TIME : YAW_SMOOTH_TIME, dt);
	m_Pitch = CameraCalcSmoothDamp(m_Pitch, m_PitchGoal, m_PitchVelocity,
		switching ? SWITCH_SMOOTH_TIME : PITCH_SMOOTH_TIME, dt);
	m_SwitchRemaining = fmaxf(0.0f, m_SwitchRemaining - dt);

	// 左右は円形の遊びを超えた分だけ目標を動かす（高さとは分ける）。
	const VEC3 head = PlayerHead();
	const VEC3 horizontal = { head.X - m_PlayerGoal.X, 0.0f, head.Z - m_PlayerGoal.Z };
	const float length = Vec3Length(horizontal);
	if (length > LOOK_DEADZONE) {
		m_PlayerGoal = Vec3Add(m_PlayerGoal, Vec3Scale(horizontal, (length - LOOK_DEADZONE) / length));
	}
	// 地上と、浮島の上面より下へ落ちたときは高さをそのまま追う。
	// 空中では上下の遊びを超えた分だけ追う（高く飛んでも頭が画面から外れないように）。
	if (m_Player->IsOnGround() || m_Player->GetPosition().y < m_Stage->GetStage().TopY) {
		m_PlayerGoal.Y = head.Y;
	}
	else if (head.Y > m_PlayerGoal.Y + LOOK_HEIGHT_DEADZONE_UP) {
		m_PlayerGoal.Y = head.Y - LOOK_HEIGHT_DEADZONE_UP;
	}
	else if (head.Y < m_PlayerGoal.Y - LOOK_HEIGHT_DEADZONE_DOWN) {
		m_PlayerGoal.Y = head.Y + LOOK_HEIGHT_DEADZONE_DOWN;
	}
	const VEC3 lockHead{ head.X, m_PlayerGoal.Y, head.Z };
	m_TargetGoal = hardLock ? LockFocusPoint(lockHead, targets) : m_PlayerGoal;
	m_Target.X = CameraCalcSmoothDamp(m_Target.X, m_TargetGoal.X, m_TargetVelocity.X, LOOK_SMOOTH_TIME, dt);
	m_Target.Z = CameraCalcSmoothDamp(m_Target.Z, m_TargetGoal.Z, m_TargetVelocity.Z, LOOK_SMOOTH_TIME, dt);
	m_Target.Y = CameraCalcSmoothDamp(m_Target.Y, m_TargetGoal.Y, m_TargetVelocity.Y, LOOK_HEIGHT_SMOOTH_TIME, dt);
	// ハードロック中は肩越しに横へ寄せ、ソフトロック・フリー・相手がいないときは真ん中。左右へ動く間はキャラを画面の端へ流す。
	const float moveX = m_Match->IsPlaying() ? m_Player->GetMoveInput().X * m_Arms->GetMoveScale() : 0.0f;
	const float sideGoal = CameraCalcSideOffsetGoal(m_Mode == CAMERA_MODE_HARD && GetTarget() >= 0, moveX, SHOULDER_OFFSET, STRAFE_DRIFT);
	m_SideOffset = CameraCalcSmoothDamp(m_SideOffset, sideGoal, m_SideVelocity, SIDE_SMOOTH_TIME, dt);
	PlaceCamera(dt);
}

// ビュー行列とプロジェクション行列を作る（描画と、画面の位置の計算で同じものを使う）
void Camera::MakeMatrices(XMMATRIX* view, XMMATRIX* projection) const
{
	// ビュー行列（カメラの位置と、見ている向き）
	// カメラは見ている点から向きの反対に置いてあるので、この向きで見れば見ている点が画面の真ん中に来る
	// （浮島に近づけてカメラが見ている点にほぼ重なっても、向きは決まる）
	XMVECTOR eye = XMLoadFloat3(&m_Position);
	const XMFLOAT3 direction = GetDirection();
	XMVECTOR dir = XMLoadFloat3(&direction);
	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	*view = XMMatrixLookToLH(eye, dir, up);

	// プロジェクション行列（遠くのものほど小さく見える透視投影）
	float aspect = m_ViewWidth / m_ViewHeight;
	*projection = XMMatrixPerspectiveFovLH(FOV, aspect, NEAR_Z, FAR_Z);
}

void Camera::Draw(void) const
{
	XMMATRIX view, projection;
	MakeMatrices(&view, &projection);
	Shader_SetViewMatrix(view);
	Shader_SetProjectionMatrix(projection);
}

bool Camera::WorldToScreen(const XMFLOAT3& world, XMFLOAT2* screen) const
{
	XMMATRIX view, projection;
	MakeMatrices(&view, &projection);
	const XMVECTOR clip = XMVector4Transform(XMVectorSet(world.x, world.y, world.z, 1.0f), view * projection);
	const float w = XMVectorGetW(clip);
	if (w <= 0.0f) {
		return false;	// カメラの後ろ
	}
	// -1〜1（上が +1）を、左上が原点のピクセルに変える
	const float ndcX = XMVectorGetX(clip) / w;
	const float ndcY = XMVectorGetY(clip) / w;
	screen->x = (ndcX + 1.0f) * 0.5f * m_ViewWidth;
	screen->y = (1.0f - ndcY) * 0.5f * m_ViewHeight;
	return true;
}

void Camera::SetViewSize(float width, float height)
{
	m_ViewWidth = width;
	m_ViewHeight = height;
}

XMFLOAT3 Camera::GetPosition(void) const
{
	return m_Position;
}

float Camera::GetYaw(void) const
{
	return m_Yaw;
}

int Camera::GetTarget(void) const
{
	// 触腕はカメラのあとに更新されるので、つかんだフレームから公開上も解除する。
	return m_Arms->IsHolding() ? -1 : m_LockTarget;
}

XMFLOAT3 Camera::GetDirection(void) const
{
	return { cosf(m_Pitch) * sinf(m_Yaw), sinf(m_Pitch), cosf(m_Pitch) * cosf(m_Yaw) };
}

void Camera::Initialize(const Stage& stage, const FighterManager& fighters, int slot, const ArmManager& arms, const Match& match)
{
	m_Stage = &stage;
	m_Player = &fighters.GetFighter(slot);
	m_Fighters = &fighters;
	m_Slot = slot;
	m_Arms = &arms;
	m_Match = &match;
	Reset();
}
