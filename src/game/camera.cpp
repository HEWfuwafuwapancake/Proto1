#include "core/gametime.h"
#include "core/shader.h"
#include "game/camera.h"
#include "game/player.h"
#include "game/match.h"
#include "game/stage.h"
#include "game/dummy.h"
#include "game/arm.h"
#include <limits>
#include "input/action.h"
#include "physics/cameracalc.h"


//------------------------------------------------------------------------------
//	調整用の値
//------------------------------------------------------------------------------
// マウス感度・矢印キー・右スティックで回る速さは action.cpp の「調整用の値」にある
static constexpr float FOLLOW_DISTANCE = 8.0f;						// 見ている点からカメラまでの距離
static constexpr float TARGET_HEIGHT_RATE = 0.8f;					// 見ている点の高さ（プレイヤーの高さに対する割合。0.8 で頭のあたり）
static constexpr float YAW_SMOOTH_TIME = 0.12f;						// 左右の角度が目標へ追いつくまでのおよその秒数
static constexpr float PITCH_SMOOTH_TIME = 0.12f;					// 上下の角度が目標へ追いつくまでのおよその秒数
static constexpr float DISTANCE_SMOOTH_TIME = 0.3f;					// 障害物から離れたとき、元の距離へ戻るまでのおよその秒数
static constexpr float LOOK_SMOOTH_TIME = 0.15f;						// 見ている点の左右（XZ）が追いつくまでのおよその秒数
static constexpr float LOOK_HEIGHT_SMOOTH_TIME = 0.5f;				// 見ている点の高さが追いつくまでのおよその秒数
static constexpr float LOOK_DEADZONE = 0.3f;							// 見ている点の左右（XZ）の遊び。この距離を超えた分だけ目標を動かす
static constexpr float FOLLOW_TURN_SPEED = XMConvertToRadians(90.0f);	// 横移動について回る速さ（1秒に回る角度。入力の大きさに比例）
static constexpr float MANUAL_PAUSE = 0.5f;							// 自分でカメラを回したあと、横移動について回るのを止める秒数
static constexpr float STAGE_MARGIN = 0.3f;							// 浮島の面から、カメラをこれだけ離す（めり込んで見えないように）
static constexpr float PITCH_MIN = XMConvertToRadians(-35.0f);		// いちばん下を向いたとき（上から見下ろす）
static constexpr float PITCH_MAX = XMConvertToRadians(0.0f);		// いちばん上を向いたとき（水平まで）
static constexpr float START_PITCH = XMConvertToRadians(-20.0f);	// 最初の上下の向き（少し見下ろす）
static constexpr float SWITCH_SMOOTH_TIME = 0.06f;					// 切り替え直後の角度のバネ
static constexpr float SWITCH_TIME = 0.3f;							// 角度のバネを速める秒数
static constexpr float TARGET_POINT_RATE = 0.25f;					// 見ている点を敵へずらす割合
static constexpr float TARGET_SCREEN_MARGIN = 0.15f;				// 画面の端に残す余白
static constexpr float TARGET_FALL_MARGIN = 0.1f;					// 敵の足元が浮島の上面からこれだけ下へ落ちたら、ターゲットから外す（追いかけない）

static constexpr float FOV = XMConvertToRadians(60.0f);				// 視野角（縦）
static constexpr float NEAR_Z = 0.1f;								// これより近いものは描かない
static constexpr float FAR_Z = 1000.0f;								// これより遠いものは描かない


// プレイヤーの頭のあたり（見ている点の目標）
VEC3 Camera::PlayerHead(void) const
{
	const XMFLOAT3 feet = m_Player->GetPosition();
	return { feet.x, feet.y + m_Player->GetHeight() * TARGET_HEIGHT_RATE, feet.z };
}

// 生存・浮島より下への落下・保持状態を候補へ反映する。配列の添字とダミー番号は同じ。
// 浮島の上面より下へ落ちた敵は候補から外し、カメラが下まで追いかけないようにする。
void Camera::CollectTargets(CAMERA_FOCUS_TARGET* targets) const
{
	for (int i = 0; i < DUMMY_COUNT; ++i) {
		const TENTACLE_TARGET dummy = m_Dummies->GetTarget(i);
		targets[i].Position = Vec3Scale(Vec3Add(dummy.Body.Start, dummy.Body.End), 0.5f);
		const float feetY = dummy.Body.Start.Y - dummy.Body.Radius;
		targets[i].Available = dummy.Available && feetY >= m_Stage->GetStage().TopY - TARGET_FALL_MARGIN;
	}
}

bool Camera::IsTargetAvailable(int target, const CAMERA_FOCUS_TARGET* targets) const
{
	return target >= 0 && target < DUMMY_COUNT && targets[target].Available;
}

