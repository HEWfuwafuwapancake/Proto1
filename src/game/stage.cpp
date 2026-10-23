#include "core/dx11.h"
#include "core/shader.h"
#include "game/camera.h"
#include "game/stage.h"

#ifdef _DEBUG
#include <iostream>
#include "core/debug_ostream.h"
#endif


//------------------------------------------------------------------------------
//	調整用の値
//------------------------------------------------------------------------------
static constexpr float STAGE_WIDTH = 30.0f;		// 浮島の横幅（X）
static constexpr float STAGE_DEPTH = 30.0f;		// 浮島の奥行き（Z）
static constexpr float STAGE_THICKNESS = 2.0f;	// 浮島の厚さ（見た目だけ）
static constexpr float STAGE_TOP_Y = 0.0f;		// 浮島の上の面の高さ
static constexpr float OUT_Y = -15.0f;			// 場外の下の線（これより下に落ちたら場外）
static constexpr XMFLOAT4 STAGE_COLOR = { 0.45f, 0.65f, 0.4f, 1.0f };	// 浮島の色（仮。くすんだ緑）


static ID3D11Buffer* g_pVertexBuffer = nullptr;	// 頂点バッファ
static ID3D11Buffer* g_pIndexBuffer = nullptr;	// インデックスバッファ

static constexpr int FACE_COUNT = 6;
static constexpr int VERTEX_COUNT = FACE_COUNT * 4;	// 法線を面ごとに変えるため、角の頂点は面ごとに別に持つ
static constexpr int INDEX_COUNT = FACE_COUNT * 6;	// 1面 = 三角形2枚 = 6インデックス

static STAGE g_Stage = {};		// 浮島と場外の線

#ifdef _DEBUG
static bool g_CameraWasOut = false;	// 確かめ用：前のフレームでカメラが場外だったか
#endif


void StageInitialize(void)
{
	g_Stage.CenterX = 0.0f;
	g_Stage.CenterZ = 0.0f;
	g_Stage.HalfWidth = STAGE_WIDTH * 0.5f;
	g_Stage.HalfDepth = STAGE_DEPTH * 0.5f;
	g_Stage.TopY = STAGE_TOP_Y;
	g_Stage.Thickness = STAGE_THICKNESS;
	g_Stage.OutY = OUT_Y;

	// 1辺 1.0 の箱を作り、描くときに浮島の大きさに広げる（作り方は cube.cpp と同じ）
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
		XMVECTOR c = n * 0.5f;	// 面の中心

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

#ifdef _DEBUG
	g_CameraWasOut = false;
#endif
}

void StageFinalize(void)
{
	SAFE_RELEASE(g_pIndexBuffer);
	SAFE_RELEASE(g_pVertexBuffer);
}

void StageUpdate(void)
{
#ifdef _DEBUG
	// 確かめ用：カメラが場外の下の線を越えた・戻ったときに出力ウィンドウに出す（No.8 でプレイヤーに置き換える）
	const bool out = StageCalcIsOut(g_Stage, CameraGetPosition().y);
	if (out != g_CameraWasOut) {
		hal::dout << (out ? "ステージ：場外（カメラ）" : "ステージ：場内に戻った（カメラ）") << std::endl;
		g_CameraWasOut = out;
	}
#endif
}

void StageDraw(void)
{
	// 1辺 1.0 の箱を浮島の大きさに広げて、上の面が TopY になるように置く
	const float centerY = g_Stage.TopY - g_Stage.Thickness * 0.5f;
	Shader_SetWorldMatrix(
		XMMatrixScaling(g_Stage.HalfWidth * 2.0f, g_Stage.Thickness, g_Stage.HalfDepth * 2.0f)
		* XMMatrixTranslation(g_Stage.CenterX, centerY, g_Stage.CenterZ));

	// マテリアル
	MATERIAL material{};
	material.Diffuse = STAGE_COLOR;
	material.Ambient = STAGE_COLOR;
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

const STAGE& StageGet(void)
{
	return g_Stage;
}
