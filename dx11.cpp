
#include "dx11.h"

//スタティックリンクライブラリのリンク
#pragma comment (lib, "d3d11.lib")

static ID3D11Device* g_Device = NULL;
static ID3D11DeviceContext* g_DeviceContext = NULL;
static IDXGISwapChain* g_SwapChain = NULL;
static ID3D11Texture2D* g_DepthStencilTexture = NULL;
static ID3D11RenderTargetView* g_RenderTargetView = NULL;
static ID3D11DepthStencilView* g_DepthStencilView = NULL;

static ID3D11RasterizerState* g_RasterizerState = NULL;
static ID3D11BlendState* g_BlendState = NULL;
static ID3D11DepthStencilState* g_DepthStencilStateDepthEnable = NULL;	// 3D用（深度テストあり）
static ID3D11DepthStencilState* g_DepthStencilStateDepthDisable = NULL;	// 2D用（深度テストなし）

// 垂直同期：モニターが画面を書き換えるタイミングに合わせて表示する
// true にすると画面のちらつき（上下で絵がずれる「ティアリング」）が出ない。
// FPS が 60 を大きく下回る PC で、さらにカクつくようなら false にして試す
static constexpr bool VSYNC = true;


bool DirectXInitialize(HWND hWnd)
{
	// デバイス、スワップチェイン、コンテキスト生成
	DXGI_SWAP_CHAIN_DESC sd = {};
	sd.BufferCount = 1;
	sd.BufferDesc.Width = SCREEN_WIDTH;
	sd.BufferDesc.Height = SCREEN_HEIGHT;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.OutputWindow = hWnd;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.Windowed = TRUE;

	D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;

	// Debugビルドのときは、DirectXの使い方の間違いを出力ウィンドウに表示する（デバッグレイヤー）
	UINT createFlags = 0;
#if defined(_DEBUG)
	createFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	HRESULT hr = D3D11CreateDeviceAndSwapChain(NULL,
		D3D_DRIVER_TYPE_HARDWARE,
		NULL,
		createFlags,
		NULL,
		0,
		D3D11_SDK_VERSION,
		&sd,
		&g_SwapChain,
		&g_Device,
		&featureLevel,
		&g_DeviceContext);

#if defined(_DEBUG)
	// デバッグレイヤーが入っていないPCでは失敗するので、外してもう一度作る
	if (FAILED(hr)) {
		hr = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, NULL, 0,
			D3D11_SDK_VERSION, &sd, &g_SwapChain, &g_Device, &featureLevel, &g_DeviceContext);
	}