// 角度は全周・距離は無制限。現在の敵への優先ボーナスも付けない。
int Camera::PickFrontTarget(const CAMERA_FOCUS_TARGET* targets) const
{
	return CameraCalcPickFocus(m_Yaw, PlayerHead(), targets, DUMMY_COUNT, -1,
		XM_PI, (std::numeric_limits<float>::infinity)(), 0.0f);
}

VEC3 Camera::LockFocusPoint(const VEC3& player, const CAMERA_FOCUS_TARGET* targets) const
{
	// 身長を半径にすれば、頭付近を中心に体全体を安全側で囲める。
	return CameraCalcFocusPoint(player, targets[m_LockTarget].Position, m_Yaw, m_Pitch,
		m_Distance, FOV, (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT,
		TARGET_SCREEN_MARGIN, m_Player->GetHeight(), TARGET_POINT_RATE);
}

void Camera::UpdateLockTarget(const CAMERA_FOCUS_TARGET* targets)
{
	const bool holding = m_Arms->IsHolding();
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
			const int direction = (ActionIsTrigger(ACTION_TARGET_RIGHT) ? 1 : 0)
				- (ActionIsTrigger(ACTION_TARGET_LEFT) ? 1 : 0);
			next = CameraCalcNextTarget(PlayerHead(), targets, DUMMY_COUNT, next, direction);
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

// 見ている点のまわりの、向きと距離で決まる位置に置き、浮島にめり込むなら手前まで近づける
void Camera::PlaceCamera(float dt)
{
	const VEC3 orbit = CameraCalcOrbitPosition(m_Target, m_Yaw, m_Pitch, FOLLOW_DISTANCE);
	const VEC3 clipped = CameraCalcClipToStage(m_Target, orbit, m_Stage->GetStage(), STAGE_MARGIN);
	const float distanceGoal = Vec3Length(Vec3Sub(clipped, m_Target));
	if (distanceGoal < m_Distance) {
		// 浮島にぶつかるときは即座に近づけ、離れる向きの速度も消す。
		m_Distance = distanceGoal;
		m_DistanceVelocity = 0.0f;
	}
	else {
		m_Distance = CameraCalcSmoothDamp(m_Distance, distanceGoal, m_DistanceVelocity, DISTANCE_SMOOTH_TIME, dt);
	}
	const VEC3 pos = CameraCalcOrbitPosition(m_Target, m_Yaw, m_Pitch, m_Distance);
	m_Position = { pos.X, pos.Y, pos.Z };
}

// やり直し・復活では目標へ瞬時に合わせ、すべてのバネの速度を消す。
void Camera::SnapToPlayer(void)
{
	m_Yaw = m_YawGoal;
	m_Pitch = m_PitchGoal;
	CAMERA_FOCUS_TARGET targets[DUMMY_COUNT];
	CollectTargets(targets);
	m_LockTarget = PickFrontTarget(targets);
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
	const float dt = GameTimeGetDeltaTime();
	CAMERA_FOCUS_TARGET targets[DUMMY_COUNT];
	CollectTargets(targets);
	UpdateLockTarget(targets);
	// 手動入力も横移動の sway も、現在角ではなく目標角へ足す。
	const ACTION_AXIS look = ActionGetLook();
	if (m_LockTarget >= 0) {
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
	else if (m_LockTarget < 0 && m_Match->IsPlaying()) {
		m_YawGoal += ActionGetMove().X * FOLLOW_TURN_SPEED * dt;
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
	// ジャンプ中は最後に立っていた高さを保つ。浮島の上面より下へ落ちたら、その先まで追う。
	if (m_Player->IsOnGround() || m_Player->GetPosition().y < m_Stage->GetStage().TopY) {
		m_PlayerGoal.Y = head.Y;
	}
	const VEC3 lockHead{ head.X, m_PlayerGoal.Y, head.Z };
	m_TargetGoal = m_LockTarget >= 0 ? LockFocusPoint(lockHead, targets) : m_PlayerGoal;
	m_Target.X = CameraCalcSmoothDamp(m_Target.X, m_TargetGoal.X, m_TargetVelocity.X, LOOK_SMOOTH_TIME, dt);
	m_Target.Z = CameraCalcSmoothDamp(m_Target.Z, m_TargetGoal.Z, m_TargetVelocity.Z, LOOK_SMOOTH_TIME, dt);
	m_Target.Y = CameraCalcSmoothDamp(m_Target.Y, m_TargetGoal.Y, m_TargetVelocity.Y, LOOK_HEIGHT_SMOOTH_TIME, dt);
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
	float aspect = (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT;
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
	screen->x = (ndcX + 1.0f) * 0.5f * (float)SCREEN_WIDTH;
	screen->y = (1.0f - ndcY) * 0.5f * (float)SCREEN_HEIGHT;
	return true;
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

void Camera::Initialize(const Stage& stage, const Player& player, const DummyManager& dummies, const ArmManager& arms, const Match& match)
{
	m_Stage = &stage;
	m_Player = &player;
	m_Dummies = &dummies;
	m_Arms = &arms;
	m_Match = &match;
	Reset();
}
