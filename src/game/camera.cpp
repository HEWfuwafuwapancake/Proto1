#include "core/gametime.h"
#include "core/shader.h"
#include "game/camera.h"
#include "game/player.h"
#include "game/match.h"
#include "game/stage.h"
#include "game/dummy.h"
#include "game/tentacle.h"
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

static constexpr float FOV = XMConvertToRadians(60.0f);				// 視野角（縦）
static constexpr float NEAR_Z = 0.1f;								// これより近いものは描かない
static constexpr float FAR_Z = 1000.0f;								// これより遠いものは描かない


static XMFLOAT3 g_Position;		// カメラの位置
static VEC3 g_Target;			// 今の見ている点
static VEC3 g_TargetGoal;		// 遊び・接地した高さを反映した、見ている点の目標
static VEC3 g_TargetVelocity;	// 見ている点のバネの速度（軸ごとに持つ）
static float g_Yaw;				// 左右の向き（ラジアン。0 で +Z 方向を見る）
static float g_Pitch;			// 上下の向き（ラジアン。プラスで上を見る）
static float g_YawGoal;			// 入力を足す左右の目標角度
static float g_PitchGoal;		// 入力を足す上下の目標角度
static float g_YawVelocity;		// 左右角のバネの速度
static float g_PitchVelocity;	// 上下角のバネの速度
static float g_Distance;			// 今のカメラまでの距離
static float g_DistanceVelocity;	// 距離のバネの速度
static float g_ManualPause;		// 横移動について回るのを止める残り秒数
static VEC3 g_PlayerGoal;			// 自由カメラ用の遊び・最後に立っていた高さ（敵の注視点と分ける）
static int g_LockTarget = -1;		// 現在ロックオンしているダミー番号
static int g_SavedTarget = -1;	// つかむ前のターゲット番号
static bool g_WasHolding = false;
static float g_SwitchRemaining = 0.0f;


// プレイヤーの頭のあたり（見ている点の目標）
static VEC3 PlayerHead(void)
{
	const XMFLOAT3 feet = PlayerGetPosition();
	return { feet.x, feet.y + PlayerGetHeight() * TARGET_HEIGHT_RATE, feet.z };
}

// 生存・場外・保持状態を候補へ反映する。配列の添字とダミー番号は同じ。
static void CollectTargets(CAMERA_FOCUS_TARGET* targets)
{
	for (int i = 0; i < DUMMY_COUNT; ++i) {
		const TENTACLE_TARGET dummy = DummyGetTarget(i);
		targets[i].Position = Vec3Scale(Vec3Add(dummy.Body.Start, dummy.Body.End), 0.5f);
		const float feetY = dummy.Body.Start.Y - dummy.Body.Radius;
		targets[i].Available = dummy.Available && !StageCalcIsOut(StageGet(), feetY);
	}
}

static bool IsTargetAvailable(int target, const CAMERA_FOCUS_TARGET* targets)
{
	return target >= 0 && target < DUMMY_COUNT && targets[target].Available;
}

// 角度は全周・距離は無制限。現在の敵への優先ボーナスも付けない。
static int PickFrontTarget(const CAMERA_FOCUS_TARGET* targets)
{
	return CameraCalcPickFocus(g_Yaw, PlayerHead(), targets, DUMMY_COUNT, -1,
		XM_PI, (std::numeric_limits<float>::infinity)(), 0.0f);
}

