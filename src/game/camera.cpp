#include "core/gametime.h"
#include "core/shader.h"
#include "game/camera.h"
#include "game/player.h"
#include "game/stage.h"
#include "input/action.h"
#include "physics/cameracalc.h"


//------------------------------------------------------------------------------
//	調整用の値
//------------------------------------------------------------------------------
// マウス感度・矢印キー・右スティックで回る速さは action.cpp の「調整用の値」にある
static constexpr float FOLLOW_DISTANCE = 8.0f;						// 見ている点からカメラまでの距離
static constexpr float TARGET_HEIGHT_RATE = 0.8f;					// 見ている点の高さ（プレイヤーの高さに対する割合。0.8 で頭のあたり）
static constexpr float FOLLOW_SPEED = 10.0f;						// 見ている点がプレイヤーに追いつく速さ（大きいほどぴったり。小さいほどゆっくり遅れてついてくる）
static constexpr float STAGE_MARGIN = 0.3f;							// 浮島の面から、カメラをこれだけ離す（めり込んで見えないように）
static constexpr float PITCH_MIN = XMConvertToRadians(-70.0f);		// いちばん下を向いたとき（上から見下ろす）
static constexpr float PITCH_MAX = XMConvertToRadians(30.0f);		// いちばん上を向いたとき（下から見上げる。浮島に当たったら近づく）
static constexpr float START_PITCH = XMConvertToRadians(-20.0f);	// 最初の上下の向き（少し見下ろす）

static constexpr float FOV = XMConvertToRadians(60.0f);				// 視野角（縦）
static constexpr float NEAR_Z = 0.1f;								// これより近いものは描かない
static constexpr float FAR_Z = 1000.0f;								// これより遠いものは描かない


static XMFLOAT3 g_Position;		// カメラの位置
static VEC3 g_Target;	// 見ている点（プレイヤーの頭のあたりを、なめらかに追いかける）
static float g_Yaw;				// 左右の向き（ラジアン。0 で +Z 方向を見る）
static float g_Pitch;			// 上下の向き（ラジアン。プラスで上を見る）


// プレイヤーの頭のあたり（見ている点の目標）
static VEC3 PlayerHead(void)
{
	const XMFLOAT3 feet = PlayerGetPosition();
	return { feet.x, feet.y + PlayerGetHeight() * TARGET_HEIGHT_RATE, feet.z };
}

// 見ている点のまわりの、向きと距離で決まる位置に置き、浮島にめり込むなら手前まで近づける
static void PlaceCamera(void)
{
	const VEC3 orbit = CameraCalcOrbitPosition(g_Target, g_Yaw, g_Pitch, FOLLOW_DISTANCE);
	const VEC3 pos = CameraCalcClipToStage(g_Target, orbit, StageGet(), STAGE_MARGIN);
	g_Position = { pos.X, pos.Y, pos.Z };
}


void CameraInitialize(void)
{
	g_Yaw = 0.0f;
	g_Pitch = START_PITCH;
	g_Target = PlayerHead();
	PlaceCamera();
}

void CameraFinalize(void)
{

}

void CameraUpdate(void)
{
	// ---- 向き ----
	// 「カメラを回す」行動（マウス ＋ 矢印キー ＋ 右スティック）。プレイヤーが歩いても自動では回り込まない（向きは自分で変える）
	const ACTION_AXIS look = ActionGetLook();
	g_Yaw += look.X;
	g_Pitch += look.Y;
	if (g_Pitch > PITCH_MAX) g_Pitch = PITCH_MAX;
	if (g_Pitch < PITCH_MIN) g_Pitch = PITCH_MIN;

	// ---- 位置 ----
	// 見ている点だけをなめらかに追わせる（カメラの位置ごと遅らせると、回したときに画面が揺れるため）
	g_Target = CameraCalcSmooth(g_Target, PlayerHead(), FOLLOW_SPEED, GameTimeGetDeltaTime());
	PlaceCamera();
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

XMFLOAT3 CameraGetDirection(void)
{
	return { cosf(g_Pitch) * sinf(g_Yaw), sinf(g_Pitch), cosf(g_Pitch) * cosf(g_Yaw) };
}
