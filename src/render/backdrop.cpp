#include "render/backdrop.h"
#include "core/shader.h"
#include <cstring>

// 調整用の値
static constexpr int LEVEL_COUNT = 4; // 640×360 → 320×180 → 160×90 → 80×45 と半分ずつ縮める
static constexpr int DRAW_LEVEL = 2; // 画面に貼る段（160×90。小さいほど強くぼける）
static constexpr float JITTER = 7.0f; // 貼るときに上下左右へずらして重ねる量（ゲームの画面のピクセル。段の境目をなめらかにする）
static constexpr float SKY_COLOR[4] = { 1.0f, 0.62f, 0.84f, 1.0f }; // 背景の空の色（メニューのピンクに合わせる）

struct BACKDROP_LEVEL
{
	ID3D11Texture2D* Texture = nullptr;
	ID3D11RenderTargetView* Target = nullptr;
	ID3D11ShaderResourceView* View = nullptr;
	UINT Width = 0;
	UINT Height = 0;
};

static BACKDROP_LEVEL g_Levels[LEVEL_COUNT];
static ID3D11Texture2D* g_DepthTexture = nullptr;
static ID3D11DepthStencilView* g_DepthView = nullptr;
static ID3D11SamplerState* g_ClampSampler = nullptr; // 端の外を反対側から拾わないように、端の色で止める
static ID3D11Buffer* g_VertexBuffer = nullptr;
static bool g_Drawn = false;

static bool CreateLevel(BACKDROP_LEVEL& level, UINT width, UINT height)
{
	ID3D11Device* device = DirectXGetDevice();
	D3D11_TEXTURE2D_DESC desc{};
	desc.Width = width;
	desc.Height = height;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.SampleDesc.Count = 1;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	if (FAILED(device->CreateTexture2D(&desc, nullptr, &level.Texture))) return false;
	if (FAILED(device->CreateRenderTargetView(level.Texture, nullptr, &level.Target))) return false;
	if (FAILED(device->CreateShaderResourceView(level.Texture, nullptr, &level.View))) return false;
	level.Width = width;
	level.Height = height;
	return true;
}

bool BackdropInitialize(void)
{
	if (g_VertexBuffer != nullptr) return true;
	ID3D11Device* device = DirectXGetDevice();
	if (device == nullptr) return false;
	for (int i = 0; i < LEVEL_COUNT; ++i) {
		if (!CreateLevel(g_Levels[i], BACKDROP_WIDTH >> i, BACKDROP_HEIGHT >> i)) {
			BackdropFinalize();
			return false;
		}
	}
	D3D11_TEXTURE2D_DESC depth{};
	depth.Width = BACKDROP_WIDTH;
	depth.Height = BACKDROP_HEIGHT;
	depth.MipLevels = 1;
	depth.ArraySize = 1;
	depth.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depth.SampleDesc.Count = 1;
	depth.Usage = D3D11_USAGE_DEFAULT;
	depth.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	D3D11_SAMPLER_DESC sampler{};
	sampler.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sampler.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampler.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampler.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampler.ComparisonFunc = D3D11_COMPARISON_NEVER;
	sampler.MaxLOD = D3D11_FLOAT32_MAX;
	D3D11_BUFFER_DESC buffer{};
	buffer.ByteWidth = sizeof(VERTEX_3D) * 4;
	buffer.Usage = D3D11_USAGE_DYNAMIC;
	buffer.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	buffer.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	if (FAILED(device->CreateTexture2D(&depth, nullptr, &g_DepthTexture))
		|| FAILED(device->CreateDepthStencilView(g_DepthTexture, nullptr, &g_DepthView))
		|| FAILED(device->CreateSamplerState(&sampler, &g_ClampSampler))
		|| FAILED(device->CreateBuffer(&buffer, nullptr, &g_VertexBuffer))) {
		BackdropFinalize();
		return false;
	}
	return true;
}

void BackdropFinalize(void)
{
	for (BACKDROP_LEVEL& level : g_Levels) {
		SAFE_RELEASE(level.View);
		SAFE_RELEASE(level.Target);
		SAFE_RELEASE(level.Texture);
		level = {};
	}
	SAFE_RELEASE(g_DepthView);
	SAFE_RELEASE(g_DepthTexture);
	SAFE_RELEASE(g_ClampSampler);
	SAFE_RELEASE(g_VertexBuffer);
	g_Drawn = false;
}