static VEC3 LockFocusPoint(const VEC3& player, const CAMERA_FOCUS_TARGET* targets)
{
	// 身長を半径にすれば、頭付近を中心に体全体を安全側で囲める。
	return CameraCalcFocusPoint(player, targets[g_LockTarget].Position, g_Yaw, g_Pitch,
		g_Distance, FOV, (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT,
		TARGET_SCREEN_MARGIN, PlayerGetHeight(), TARGET_POINT_RATE);
}

static void UpdateLockTarget(const CAMERA_FOCUS_TARGET* targets)
{
	const bool holding = TentacleIsHolding();
	if (holding) {
		if (!g_WasHolding) {
			g_SavedTarget = g_LockTarget;
			g_LockTarget = -1;
			g_YawGoal = g_Yaw;
			g_SwitchRemaining = 0.0f;
		}
	}
	else {
		int next = g_LockTarget;
		if (g_WasHolding) {
			next = IsTargetAvailable(g_SavedTarget, targets) ? g_SavedTarget : PickFrontTarget(targets);
			g_SavedTarget = -1;
		}
		else if (!IsTargetAvailable(next, targets)) {
			next = PickFrontTarget(targets);
		}
		else {
			const int direction = (ActionIsTrigger(ACTION_TARGET_RIGHT) ? 1 : 0)
				- (ActionIsTrigger(ACTION_TARGET_LEFT) ? 1 : 0);
			next = CameraCalcNextTarget(PlayerHead(), targets, DUMMY_COUNT, next, direction);
		}
		if (next != g_LockTarget) {
			g_LockTarget = next;
			g_SwitchRemaining = next >= 0 ? SWITCH_TIME : 0.0f;
			// 自由カメラへ移るときも、角度の速度は持ち越す。
			if (next < 0) g_YawGoal = g_Yaw;
		}
	}
	g_WasHolding = holding;
}

// 見ている点のまわりの、向きと距離で決まる位置に置き、浮島にめり込むなら手前まで近づける
static void PlaceCamera(float dt)
{
	const VEC3 orbit = CameraCalcOrbitPosition(g_Target, g_Yaw, g_Pitch, FOLLOW_DISTANCE);
	const VEC3 clipped = CameraCalcClipToStage(g_Target, orbit, StageGet(), STAGE_MARGIN);
	const float distanceGoal = Vec3Length(Vec3Sub(clipped, g_Target));
	if (distanceGoal < g_Distance) {
		// 浮島にぶつかるときは即座に近づけ、離れる向きの速度も消す。
		g_Distance = distanceGoal;
		g_DistanceVelocity = 0.0f;
	}
	else {
		g_Distance = CameraCalcSmoothDamp(g_Distance, distanceGoal, g_DistanceVelocity, DISTANCE_SMOOTH_TIME, dt);
	}
	const VEC3 pos = CameraCalcOrbitPosition(g_Target, g_Yaw, g_Pitch, g_Distance);
	g_Position = { pos.X, pos.Y, pos.Z };
}

// やり直し・復活では目標へ瞬時に合わせ、すべてのバネの速度を消す。
static void SnapToPlayer(void)
{
	g_Yaw = g_YawGoal;
	g_Pitch = g_PitchGoal;
	CAMERA_FOCUS_TARGET targets[DUMMY_COUNT];
	CollectTargets(targets);
	g_LockTarget = PickFrontTarget(targets);
	g_SavedTarget = -1;
	g_WasHolding = false;
	g_SwitchRemaining = 0.0f;
	if (g_LockTarget >= 0) {
		g_Yaw = g_YawGoal = CameraCalcYawTo(PlayerHead(), targets[g_LockTarget].Position, g_Yaw);
	}
	g_YawVelocity = 0.0f;
	g_PitchVelocity = 0.0f;
	g_PlayerGoal = PlayerHead();
	g_TargetVelocity = {};
	g_Distance = FOLLOW_DISTANCE;
	g_Target = g_TargetGoal = g_LockTarget >= 0 ? LockFocusPoint(g_PlayerGoal, targets) : g_PlayerGoal;
	g_DistanceVelocity = 0.0f;
	g_ManualPause = 0.0f;
	PlaceCamera(0.0f);
}

void CameraInitialize(void)
{
	g_YawGoal = 0.0f;
	g_PitchGoal = START_PITCH;
	SnapToPlayer();
}

void CameraFinalize(void)
{

}

void CameraUpdate(void)
{
	if (PlayerDidRespawn()) {
		SnapToPlayer();
		return;
	}
	const float dt = GameTimeGetDeltaTime();
	CAMERA_FOCUS_TARGET targets[DUMMY_COUNT];
	CollectTargets(targets);
	UpdateLockTarget(targets);
	// 手動入力も横移動の sway も、現在角ではなく目標角へ足す。
	const ACTION_AXIS look = ActionGetLook();
	if (g_LockTarget >= 0) {
		g_YawGoal = CameraCalcYawTo(PlayerHead(), targets[g_LockTarget].Position, g_YawGoal);
	}
	else {
		g_YawGoal += look.X;
	}
	g_PitchGoal += look.Y;
	if (g_PitchGoal > PITCH_MAX) g_PitchGoal = PITCH_MAX;
	if (g_PitchGoal < PITCH_MIN) g_PitchGoal = PITCH_MIN;
	if (look.X != 0.0f || look.Y != 0.0f) {
		g_ManualPause = MANUAL_PAUSE;
	}
	else if (g_ManualPause > 0.0f) {
		g_ManualPause = fmaxf(0.0f, g_ManualPause - dt);
	}
	else if (g_LockTarget < 0 && MatchIsPlaying()) {
		g_YawGoal += ActionGetMove().X * FOLLOW_TURN_SPEED * dt;
	}
	// 長く回し続けても、角度が大きくなり続けないようにする。
	g_YawGoal = atan2f(sinf(g_YawGoal), cosf(g_YawGoal));
	const bool switching = g_SwitchRemaining > 0.0f;
	g_Yaw = CameraCalcSmoothDampAngle(g_Yaw, g_YawGoal, g_YawVelocity,
		switching ? SWITCH_SMOOTH_TIME : YAW_SMOOTH_TIME, dt);
	g_Pitch = CameraCalcSmoothDamp(g_Pitch, g_PitchGoal, g_PitchVelocity,
		switching ? SWITCH_SMOOTH_TIME : PITCH_SMOOTH_TIME, dt);
	g_SwitchRemaining = fmaxf(0.0f, g_SwitchRemaining - dt);

	// 左右は円形の遊びを超えた分だけ目標を動かす（高さとは分ける）。
	const VEC3 head = PlayerHead();
	const VEC3 horizontal = { head.X - g_PlayerGoal.X, 0.0f, head.Z - g_PlayerGoal.Z };
	const float length = Vec3Length(horizontal);
	if (length > LOOK_DEADZONE) {
		g_PlayerGoal = Vec3Add(g_PlayerGoal, Vec3Scale(horizontal, (length - LOOK_DEADZONE) / length));
	}
	// ジャンプ中は最後に立っていた高さを保つ。浮島の上面より下へ落ちたら、その先まで追う。
	if (PlayerIsOnGround() || PlayerGetPosition().y < StageGet().TopY) {
		g_PlayerGoal.Y = head.Y;
	}
	const VEC3 lockHead{ head.X, g_PlayerGoal.Y, head.Z };
	g_TargetGoal = g_LockTarget >= 0 ? LockFocusPoint(lockHead, targets) : g_PlayerGoal;
	g_Target.X = CameraCalcSmoothDamp(g_Target.X, g_TargetGoal.X, g_TargetVelocity.X, LOOK_SMOOTH_TIME, dt);
	g_Target.Z = CameraCalcSmoothDamp(g_Target.Z, g_TargetGoal.Z, g_TargetVelocity.Z, LOOK_SMOOTH_TIME, dt);
	g_Target.Y = CameraCalcSmoothDamp(g_Target.Y, g_TargetGoal.Y, g_TargetVelocity.Y, LOOK_HEIGHT_SMOOTH_TIME, dt);
	PlaceCamera(dt);
}

// ビュー行列とプロジェクション行列を作る（描画と、画面の位置の計算で同じものを使う）
static void MakeMatrices(XMMATRIX* view, XMMATRIX* projection)
{
	// ビュー行列（カメラの位置と、見ている向き）
	// カメラは見ている点から向きの反対に置いてあるので、この向きで見れば見ている点が画面の真ん中に来る
	// （浮島に近づけてカメラが見ている点にほぼ重なっても、向きは決まる）
	XMVECTOR eye = XMLoadFloat3(&g_Position);
	const XMFLOAT3 direction = CameraGetDirection();
	XMVECTOR dir = XMLoadFloat3(&direction);
	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	*view = XMMatrixLookToLH(eye, dir, up);

	// プロジェクション行列（遠くのものほど小さく見える透視投影）
	float aspect = (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT;
	*projection = XMMatrixPerspectiveFovLH(FOV, aspect, NEAR_Z, FAR_Z);
}

void CameraDraw(void)
{
	XMMATRIX view, projection;
	MakeMatrices(&view, &projection);
	Shader_SetViewMatrix(view);
	Shader_SetProjectionMatrix(projection);
}

bool CameraWorldToScreen(const XMFLOAT3& world, XMFLOAT2* screen)
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

XMFLOAT3 CameraGetPosition(void)
{
	return g_Position;
}

float CameraGetYaw(void)
{
	return g_Yaw;
}

int CameraGetTarget(void)
{
	// 触腕はカメラのあとに更新されるので、つかんだフレームから公開上も解除する。
	return TentacleIsHolding() ? -1 : g_LockTarget;
}

XMFLOAT3 CameraGetDirection(void)
{
	return { cosf(g_Pitch) * sinf(g_Yaw), sinf(g_Pitch), cosf(g_Pitch) * cosf(g_Yaw) };
}
