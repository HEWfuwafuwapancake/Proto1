#include "core/dx11.h"
#include "core/shader.h"
#include "game/cube.h"


static ID3D11Buffer* g_pVertexBuffer = nullptr;	// 頂点バッファ
static ID3D11Buffer* g_pIndexBuffer = nullptr;	// インデックスバッファ

static constexpr int FACE_COUNT = 6;
static constexpr int VERTEX_COUNT = FACE_COUNT * 4;	// 法線を面ごとに変えるため、角の頂点は面ごとに別に持つ
static constexpr int INDEX_COUNT = FACE_COUNT * 6;	// 1面 = 三角形2枚 = 6インデックス

static XMFLOAT3 g_Position = { 0.0f, 1.5f, 0.0f };	// 位置
static XMFLOAT3 g_Rotation = { 0.0f, 0.0f, 0.0f };	// 回転（ラジアン）
static XMFLOAT3 g_Scale = { 1.0f, 1.0f, 1.0f };		// 大きさ
static XMFLOAT4 g_Color = { 1.0f, 0.6f, 0.2f, 1.0f };	// 色（オレンジ）


void CubeInitialize(void)
{
	// 各面の「法線」「外から見て右向き」「外から見て上向き」
	// 外から見て 左上→右上→左下 が時計回りになるように頂点を並べる（バックフェースカリング対策）
	const XMFLOAT3 faces[FACE_COUNT][3] = {
		{ {  0.0f,  0.0f, -1.0f }, {  1.0f, 0.0f,  0.0f }, { 0.0f, 1.0f,  0.0f } },	// 手前
		{ {  0.0f,  0.0f,  1.0f }, { -1.0f, 0.0f,  0.0f }, { 0.0f, 1.0f,  0.0f } },	// 奥
		{ {  1.0f,  0.0f,  0.0f }, {  0.0f, 0.0f,  1.0f }, { 0.0f, 1.0f,  0.0f } },	// 右
		{ { -1.0f,  0.0f,  0.0f }, {  0.0f, 0.0f, -1.0f }, { 0.0f, 1.0f,  0.0f } },	// 左
		{ {  0.0f,  1.0f,  0.0f }, {  1.0f, 0.0f,  0.0f }, { 0.0f, 0.0f,  1.0f } },	// 上
		{ {  0.0f, -1.0f,  0.0f }, {  1.0f, 0.0f,  0.0f }, { 0.0f, 0.0f, -1.0f } },	// 下
	};

	VERTEX_3D vertex[VERTEX_COUNT];
	WORD index[INDEX_COUNT];

	for (int i = 0; i < FACE_COUNT; i++) {
		XMVECTOR n = XMLoadFloat3(&faces[i][0]);
		XMVECTOR r = XMLoadFloat3(&faces[i][1]) * 0.5f;
		XMVECTOR u = XMLoadFloat3(&faces[i][2]) * 0.5f;
		XMVECTOR c = n * 0.5f;	// 面の中心（1辺1.0の立方体）

		VERTEX_3D* v = &vertex[i * 4];
		XMStoreFloat3(&v[0].Position, c - r + u);	// 左上
		XMStoreFloat3(&v[1].Position, c + r + u);	// 右上
		XMStoreFloat3(&v[2].Position, c - r - u);	// 左下
		XMStoreFloat3(&v[3].Position, c + r - u);	// 右下

		v[0].TexCoord = { 0.0f, 0.0f };
		v[1].TexCoord = { 1.0f, 0.0f };
		v[2].TexCoord = { 0.0f, 1.0f };
		v[3].TexCoord = { 1.0f, 1.0f };

		for (int j = 0; j < 4; j++) {
			v[j].Normal = faces[i][0];
			v[j].Diffuse = { 1.0f, 1.0f, 1.0f, 1.0f };
		}

		// 三角形2枚（左上・右上・左下）（左下・右上・右下）
		WORD base = (WORD)(i * 4);
		WORD* idx = &index[i * 6];
		idx[0] = base + 0; idx[1] = base + 1; idx[2] = base + 2;
		idx[3] = base + 2; idx[4] = base + 1; idx[5] = base + 3;
	}

	// 頂点バッファ生成（中身を変えないので DEFAULT で作り、作るときにデータを渡す）
	D3D11_BUFFER_DESC bd{};
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(VERTEX_3D) * VERTEX_COUNT;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;

	D3D11_SUBRESOURCE_DATA sd{};
	sd.pSysMem = vertex;
	DirectXGetDevice()->CreateBuffer(&bd, &sd, &g_pVertexBuffer);

	// インデックスバッファ生成
	bd.ByteWidth = sizeof(WORD) * INDEX_COUNT;
	bd.BindFlags = D3D11_BIND_INDEX_BUFFER;

	sd.pSysMem = index;
	DirectXGetDevice()->CreateBuffer(&bd, &sd, &g_pIndexBuffer);
}

void CubeFinalize(void)
{
	SAFE_RELEASE(g_pIndexBuffer);
	SAFE_RELEASE(g_pVertexBuffer);
}

