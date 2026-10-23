#include "core/main.h"
#include "core/dx11.h"
#include "core/shader.h"
#include "render/number2d.h"


static constexpr int VERTICES_PER_RECT = 6;	// 四角1つ ＝ 三角形2つ（三角形リスト）

static ID3D11Buffer* g_pVertexBuffer = nullptr;	// 頂点バッファ（描くたびに書き換える）


void Number2DInitialize(void)
{
	D3D11_BUFFER_DESC bd{};
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(VERTEX_3D) * NUMBER2D_MAX_RECTS * VERTICES_PER_RECT;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	DirectXGetDevice()->CreateBuffer(&bd, NULL, &g_pVertexBuffer);
}

void Number2DFinalize(void)
{
	SAFE_RELEASE(g_pVertexBuffer);
}

void Number2DDrawRects(const RECT2D rects[], int count, const XMFLOAT4& color)
{
	if (g_pVertexBuffer == nullptr || count <= 0) return;
	if (count > NUMBER2D_MAX_RECTS) count = NUMBER2D_MAX_RECTS;

	// 頂点バッファに四角を書く
	D3D11_MAPPED_SUBRESOURCE msr;
	if (FAILED(DirectXGetDeviceContext()->Map(g_pVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr))) return;
	VERTEX_3D* v = (VERTEX_3D*)msr.pData;
	for (int i = 0; i < count; i++) {
		const float left = rects[i].X;
		const float top = rects[i].Y;
		const float right = rects[i].X + rects[i].Width;
		const float bottom = rects[i].Y + rects[i].Height;

		// 画面で見て時計回り（左上 → 右上 → 左下、右上 → 右下 → 左下）
		const XMFLOAT3 corners[VERTICES_PER_RECT] = {
			{ left,  top,    0.0f }, { right, top,    0.0f }, { left,  bottom, 0.0f },
			{ right, top,    0.0f }, { right, bottom, 0.0f }, { left,  bottom, 0.0f },
		};
		for (int k = 0; k < VERTICES_PER_RECT; k++) {
			VERTEX_3D& vertex = v[i * VERTICES_PER_RECT + k];
			vertex.Position = corners[k];
			vertex.Normal = { 0.0f, 0.0f, -1.0f };	// 2D はライトを使わないので使われない
			vertex.Diffuse = color;
			vertex.TexCoord = { 0.0f, 0.0f };
		}
	}
	DirectXGetDeviceContext()->Unmap(g_pVertexBuffer, 0);

	//2Dマトリクス設定
	Shader_Set2DMatrix();

	// マテリアル（白。頂点カラーをそのまま使う）
	MATERIAL material{};
	material.Diffuse = { 1.0f, 1.0f, 1.0f, 1.0f };
	material.Ambient = { 1.0f, 1.0f, 1.0f, 1.0f };
	Shader_SetMaterial(material);

	UINT stride = sizeof(VERTEX_3D);
	UINT offset = 0;
	DirectXGetDeviceContext()->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);
	DirectXGetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	DirectXGetDeviceContext()->Draw(count * VERTICES_PER_RECT, 0);
}

void Number2DDrawNumber(int value, int minDigits, float x, float y, float digitWidth, GLYPH_ALIGN align, const XMFLOAT4& color)
{
	RECT2D rects[NUMBER2D_MAX_RECTS];
	const int count = GlyphCalcNumberRects(value, minDigits, x, y, digitWidth, align, rects, NUMBER2D_MAX_RECTS);
	Number2DDrawRects(rects, count, color);
}

void Number2DDrawText(const char* text, float x, float y, float dotSize, GLYPH_ALIGN align, const XMFLOAT4& color)
{
	RECT2D rects[NUMBER2D_MAX_RECTS];
	const int count = GlyphCalcTextRects(text, x, y, dotSize, align, rects, NUMBER2D_MAX_RECTS);
	Number2DDrawRects(rects, count, color);
}

void Number2DDrawBox(const RECT2D& rect, const XMFLOAT4& color)
{
	Number2DDrawRects(&rect, 1, color);
}