static void SetTargetViewport(UINT width, UINT height)
{
	D3D11_VIEWPORT viewport{};
	viewport.Width = static_cast<float>(width);
	viewport.Height = static_cast<float>(height);
	viewport.MaxDepth = 1.0f;
	DirectXGetDeviceContext()->RSSetViewports(1, &viewport);
}

// 1280×720 の座標の四角に画像を貼る（今のビューポートいっぱいが 1280×720 になる）。
static void DrawImage(ID3D11ShaderResourceView* view, float x, float y, float width, float height, const XMFLOAT4& color)
{
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
	context->PSSetShaderResources(0, 1, &view);
	const UINT stride = sizeof(VERTEX_3D);
	const UINT offset = 0;
	context->IASetVertexBuffers(0, 1, &g_VertexBuffer, &stride, &offset);
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	context->Draw(4, 0);
	ID3D11ShaderResourceView* empty = nullptr;
	context->PSSetShaderResources(0, 1, &empty);
}

bool BackdropBegin(void)
{
	g_Drawn = false;
	if (!BackdropInitialize()) return false;
	ID3D11DeviceContext* context = DirectXGetDeviceContext();
	context->OMSetRenderTargets(1, &g_Levels[0].Target, g_DepthView);
	context->ClearRenderTargetView(g_Levels[0].Target, SKY_COLOR);
	context->ClearDepthStencilView(g_DepthView, D3D11_CLEAR_DEPTH, 1.0f, 0);
	SetTargetViewport(g_Levels[0].Width, g_Levels[0].Height);
	SetDepthEnable(true);
	return true;
}

void BackdropEnd(void)
{
	ID3D11DeviceContext* context = DirectXGetDeviceContext();
	ID3D11SamplerState* previous = nullptr;
	context->PSGetSamplers(0, 1, &previous);
	context->PSSetSamplers(0, 1, &g_ClampSampler);
	SetDepthEnable(false);
	Shader_Begin();
	LIGHT light{};
	Shader_SetLight(light);
	// 1段ずつ半分の大きさへ描き写す（線形補間で 2×2 の平均になり、少しずつぼける）。
	for (int i = 1; i < LEVEL_COUNT; ++i) {
		context->OMSetRenderTargets(1, &g_Levels[i].Target, nullptr);
		SetTargetViewport(g_Levels[i].Width, g_Levels[i].Height);
		DrawImage(g_Levels[i - 1].View, 0.0f, 0.0f, static_cast<float>(SCREEN_WIDTH), static_cast<float>(SCREEN_HEIGHT),
			{ 1.0f, 1.0f, 1.0f, 1.0f });
	}
	context->PSSetSamplers(0, 1, &previous);
	SAFE_RELEASE(previous);
	SetRenderTarget();
	g_Drawn = true;
}

void BackdropDraw(const XMFLOAT4& tint)
{
	if (!g_Drawn) return;
	ID3D11DeviceContext* context = DirectXGetDeviceContext();
	ID3D11SamplerState* previous = nullptr;
	context->PSGetSamplers(0, 1, &previous);
	context->PSSetSamplers(0, 1, &g_ClampSampler);
	// 小さな段を少しずつずらして重ね、引き伸ばしたときの角ばりを消す（2枚目から 1/2・1/3・1/4 の濃さで重ねると平均になる）。
	static constexpr float OFFSETS[4][2] = { { -1.0f, -1.0f }, { 1.0f, -1.0f }, { -1.0f, 1.0f }, { 1.0f, 1.0f } };
	const float width = static_cast<float>(SCREEN_WIDTH) + JITTER * 2.0f;
	const float height = static_cast<float>(SCREEN_HEIGHT) + JITTER * 2.0f;
	for (int i = 0; i < 4; ++i) {
		const XMFLOAT4 color{ tint.x, tint.y, tint.z, tint.w / static_cast<float>(i + 1) };
		DrawImage(g_Levels[DRAW_LEVEL].View, -JITTER + OFFSETS[i][0] * JITTER * 0.5f, -JITTER + OFFSETS[i][1] * JITTER * 0.5f,
			width, height, color);
	}
	context->PSSetSamplers(0, 1, &previous);
	SAFE_RELEASE(previous);
}