void CubeUpdate(void)
{
	// くるくる回す
	g_Rotation.x += XMConvertToRadians(0.5f);
	g_Rotation.y += XMConvertToRadians(1.0f);
}

XMMATRIX CubeGetWorldMatrix(void)
{
	// ワールド行列（拡大縮小 → 回転 → 移動 の順に掛ける）
	return XMMatrixScaling(g_Scale.x, g_Scale.y, g_Scale.z)
		* XMMatrixRotationRollPitchYaw(g_Rotation.x, g_Rotation.y, g_Rotation.z)
		* XMMatrixTranslation(g_Position.x, g_Position.y, g_Position.z);
}

XMFLOAT4 CubeGetColor(void)
{
	return g_Color;
}

XMFLOAT3 CubeGetPosition(void)
{
	return g_Position;
}

// 球の中心を立方体の「回転だけを戻した座標」に直す（原点が立方体の中心、箱は -half 〜 +half）
// 大きさは half に入れるので、軸ごとに大きさが違っても距離を正しく測れる
static XMVECTOR ToCubeLocal(const XMFLOAT3& center, const XMMATRIX& rotation)
{
	// ワールド → 立方体の座標（移動を戻してから、回転を戻す＝回転の転置を掛ける）
	return XMVector3TransformNormal(XMLoadFloat3(&center) - XMLoadFloat3(&g_Position), XMMatrixTranspose(rotation));
}

bool CubeOverlapsSphere(const XMFLOAT3& center, float radius)
{
	XMMATRIX rotation = XMMatrixRotationRollPitchYaw(g_Rotation.x, g_Rotation.y, g_Rotation.z);
	XMVECTOR half = XMVectorSet(0.5f * g_Scale.x, 0.5f * g_Scale.y, 0.5f * g_Scale.z, 0.0f);
	XMVECTOR local = ToCubeLocal(center, rotation);

	// 箱の中で、球の中心にいちばん近い点までの距離が半径より短ければ重なっている
	XMVECTOR closest = XMVectorClamp(local, -half, half);
	return XMVectorGetX(XMVector3LengthSq(local - closest)) < radius * radius;
}

bool CubePushOutSphere(XMFLOAT3& center, float radius)
{
	XMMATRIX rotation = XMMatrixRotationRollPitchYaw(g_Rotation.x, g_Rotation.y, g_Rotation.z);
	XMVECTOR cubePos = XMLoadFloat3(&g_Position);
	XMVECTOR half = XMVectorSet(0.5f * g_Scale.x, 0.5f * g_Scale.y, 0.5f * g_Scale.z, 0.0f);
	XMVECTOR local = ToCubeLocal(center, rotation);

	// 箱の中で、球の中心にいちばん近い点
	XMVECTOR closest = XMVectorClamp(local, -half, half);
	XMVECTOR diff = local - closest;
	float distSq = XMVectorGetX(XMVector3LengthSq(diff));

	if (distSq >= radius * radius) {
		return false;	// 重なっていない
	}

	if (distSq > 0.0f) {
		// 中心は箱の外：いちばん近い点から、ちょうど radius だけ離れた位置まで押し出す
		local = closest + XMVector3Normalize(diff) * radius;
	}
	else {
		// 中心が箱の中にめり込んでいる：いちばん浅い面の方向へ押し出す
		XMFLOAT3 l, h;
		XMStoreFloat3(&l, local);
		XMStoreFloat3(&h, half);
		float depthX = h.x - fabsf(l.x);
		float depthY = h.y - fabsf(l.y);
		float depthZ = h.z - fabsf(l.z);
		if (depthX <= depthY && depthX <= depthZ) l.x = (l.x < 0.0f ? -1.0f : 1.0f) * (h.x + radius);
		else if (depthY <= depthZ)                l.y = (l.y < 0.0f ? -1.0f : 1.0f) * (h.y + radius);
		else                                      l.z = (l.z < 0.0f ? -1.0f : 1.0f) * (h.z + radius);
		local = XMLoadFloat3(&l);
	}

	// 立方体の座標 → ワールド
	XMStoreFloat3(&center, XMVector3TransformNormal(local, rotation) + cubePos);
	return true;
}

void CubeDraw(void)
{
	Shader_SetWorldMatrix(CubeGetWorldMatrix());

	// マテリアル
	MATERIAL material{};
	material.Diffuse = g_Color;
	material.Ambient = g_Color;
	Shader_SetMaterial(material);

	// 頂点バッファとインデックスバッファを描画パイプラインに設定
	UINT stride = sizeof(VERTEX_3D);
	UINT offset = 0;
	DirectXGetDeviceContext()->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);
	DirectXGetDeviceContext()->IASetIndexBuffer(g_pIndexBuffer, DXGI_FORMAT_R16_UINT, 0);

	// プリミティブトポロジの設定（三角形をばらばらに並べる）
	DirectXGetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// ポリゴン描画命令発行
	DirectXGetDeviceContext()->DrawIndexed(INDEX_COUNT, 0, 0);
}
