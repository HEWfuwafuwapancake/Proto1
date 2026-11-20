#include "render/text2d.h"
#include "core/shader.h"
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

// 調整用の値
static constexpr int FONT_HEIGHTS[TEXT2D_FONT_COUNT] = { 32, 48, 96 }; // 書体ごとの文字画像の高さ（大きく出す書体ほど大きく作る）
static constexpr int BITMAP_PADDING = 2;
static constexpr const char* POP_FONT_FILE = "assets/fonts/MochiyPopOne-Regular.ttf"; // 置けば最優先で使うフォントのファイル
// ポップな見出しに使う書体（前から順に探す）。GDI は日本語の Windows で日本語の書体名を返すので、その名前も持つ。
static constexpr const char* POP_FONT_FACES[][2] = {
	{ "Mochiy Pop One", "モッチーポップ One" },
	{ "HGP創英角ﾎﾟｯﾌﾟ体", "HGP創英角ﾎﾟｯﾌﾟ体" },
};

struct TEXT2D_IMAGE
{
	std::string Text;
	TEXT2D_FONT Font;
	ID3D11ShaderResourceView* View;
	float Aspect;
};

static HDC g_TextDC = nullptr;
static HFONT g_Fonts[TEXT2D_FONT_COUNT] = {};
static HGDIOBJ g_PreviousFont = nullptr;
static bool g_PopFontFileAdded = false;
static ID3D11Buffer* g_VertexBuffer = nullptr;
static std::vector<TEXT2D_IMAGE> g_Images;

static HFONT CreateFace(const char* face, int height, int weight)
{
	return CreateFontA(-height, 0, 0, 0, weight, FALSE, FALSE, FALSE, SHIFTJIS_CHARSET,
		OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH, face);
}

// その書体が本当に入っているか（GDI は無い書体を黙って別の書体に替えるので、選んだ名前を読み直す。英語名・日本語名のどちらでもよい）。
static bool HasFace(HFONT font, const char* const names[2])
{
	if (font == nullptr) return false;
	const HGDIOBJ previous = SelectObject(g_TextDC, font);
	char selected[LF_FACESIZE] = {};
	GetTextFaceA(g_TextDC, LF_FACESIZE, selected);
	SelectObject(g_TextDC, previous);
	return std::strcmp(selected, names[0]) == 0 || std::strcmp(selected, names[1]) == 0;
}

static HFONT CreatePopFont()
{
	if (!g_PopFontFileAdded) {
		g_PopFontFileAdded = AddFontResourceExA(POP_FONT_FILE, FR_PRIVATE, nullptr) > 0;
	}
	for (const auto& names : POP_FONT_FACES) {
		const HFONT font = CreateFace(names[0], FONT_HEIGHTS[TEXT2D_FONT_POP], FW_NORMAL);
		if (HasFace(font, names)) return font;
		if (font != nullptr) DeleteObject(font);
	}
	return CreateFace("Meiryo", FONT_HEIGHTS[TEXT2D_FONT_POP], FW_BOLD);
}

static bool PrepareDrawing()
{
	if (g_VertexBuffer != nullptr) return true;

	g_TextDC = CreateCompatibleDC(nullptr);
	if (g_TextDC == nullptr) return false;
	g_Fonts[TEXT2D_FONT_BODY] = CreateFace("Meiryo", FONT_HEIGHTS[TEXT2D_FONT_BODY], FW_NORMAL);
	g_Fonts[TEXT2D_FONT_BOLD] = CreateFace("Meiryo", FONT_HEIGHTS[TEXT2D_FONT_BOLD], FW_BOLD);
	g_Fonts[TEXT2D_FONT_POP] = CreatePopFont();
	if (g_Fonts[TEXT2D_FONT_BODY] != nullptr) g_PreviousFont = SelectObject(g_TextDC, g_Fonts[TEXT2D_FONT_BODY]);
	if (g_PreviousFont == nullptr) {
		Text2DFinalize();
		return false;
	}
	SetBkMode(g_TextDC, TRANSPARENT);
	SetTextColor(g_TextDC, RGB(255, 255, 255));

	D3D11_BUFFER_DESC buffer{};
	buffer.ByteWidth = sizeof(VERTEX_3D) * 4;
	buffer.Usage = D3D11_USAGE_DYNAMIC;
	buffer.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	buffer.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	if (FAILED(DirectXGetDevice()->CreateBuffer(&buffer, nullptr, &g_VertexBuffer))) {
		Text2DFinalize();
		return false;
	}
	return true;
}

static ID3D11ShaderResourceView* CreateTexture(const uint32_t* pixels, UINT width, UINT height)
{
	D3D11_TEXTURE2D_DESC desc{};
	desc.Width = width;
	desc.Height = height;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.SampleDesc.Count = 1;
	desc.Usage = D3D11_USAGE_IMMUTABLE;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	const D3D11_SUBRESOURCE_DATA data{ pixels, width * 4, 0 };
	ID3D11Texture2D* texture = nullptr;
	ID3D11ShaderResourceView* view = nullptr;
	ID3D11Device* device = DirectXGetDevice();
	if (SUCCEEDED(device->CreateTexture2D(&desc, &data, &texture))) {
		device->CreateShaderResourceView(texture, nullptr, &view);
		SAFE_RELEASE(texture);
	}
	return view;
}

