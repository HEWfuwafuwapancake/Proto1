#include "dx11.h"
#include "shader.h"
#include "field.h"
#include <vector>


static ID3D11Buffer* g_pVertexBuffer = nullptr;	// 頂点バッファ
static ID3D11Buffer* g_pIndexBuffer = nullptr;	// インデックスバッファ

static constexpr float FIELD_SIZE = 100.0f;		// 1辺の長さ
static constexpr int   FIELD_DIVISION = 200;	// 1辺を何マスに分けるか（細かいほどなめらか）
static constexpr int   VERTEX_COUNT = (FIELD_DIVISION + 1) * (FIELD_DIVISION + 1);
static constexpr int   INDEX_COUNT = FIELD_DIVISION * FIELD_DIVISION * 6;

// 地形の形
static constexpr float LAKE_RADIUS = 12.0f;			// 湖の半径
static constexpr float LAKE_BOTTOM = -3.0f;			// 湖のいちばん深いところの高さ
static constexpr float SHORE_HEIGHT = 0.6f;			// 岸の高さ


static float Saturate(float v)
{
	return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
}

static float SmoothStep01(float t)
{
	return t * t * (3.0f - 2.0f * t);
}

static float Lerp(float a, float b, float t)
{
	return a + (b - a) * t;
}


float FieldGetHeight(float x, float z)
{
	// 中心からの距離（少しゆがませて、湖の形を丸ではなく自然にする）
	float d = sqrtf(x * x + z * z) + 1.2f * sinf(x * 0.35f) * sinf(z * 0.3f);

	// 湖のくぼみ：中心が LAKE_BOTTOM、半径のところで SHORE_HEIGHT になめらかにつながる
	float basin = Lerp(LAKE_BOTTOM, SHORE_HEIGHT, SmoothStep01(Saturate(d / LAKE_RADIUS)));

	// 丘：湖から離れるほど高くなる
	float hillMask = Saturate((d - LAKE_RADIUS) / LAKE_RADIUS);
	float hills = 1.2f * (sinf(x * 0.15f) * cosf(z * 0.13f) + 1.0f)
				+ 0.6f * (sinf(x * 0.31f + 1.7f) * sinf(z * 0.27f + 0.5f) + 1.0f);

	return basin + hills * hillMask;
}

XMFLOAT3 FieldGetColor(float height)
{
	const XMFLOAT3 sand = { 0.78f, 0.72f, 0.52f };
	const XMFLOAT3 grass = { 0.30f, 0.62f, 0.25f };
	const XMFLOAT3 darkGrass = { 0.20f, 0.45f, 0.18f };

	float t1 = Saturate((height - 0.5f) / 0.6f);
	float t2 = Saturate((height - 2.5f) / 1.5f);

	XMFLOAT3 c;
	c.x = Lerp(Lerp(sand.x, grass.x, t1), darkGrass.x, t2);
	c.y = Lerp(Lerp(sand.y, grass.y, t1), darkGrass.y, t2);
	c.z = Lerp(Lerp(sand.z, grass.z, t1), darkGrass.z, t2);
	return c;
}

// 地形の法線（まわりの高さの差から傾きを求める）
static XMFLOAT3 FieldGetNormal(float x, float z)
{
	const float e = 0.1f;	// 前後左右どれだけずらして高さを比べるか
	XMVECTOR n = XMVectorSet(
		FieldGetHeight(x - e, z) - FieldGetHeight(x + e, z),
		2.0f * e,
		FieldGetHeight(x, z - e) - FieldGetHeight(x, z + e),
		0.0f);
	XMFLOAT3 normal;
	XMStoreFloat3(&normal, XMVector3Normalize(n));
	return normal;
}


void FieldInitialize(void)
{
	std::vector<VERTEX_3D> vertex(VERTEX_COUNT);
	std::vector<WORD> index(INDEX_COUNT);

	const float half = FIELD_SIZE * 0.5f;
	const float step = FIELD_SIZE / FIELD_DIVISION;

	// 頂点：x 方向に i、z 方向に j 番目
	for (int j = 0; j <= FIELD_DIVISION; j++) {
		for (int i = 0; i <= FIELD_DIVISION; i++) {
			float x = -half + i * step;
			float z = -half + j * step;
			float y = FieldGetHeight(x, z);

			VERTEX_3D& v = vertex[j * (FIELD_DIVISION + 1) + i];
			v.Position = { x, y, z };
			v.Normal = FieldGetNormal(x, z);	// 法線：まわりの高さの差から傾きを求める

			XMFLOAT3 color = FieldGetColor(y);
			v.Diffuse = { color.x, color.y, color.z, 1.0f };
			v.TexCoord = { x / 4.0f, z / 4.0f };
		}
	}

	// インデックス：1マス = 三角形2枚。上から見て時計回りになるように並べる
	int k = 0;
	for (int j = 0; j < FIELD_DIVISION; j++) {
		for (int i = 0; i < FIELD_DIVISION; i++) {
			WORD v00 = (WORD)(j * (FIELD_DIVISION + 1) + i);	// 手前左
			WORD v10 = v00 + 1;									// 手前右
			WORD v01 = v00 + (FIELD_DIVISION + 1);				// 奥左
			WORD v11 = v01 + 1;									// 奥右

			index[k++] = v01; index[k++] = v11; index[k++] = v00;
			index[k++] = v00; index[k++] = v11; index[k++] = v10;
		}
	}

	// 頂点バッファ生成
	D3D11_BUFFER_DESC bd{};
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(VERTEX_3D) * VERTEX_COUNT;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;

	D3D11_SUBRESOURCE_DATA sd{};
	sd.pSysMem = vertex.data();
	DirectXGetDevice()->CreateBuffer(&bd, &sd, &g_pVertexBuffer);

	// インデックスバッファ生成
	bd.ByteWidth = sizeof(WORD) * INDEX_COUNT;
	bd.BindFlags = D3D11_BIND_INDEX_BUFFER;

	sd.pSysMem = index.data();
	DirectXGetDevice()->CreateBuffer(&bd, &sd, &g_pIndexBuffer);
}

void FieldFinalize(void)
{
	SAFE_RELEASE(g_pIndexBuffer);
	SAFE_RELEASE(g_pVertexBuffer);
}

void FieldUpdate(void)
{

}

void FieldDraw(void)
{
	// ワールド行列（原点に置いたまま）
	Shader_SetWorldMatrix(XMMatrixIdentity());

	// マテリアル（色は頂点カラーで付けているので白）
	MATERIAL material{};
	material.Diffuse = { 1.0f, 1.0f, 1.0f, 1.0f };
	material.Ambient = { 1.0f, 1.0f, 1.0f, 1.0f };
	Shader_SetMaterial(material);

	// 頂点バッファとインデックスバッファを描画パイプラインに設定
	UINT stride = sizeof(VERTEX_3D);
	UINT offset = 0;
	DirectXGetDeviceContext()->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);
	DirectXGetDeviceContext()->IASetIndexBuffer(g_pIndexBuffer, DXGI_FORMAT_R16_UINT, 0);

	// プリミティブトポロジの設定
	DirectXGetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// ポリゴン描画命令発行
	DirectXGetDeviceContext()->DrawIndexed(INDEX_COUNT, 0, 0);
}
