#include "game/camera.h"
#include "core/shader.h"
#include "input/action.h"
#include "input/keyboard.h"
#include "game/field.h"
#include "game/cube.h"


//------------------------------------------------------------------------------
//	調整用の値
//------------------------------------------------------------------------------
// マウス感度・矢印キーで回る速さは action.cpp の「調整用の値」にある
static constexpr float MOVE_SPEED = 0.15f;							// 1フレームに進む距離
static constexpr float RUN_MULTIPLIER = 3.0f;						// Shift を押したときの速さの倍率
static constexpr float EYE_MIN_HEIGHT = 0.5f;						// 地面からこれ以上は下がらない
static constexpr float COLLISION_RADIUS = 0.25f;						// カメラの当たり判定を球として扱う半径
static constexpr float WORLD_LIMIT = 48.0f;							// ワールドの端（これより外へは行けない）

static constexpr float FOV = XMConvertToRadians(60.0f);				// 視野角（縦）
static constexpr float NEAR_Z = 0.1f;								// これより近いものは描かない
static constexpr float FAR_Z = 1000.0f;								// これより遠いものは描かない


static XMFLOAT3 g_Position;		// カメラの位置
static float g_Yaw;				// 左右の向き（ラジアン。0 で +Z 方向を見る）
static float g_Pitch;			// 上下の向き（ラジアン。プラスで上を見る）


// ワールドの端から出ない・地面にもぐらない
static void ClampToWorld(XMFLOAT3& p)
{
	if (p.x > WORLD_LIMIT)  p.x = WORLD_LIMIT;
	if (p.x < -WORLD_LIMIT) p.x = -WORLD_LIMIT;
	if (p.z > WORLD_LIMIT)  p.z = WORLD_LIMIT;
	if (p.z < -WORLD_LIMIT) p.z = -WORLD_LIMIT;

	float ground = FieldGetHeight(p.x, p.z);
	if (p.y < ground + EYE_MIN_HEIGHT) p.y = ground + EYE_MIN_HEIGHT;
}

// 横（XZ）に distance だけずらして、ワールドの制限をかけた位置
static XMFLOAT3 ShiftXZ(XMFLOAT3 p, float dirX, float dirZ, float distance)
{
	p.x += dirX * distance;
	p.z += dirZ * distance;
	ClampToWorld(p);
	return p;
}

// 立方体からの押し出しと、ワールドの制限（ClampToWorld）の両方を満たす位置にする
static void ResolvePosition(XMFLOAT3& p)
{
	CubePushOutSphere(p, COLLISION_RADIUS);
	ClampToWorld(p);
	if (!CubeOverlapsSphere(p, COLLISION_RADIUS)) {
		return;
	}

	// 押し出しを、地面の制限が取り消した（傾いた立方体の底と地面にはさまれた）
	// → 上下ではなく横（XZ）に、立方体の中心から離れる向きへずらして抜ける
	XMFLOAT3 cube = CubeGetPosition();
	float dirX = p.x - cube.x;
	float dirZ = p.z - cube.z;
	float len = sqrtf(dirX * dirX + dirZ * dirZ);
	if (len < 0.0001f) { dirX = 0.0f; dirZ = -1.0f; len = 1.0f; }	// 真下にいるときは手前へ
	dirX /= len;
	dirZ /= len;

	// 重ならなくなる最小の距離を探す（倍々に広げて外に出る距離を見つけてから、半分ずつ狭める）
	// 箱と球の重なる範囲は凸な形なので、まっすぐ動けば一度出たらもう入らない
	float lo = 0.0f;
	float hi = COLLISION_RADIUS;
	for (int i = 0; i < 10 && CubeOverlapsSphere(ShiftXZ(p, dirX, dirZ, hi), COLLISION_RADIUS); i++) {
		lo = hi;
		hi *= 2.0f;
	}
	for (int i = 0; i < 12; i++) {
		float mid = (lo + hi) * 0.5f;
		if (CubeOverlapsSphere(ShiftXZ(p, dirX, dirZ, mid), COLLISION_RADIUS)) lo = mid;
		else hi = mid;
	}
	p = ShiftXZ(p, dirX, dirZ, hi);
}


void CameraInitialize(void)
{
	g_Position = { 0.0f, 4.0f, -18.0f };
	g_Yaw = 0.0f;
	g_Pitch = XMConvertToRadians(-12.0f);
}

void CameraFinalize(void)
{

}

void CameraUpdate(void)
{
	// ---- 向き ----
	// 「カメラを回す」行動（マウス ＋ 矢印キー。感度は action.cpp の調整用の値）
	const ACTION_AXIS look = ActionGetLook();
	g_Yaw += look.X;
	g_Pitch += look.Y;

	// 真上・真下を越えると上下が反転するので止める
	const float PITCH_LIMIT = XMConvertToRadians(89.0f);
	if (g_Pitch > PITCH_LIMIT)  g_Pitch = PITCH_LIMIT;
	if (g_Pitch < -PITCH_LIMIT) g_Pitch = -PITCH_LIMIT;

	// ---- 移動 ----
	// 前と右（上下には傾けない。下を見ながら W を押しても地面にもぐらないように）
	XMFLOAT3 forward = { sinf(g_Yaw), 0.0f, cosf(g_Yaw) };
	XMFLOAT3 right = { cosf(g_Yaw), 0.0f, -sinf(g_Yaw) };

	float speed = MOVE_SPEED;
	if (KeyboardIsPress(VK_SHIFT)) speed *= RUN_MULTIPLIER;

	XMFLOAT3 move = { 0.0f, 0.0f, 0.0f };
	if (KeyboardIsPress('W')) { move.x += forward.x; move.z += forward.z; }
	if (KeyboardIsPress('S')) { move.x -= forward.x; move.z -= forward.z; }
	if (KeyboardIsPress('D')) { move.x += right.x;   move.z += right.z; }
	if (KeyboardIsPress('A')) { move.x -= right.x;   move.z -= right.z; }
	if (KeyboardIsPress(VK_SPACE))   move.y += 1.0f;
	if (KeyboardIsPress(VK_CONTROL)) move.y -= 1.0f;

	// 斜め移動が速くならないように長さを1にそろえる
	XMVECTOR m = XMLoadFloat3(&move);
	if (XMVectorGetX(XMVector3LengthSq(m)) > 0.0f) {
		m = XMVector3Normalize(m) * speed;
	}
	XMFLOAT3 delta;
	XMStoreFloat3(&delta, m);

	// 動かしてから、立方体からの押し出しとワールドの制限をかける（押し出す向きは面に垂直なので、面に沿って滑る）
	// 1回に半径より長く動くと立方体をすり抜けるおそれがあるので、半径以下ずつに分けて動かす
	// 動いていないときも1回はかけるので、回転してきた立方体にめり込んだままにならない
	float length = XMVectorGetX(XMVector3Length(m));
	int steps = (int)ceilf(length / COLLISION_RADIUS);
	if (steps < 1) steps = 1;

	for (int i = 0; i < steps; i++) {
		g_Position.x += delta.x / steps;
		g_Position.y += delta.y / steps;
		g_Position.z += delta.z / steps;
		ResolvePosition(g_Position);
	}
}

void CameraDraw(void)
{
	// ビュー行列（カメラの位置と、見ている向き）
	XMVECTOR eye = XMLoadFloat3(&g_Position);
	XMVECTOR dir = XMVectorSet(cosf(g_Pitch) * sinf(g_Yaw), sinf(g_Pitch), cosf(g_Pitch) * cosf(g_Yaw), 0.0f);
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
