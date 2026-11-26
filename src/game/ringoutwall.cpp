#include "game/ringoutwall.h"
#include "core/dx11.h"
#include "core/shader.h"
#include "render/texture.h"
#include <math.h>


//------------------------------------------------------------------------------
//	調整用の値
//------------------------------------------------------------------------------
static constexpr const char* TEXTURE_PATH = "assets/textures/deadline.png";	// 帯1組の画像（文字の高さ:上下の帯 = 3:1、横は文字の2倍）
static constexpr float CHARACTER_HEIGHT = 1.8f;		// キャラ1体分の高さ（MEKAGIRL）
static constexpr float BAND_HEIGHT = CHARACTER_HEIGHT * 224.0f / 118.0f;	// 帯の高さ（画像の文字の高さ 118px をキャラ1体分にする）
static constexpr float BAND_WIDTH = BAND_HEIGHT * 1982.0f / 224.0f;	// 帯1組の横幅（画像の縦横の比のまま）
static constexpr float BAND_ALPHA = 0.6f;			// 帯全体の濃さ（半透明）
static constexpr int ROUND_SEGMENTS = 48;			// 円い壁を何枚の板で作るか
static constexpr int MAX_CORNERS = 64;				// 壁の角の最大数
static constexpr int VERTICES_PER_PANEL = 6;		// 板1枚の頂点（三角形2枚）


void RingOutWall::Initialize(void)
{
	m_Texture = TextureLoadFromFile(TEXTURE_PATH);
	D3D11_BUFFER_DESC bd{};
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(VERTEX_3D) * MAX_CORNERS * VERTICES_PER_PANEL;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	DirectXGetDevice()->CreateBuffer(&bd, nullptr, &m_VertexBuffer);
}

void RingOutWall::Finalize(void)
{
	SAFE_RELEASE(m_VertexBuffer);
	SAFE_RELEASE(m_Texture);
}

void RingOutWall::Draw(const STAGE& stage) const
{
	if (!m_Texture || !m_VertexBuffer) return;
	float corners[MAX_CORNERS][2];
	const int count = StageCalcRingOutOutline(stage, corners, MAX_CORNERS, ROUND_SEGMENTS);
	if (count < 3) return;
	const float bottom = StageCalcLowestSpawnY(stage) + CHARACTER_HEIGHT;
	const float top = bottom + BAND_HEIGHT;

	// 一周を1本の帯として、最初の角から測った長さで横の画像の位置を決める（画像は横にくり返すので、角をまたいでも途切れない）。
	ID3D11DeviceContext* context = DirectXGetDeviceContext();
	D3D11_MAPPED_SUBRESOURCE mapped{};
	if (FAILED(context->Map(m_VertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) return;
	VERTEX_3D* vertex = static_cast<VERTEX_3D*>(mapped.pData);
	const float total = [&] {
		float length = 0.0f;
		for (int i = 0; i < count; ++i) {
			const int next = (i + 1) % count;
			length += hypotf(corners[next][0] - corners[i][0], corners[next][1] - corners[i][1]);
		}
		return length;
	}();
	const float whole = floorf(total / BAND_WIDTH);	// 一周に入る完全な組の数（端数の組は切る）
	const XMFLOAT4 color = { 1.0f, 1.0f, 1.0f, BAND_ALPHA };
	float along = 0.0f;
	int n = 0;
	for (int i = 0; i < count; ++i) {
		const int next = (i + 1) % count;
		const float x0 = corners[i][0], z0 = corners[i][1], x1 = corners[next][0], z1 = corners[next][1];
		const float length = hypotf(x1 - x0, z1 - z0);
		const float u0 = along / BAND_WIDTH;
		const float u1 = (along + length) / BAND_WIDTH;
		along += length;
		// 一周の最後に余る端数の組は切る：最後の完全な組の終わりより先は描かない（最初の角の手前に隙間が1つできる）。
		if (u0 >= whole) continue;
		float drawU1 = u1, drawX1 = x1, drawZ1 = z1;
		if (u1 > whole) {
			const float rate = (whole - u0) / (u1 - u0);
			drawU1 = whole;
			drawX1 = x0 + (x1 - x0) * rate;
			drawZ1 = z0 + (z1 - z0) * rate;
		}
		// 内側（ステージの中心の向き）を表にする。角は上から見て反時計回りなので、左手が内側。
		const XMFLOAT3 normal = { -(z1 - z0) / length, 0.0f, (x1 - x0) / length };
		// 内側から見ると一周の向きは右から左なので、画像の横を逆に貼って文字を左から読めるようにする（くり返すので負でよい）。
		const VERTEX_3D a{ { x0, top, z0 }, normal, color, { -u0, 0.0f } };
		const VERTEX_3D b{ { drawX1, top, drawZ1 }, normal, color, { -drawU1, 0.0f } };
		const VERTEX_3D c{ { x0, bottom, z0 }, normal, color, { -u0, 1.0f } };
		const VERTEX_3D d{ { drawX1, bottom, drawZ1 }, normal, color, { -drawU1, 1.0f } };
		vertex[n++] = a; vertex[n++] = c; vertex[n++] = b;
		vertex[n++] = b; vertex[n++] = c; vertex[n++] = d;
	}
	context->Unmap(m_VertexBuffer, 0);
	if (n == 0) return;

	Shader_Begin();
	Shader_SetWorldMatrix(XMMatrixIdentity());
	MATERIAL material{};
	material.Diffuse = { 1.0f, 1.0f, 1.0f, 1.0f };
	material.Ambient = { 1.0f, 1.0f, 1.0f, 1.0f };
	material.TextureEnable = TRUE;
	Shader_SetMaterial(material);
	Shader_SetLightEnable(false);
	ID3D11ShaderResourceView* texture = m_Texture;
	context->PSSetShaderResources(0, 1, &texture);
	const UINT stride = sizeof(VERTEX_3D), offset = 0;
	ID3D11Buffer* buffer = m_VertexBuffer;
	context->IASetVertexBuffers(0, 1, &buffer, &stride, &offset);
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	// 景観より奥にあっても見えるよう、深度を見ずに重ねる（深度も書かないので、あとで描く体・腕は帯の手前に出る）。
	SetDepthEnable(false);
	context->Draw(n, 0);
	SetDepthEnable(true);
	ID3D11ShaderResourceView* empty = nullptr;
	context->PSSetShaderResources(0, 1, &empty);
	Shader_SetLightEnable(true);
}
