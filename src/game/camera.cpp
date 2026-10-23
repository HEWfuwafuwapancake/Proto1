#include "game/camera.h"
#include "core/shader.h"
#include "input/action.h"
#include "game/player.h"


//------------------------------------------------------------------------------
//	調整用の値
//------------------------------------------------------------------------------
// マウス感度・矢印キー・右スティックで回る速さは action.cpp の「調整用の値」にある
static constexpr float FOLLOW_DISTANCE = 8.0f;						// プレイヤーの見ている点からカメラまでの距離
static constexpr float TARGET_HEIGHT_RATE = 0.8f;					// 見ている点の高さ（プレイヤーの高さに対する割合。0.8 で頭のあたり）
static constexpr float PITCH_MIN = XMConvertToRadians(-70.0f);		// いちばん下を向いたとき（上から見下ろす）
static constexpr float PITCH_MAX = XMConvertToRadians(20.0f);		// いちばん上を向いたとき（下から見上げる）
static constexpr float START_PITCH = XMConvertToRadians(-20.0f);	// 最初の上下の向き（少し見下ろす）

static constexpr float FOV = XMConvertToRadians(60.0f);				// 視野角（縦）
static constexpr float NEAR_Z = 0.1f;								// これより近いものは描かない
static constexpr float FAR_Z = 1000.0f;								// これより遠いものは描かない


static XMFLOAT3 g_Position;		// カメラの位置
static float g_Yaw;				// 左右の向き（ラジアン。0 で +Z 方向を見る）
static float g_Pitch;			// 上下の向き（ラジアン。プラスで上を見る）


// 見ている向き（長さ 1）
static XMFLOAT3 LookDirection(void)
{
	return { cosf(g_Pitch) * sinf(g_Yaw), sinf(g_Pitch), cosf(g_Pitch) * cosf(g_Yaw) };
}

// プレイヤーの見ている点から、見ている向きの反対へ FOLLOW_DISTANCE 下がった位置に置く
static void FollowPlayer(void)
{
	const XMFLOAT3 feet = PlayerGetPosition();
	const XMFLOAT3 dir = LookDirection();
	const float targetY = feet.y + PlayerGetHeight() * TARGET_HEIGHT_RATE;
	g_Position = {
		feet.x - dir.x * FOLLOW_DISTANCE,
		targetY - dir.y * FOLLOW_DISTANCE,
		feet.z - dir.z * FOLLOW_DISTANCE,
	};
}


void CameraInitialize(void)
{
	g_Yaw = 0.0f;
	g_Pitch = START_PITCH;
	FollowPlayer();
}

void CameraFinalize(void)
{

}

void CameraUpdate(void)
{
	// ---- 向き ----
	// 「カメラを回す」行動（マウス ＋ 矢印キー ＋ 右スティック）
	const ACTION_AXIS look = ActionGetLook();
	g_Yaw += look.X;
	g_Pitch += look.Y;
	if (g_Pitch > PITCH_MAX) g_Pitch = PITCH_MAX;
	if (g_Pitch < PITCH_MIN) g_Pitch = PITCH_MIN;

	// ---- 位置 ----
	// 仮の動き：プレイヤーの後ろ上にぴったりついていく（めり込まない・なめらかに追うなどは No.9 の TPS カメラで作る）
	FollowPlayer();
}

void CameraDraw(void)
{
	// ビュー行列（カメラの位置と、見ている向き）
	const XMFLOAT3 d = LookDirection();
	XMVECTOR eye = XMLoadFloat3(&g_Position);
	XMVECTOR dir = XMVectorSet(d.x, d.y, d.z, 0.0f);
	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	Shader_SetViewMatrix(XMMatrixLookToLH(eye, dir, up));

	// プロジェクション行列（遠くのものほど小さく見える透視投影）
	float aspect = (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT;
	Shader_SetProjectionMatrix(XMMatrixPerspectiveFovLH(FOV, aspect, NEAR_Z, FAR_Z));
}

XMFLOAT3 CameraGetPosition(void)
{
	return g_Position;
}

float CameraGetYaw(void)
{
	return g_Yaw;
}