#endif

	if (FAILED(hr)) {
		MessageBox(hWnd, "DirectXのデバイスの作成に失敗しました", "エラー", MB_OK);
		return false;
	}

	// レンダーターゲットビュー生成
	ID3D11Texture2D* backBuffer = NULL;
	g_SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&backBuffer);
	hr = g_Device->CreateRenderTargetView(backBuffer, NULL, &g_RenderTargetView);
	backBuffer->Release();
	if (FAILED(hr)) {
		MessageBox(hWnd, "レンダーターゲットビューの作成に失敗しました", "エラー", MB_OK);
		return false;
	}

	// デプスステンシル用テクスチャー作成
	D3D11_TEXTURE2D_DESC td = {};
	td.Width = sd.BufferDesc.Width;
	td.Height = sd.BufferDesc.Height;
	td.MipLevels = 1;
	td.ArraySize = 1;
	td.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	td.SampleDesc = sd.SampleDesc;
	td.Usage = D3D11_USAGE_DEFAULT;
	td.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	td.CPUAccessFlags = 0;
	td.MiscFlags = 0;
	hr = g_Device->CreateTexture2D(&td, NULL, &g_DepthStencilTexture);
	if (FAILED(hr)) {
		MessageBox(hWnd, "デプスステンシル用テクスチャーの作成に失敗しました", "エラー", MB_OK);
		return false;
	}

	// デプスステンシルターゲット作成
	D3D11_DEPTH_STENCIL_VIEW_DESC dsvd = {};
	dsvd.Format = td.Format;
	dsvd.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	dsvd.Flags = 0;
	hr = g_Device->CreateDepthStencilView(g_DepthStencilTexture,
		&dsvd, &g_DepthStencilView);
	if (FAILED(hr)) {
		MessageBox(hWnd, "デプスステンシルビューの作成に失敗しました", "エラー", MB_OK);
		return false;
	}

	// デバイスコンテキストへレンダーターゲットビューとデプスステンシルビューを設定
	g_DeviceContext->OMSetRenderTargets(1, &g_RenderTargetView, g_DepthStencilView);

	// ビューポート設定
	D3D11_VIEWPORT vp;
	vp.Width = (FLOAT)SCREEN_WIDTH;
	vp.Height = (FLOAT)SCREEN_HEIGHT;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	g_DeviceContext->RSSetViewports(1, &vp);

	// ラスタライザステートの作成
	D3D11_RASTERIZER_DESC rd = {};
	rd.FillMode = D3D11_FILL_SOLID;
	rd.CullMode = D3D11_CULL_BACK;//バックフェースカリング（裏から見た面は描かない。表は時計回り）
	rd.DepthClipEnable = TRUE;
	rd.MultisampleEnable = FALSE;
	g_Device->CreateRasterizerState(&rd, &g_RasterizerState);

	// デバイスコンテキストにラスタライザーステートを設定
	g_DeviceContext->RSSetState(g_RasterizerState);

	// ブレンドステート設定
	D3D11_BLEND_DESC bd = {};
	bd.AlphaToCoverageEnable = FALSE;
	bd.IndependentBlendEnable = FALSE;
	bd.RenderTarget[0].BlendEnable = TRUE;

	bd.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	bd.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	bd.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;

	bd.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	bd.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	bd.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;

	bd.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	float blend_factor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	g_Device->CreateBlendState(&bd, &g_BlendState);

	g_DeviceContext->OMSetBlendState(g_BlendState, blend_factor, 0xffffffff);



	// 深度ステンシルステート設定
	D3D11_DEPTH_STENCIL_DESC dsd = {};
	dsd.DepthEnable = TRUE;		// 3D用：手前のものだけが見えるように深度テストをする
	dsd.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	dsd.DepthFunc = D3D11_COMPARISON_LESS;
	dsd.StencilEnable = FALSE;
	g_Device->CreateDepthStencilState(&dsd, &g_DepthStencilStateDepthEnable);

	dsd.DepthEnable = FALSE;	// 2D用：描いた順に上へ重ねる
	dsd.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	g_Device->CreateDepthStencilState(&dsd, &g_DepthStencilStateDepthDisable);

	SetDepthEnable(true);

	return true;
}

void DirectXFinalize(void)
{
	SAFE_RELEASE(g_DepthStencilStateDepthDisable);
	SAFE_RELEASE(g_DepthStencilStateDepthEnable);
	SAFE_RELEASE(g_BlendState);
	SAFE_RELEASE(g_RasterizerState);

	SAFE_RELEASE(g_DepthStencilView);
	SAFE_RELEASE(g_DepthStencilTexture);
	SAFE_RELEASE(g_RenderTargetView);
	SAFE_RELEASE(g_SwapChain);
	SAFE_RELEASE(g_DeviceContext);
	SAFE_RELEASE(g_Device);
}

ID3D11Device* DirectXGetDevice(void)
{
	return g_Device;
}

ID3D11DeviceContext* DirectXGetDeviceContext(void)
{
	return g_DeviceContext;
}


// レンダーターゲット設定
void SetRenderTarget()
{
	g_DeviceContext->OMSetRenderTargets(1, &g_RenderTargetView, g_DepthStencilView);
}


// 深度テストの切り替え
void SetDepthEnable(bool enable)
{
	if (enable) {
		g_DeviceContext->OMSetDepthStencilState(g_DepthStencilStateDepthEnable, NULL);
	}
	else {
		g_DeviceContext->OMSetDepthStencilState(g_DepthStencilStateDepthDisable, NULL);
	}
}


//=============================================================================
// バックバッファクリア
//=============================================================================
void Clear(void)
{
	// バックバッファクリア（空の色）
	float clearColor[4] = { 0.4f, 0.6f, 0.9f, 1.0f };
	g_DeviceContext->ClearRenderTargetView(g_RenderTargetView, clearColor);
	g_DeviceContext->ClearDepthStencilView(g_DepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);
}


//=============================================================================
// プレゼント
//=============================================================================
void Present(void)
{
	// バックバッファとフロントバッファの交換（垂直同期ありなら、モニターの書き換えを待ってから）
	g_SwapChain->Present(VSYNC ? 1 : 0, 0);
}