int Text2DCreate(const char* text, TEXT2D_FONT font)
{
	if (text == nullptr || *text == '\0' || font < 0 || font >= TEXT2D_FONT_COUNT) return -1;
	for (size_t i = 0; i < g_Images.size(); ++i) {
		if (g_Images[i].Font == font && g_Images[i].Text == text) return static_cast<int>(i);
	}
	if (!PrepareDrawing()) return -1;
	const HFONT face = g_Fonts[font] != nullptr ? g_Fonts[font] : g_Fonts[TEXT2D_FONT_BODY];
	SelectObject(g_TextDC, face);

	const int length = static_cast<int>(std::strlen(text));
	SIZE size{};
	if (!GetTextExtentPoint32A(g_TextDC, text, length, &size)) return -1;
	const LONG width = size.cx + BITMAP_PADDING * 2;
	const LONG height = size.cy + BITMAP_PADDING * 2;

	BITMAPINFO info{};
	info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	info.bmiHeader.biWidth = width;
	info.bmiHeader.biHeight = -height;
	info.bmiHeader.biPlanes = 1;
	info.bmiHeader.biBitCount = 32;
	info.bmiHeader.biCompression = BI_RGB;
	void* bits = nullptr;
	const HBITMAP bitmap = CreateDIBSection(g_TextDC, &info, DIB_RGB_COLORS, &bits, nullptr, 0);
	if (bitmap == nullptr) return -1;
	const HGDIOBJ previousBitmap = SelectObject(g_TextDC, bitmap);
	ID3D11ShaderResourceView* view = nullptr;
	if (previousBitmap != nullptr) {
		auto* pixels = static_cast<uint32_t*>(bits);
		const size_t count = static_cast<size_t>(width) * height;
		std::memset(pixels, 0, count * sizeof(*pixels));
		const BOOL drawn = TextOutA(g_TextDC, BITMAP_PADDING, BITMAP_PADDING, text, length);
		GdiFlush();
		if (drawn) {
			// 白黒の濃さをアルファに移し、RGB は白にそろえる。
			for (size_t i = 0; i < count; ++i) pixels[i] = 0x00ffffffu | ((pixels[i] & 0xffu) << 24);
			view = CreateTexture(pixels, static_cast<UINT>(width), static_cast<UINT>(height));
		}
		SelectObject(g_TextDC, previousBitmap);
	}
	DeleteObject(bitmap);
	if (view == nullptr) return -1;

	g_Images.push_back({ text, font, view, static_cast<float>(width) / static_cast<float>(height) });
	return static_cast<int>(g_Images.size() - 1);
}

void Text2DFinalize(void)
{
	for (TEXT2D_IMAGE& image : g_Images) SAFE_RELEASE(image.View);
	g_Images.clear();
	SAFE_RELEASE(g_VertexBuffer);
	if (g_PreviousFont != nullptr) SelectObject(g_TextDC, g_PreviousFont);
	for (HFONT& font : g_Fonts) {
		if (font != nullptr) DeleteObject(font);
		font = nullptr;
	}
	if (g_TextDC != nullptr) DeleteDC(g_TextDC);
	if (g_PopFontFileAdded) RemoveFontResourceExA(POP_FONT_FILE, FR_PRIVATE, nullptr);
	g_PopFontFileAdded = false;
	g_PreviousFont = nullptr;
	g_TextDC = nullptr;
}

float Text2DGetAspect(int id)
{
	if (id < 0 || static_cast<size_t>(id) >= g_Images.size()) return 0.0f;
	return g_Images[id].Aspect;
}

void Text2DDraw(int id, float x, float y, float height, GLYPH_ALIGN align, const XMFLOAT4& color, float fullHeight)
{
	if (id < 0 || static_cast<size_t>(id) >= g_Images.size() || height <= 0.0f) return;
	const float width = g_Images[id].Aspect * (fullHeight > 0.0f ? fullHeight : height);
	if (align == GLYPH_ALIGN_CENTER) x -= width * 0.5f;
	if (align == GLYPH_ALIGN_RIGHT) x -= width;
	Text2DDrawQuad(id, QuadCalcRect({ x, y, width, height }), color);
}

void Text2DDrawQuad(int id, const QUAD2D& quad, const XMFLOAT4& color)
{
	if (id < 0 || static_cast<size_t>(id) >= g_Images.size() || g_VertexBuffer == nullptr) return;
	const TEXT2D_IMAGE& image = g_Images[id];
	const XMFLOAT3 normal{ 0.0f, 0.0f, -1.0f };
	const VERTEX_3D vertices[] = {
		{ { quad.X[0], quad.Y[0], 0.0f }, normal, color, { 0.0f, 0.0f } },
		{ { quad.X[1], quad.Y[1], 0.0f }, normal, color, { 1.0f, 0.0f } },
		{ { quad.X[2], quad.Y[2], 0.0f }, normal, color, { 0.0f, 1.0f } },
		{ { quad.X[3], quad.Y[3], 0.0f }, normal, color, { 1.0f, 1.0f } },
	};

	ID3D11DeviceContext* context = DirectXGetDeviceContext();
	D3D11_MAPPED_SUBRESOURCE mapped{};
	if (FAILED(context->Map(g_VertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) return;
	std::memcpy(mapped.pData, vertices, sizeof(vertices));
	context->Unmap(g_VertexBuffer, 0);

	Shader_Set2DMatrix();
	MATERIAL material{};
	material.Diffuse = { 1.0f, 1.0f, 1.0f, 1.0f };
	material.Ambient = material.Diffuse;
	material.TextureEnable = TRUE;
	Shader_SetMaterial(material);
	context->PSSetShaderResources(0, 1, &image.View);
	const UINT stride = sizeof(VERTEX_3D);
	const UINT offset = 0;
	context->IASetVertexBuffers(0, 1, &g_VertexBuffer, &stride, &offset);
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	context->Draw(4, 0);
	ID3D11ShaderResourceView* empty = nullptr;
	context->PSSetShaderResources(0, 1, &empty);
}
