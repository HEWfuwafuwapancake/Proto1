#include "render/text2d.h"
#include "core/shader.h"
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

// 調整用の値
static constexpr int FONT_HEIGHT = 32;
static constexpr int BITMAP_PADDING = 2;

struct TEXT2D_IMAGE
{
	std::string Text;
	ID3D11ShaderResourceView* View;
	float Aspect;
};

static HDC g_TextDC = nullptr;
static HFONT g_Font = nullptr;
static HGDIOBJ g_PreviousFont = nullptr;
static ID3D11Buffer* g_VertexBuffer = nullptr;
static std::vector<TEXT2D_IMAGE> g_Images;

static bool PrepareDrawing()
{
	if (g_VertexBuffer != nullptr) return true;

	g_TextDC = CreateCompatibleDC(nullptr);
	if (g_TextDC == nullptr) return false;
	g_Font = CreateFontA(-FONT_HEIGHT, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, SHIFTJIS_CHARSET,
		OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH, "Meiryo");
	if (g_Font != nullptr) g_PreviousFont = SelectObject(g_TextDC, g_Font);
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

int Text2DCreate(const char* text)
{
	if (text == nullptr || *text == '\0') return -1;
	for (size_t i = 0; i < g_Images.size(); ++i) {
		if (g_Images[i].Text == text) return static_cast<int>(i);
	}
	if (!PrepareDrawing()) return -1;

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

	g_Images.push_back({ text, view, static_cast<float>(width) / static_cast<float>(height) });
	return static_cast<int>(g_Images.size() - 1);
}

void Text2DFinalize(void)
{
	for (TEXT2D_IMAGE& image : g_Images) SAFE_RELEASE(image.View);
	g_Images.clear();
	SAFE_RELEASE(g_VertexBuffer);
	if (g_PreviousFont != nullptr) SelectObject(g_TextDC, g_PreviousFont);
	if (g_Font != nullptr) DeleteObject(g_Font);
	if (g_TextDC != nullptr) DeleteDC(g_TextDC);
	g_PreviousFont = nullptr;
	g_Font = nullptr;
	g_TextDC = nullptr;
}

void Text2DDraw(int id, float x, float y, float height, GLYPH_ALIGN align, const XMFLOAT4& color, float fullHeight)
{
	if (id < 0 || static_cast<size_t>(id) >= g_Images.size() || height <= 0.0f) return;
	const TEXT2D_IMAGE& image = g_Images[id];
	const float width = image.Aspect * (fullHeight > 0.0f ? fullHeight : height);
	if (align == GLYPH_ALIGN_CENTER) x -= width * 0.5f;
	if (align == GLYPH_ALIGN_RIGHT) x -= width;

	const XMFLOAT3 normal{ 0.0f, 0.0f, -1.0f };
	const VERTEX_3D vertices[] = {
		{ { x, y, 0.0f }, normal, color, { 0.0f, 0.0f } },
		{ { x + width, y, 0.0f }, normal, color, { 1.0f, 0.0f } },
		{ { x, y + height, 0.0f }, normal, color, { 0.0f, 1.0f } },
		{ { x + width, y + height, 0.0f }, normal, color, { 1.0f, 1.0f } },
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
