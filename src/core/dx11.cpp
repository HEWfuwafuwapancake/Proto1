
#include "core/dx11.h"
#include "core/letterboxcalc.h"
#include <d3d11_1.h>

//スタティックリンクライブラリのリンク
#pragma comment (lib, "d3d11.lib")

static ID3D11Device* g_Device = NULL;
static ID3D11DeviceContext* g_DeviceContext = NULL;
static IDXGISwapChain* g_SwapChain = NULL;
static ID3D11Texture2D* g_DepthStencilTexture = NULL;
static ID3D11RenderTargetView* g_RenderTargetView = NULL;
static ID3D11DepthStencilView* g_DepthStencilView = NULL;
static ID3D11DeviceContext1* g_DeviceContext1 = NULL; // 黒い帯の内側だけを空の色で消す ClearView に使う

// バックバッファはウィンドウと同じ大きさにし、ゲームの 1280×720 は比率を保って真ん中に描く（No.84）。
static UINT g_BufferWidth = SCREEN_WIDTH;
static UINT g_BufferHeight = SCREEN_HEIGHT;
static LETTERBOX g_Letterbox = { 0.0f, 0.0f, 1.0f };
static HWND g_PendingWindow = NULL; // 次のフレームで大きさを合わせるウィンドウ
static UINT g_PendingWidth = 0; // 次のフレームで合わせる大きさ（0 なら変更なし）
static UINT g_PendingHeight = 0;

static ID3D11RasterizerState* g_RasterizerState = NULL;
static ID3D11BlendState* g_BlendState = NULL;
static ID3D11DepthStencilState* g_DepthStencilStateDepthEnable = NULL;	// 3D用（深度テストあり）
static ID3D11DepthStencilState* g_DepthStencilStateDepthDisable = NULL;	// 2D用（深度テストなし）

// 垂直同期：モニターが画面を書き換えるタイミングに合わせて表示する
// true にすると画面のちらつき（上下で絵がずれる「ティアリング」）が出ない。
// FPS が 60 を大きく下回る PC で、さらにカクつくようなら false にして試す
static constexpr bool VSYNC = true;


// バックバッファのレンダーターゲットと深度バッファを作り、黒い帯の置き方を決め直す（大きさが 0 なら 1280×720）
static bool CreateTargets(HWND hWnd, UINT width, UINT height)
{
	if (width == 0 || height == 0) {
		width = SCREEN_WIDTH;
		height = SCREEN_HEIGHT;
	}
	ID3D11Texture2D* backBuffer = NULL;
	HRESULT hr = g_SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&backBuffer);
	if (SUCCEEDED(hr)) hr = g_Device->CreateRenderTargetView(backBuffer, NULL, &g_RenderTargetView);
	SAFE_RELEASE(backBuffer);
	if (FAILED(hr)) {
		MessageBox(hWnd, "レンダーターゲットビューの作成に失敗しました", "エラー", MB_OK);
		return false;
	}

	// デプスステンシル用テクスチャー作成（バックバッファと同じ大きさ）
	D3D11_TEXTURE2D_DESC td = {};
	td.Width = width;
	td.Height = height;
	td.MipLevels = 1;
	td.ArraySize = 1;
	td.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	td.SampleDesc.Count = 1;
	td.Usage = D3D11_USAGE_DEFAULT;
	td.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	hr = g_Device->CreateTexture2D(&td, NULL, &g_DepthStencilTexture);
	if (FAILED(hr)) {
		MessageBox(hWnd, "デプスステンシル用テクスチャーの作成に失敗しました", "エラー", MB_OK);
		return false;
	}

	// デプスステンシルターゲット作成
	D3D11_DEPTH_STENCIL_VIEW_DESC dsvd = {};
	dsvd.Format = td.Format;
	dsvd.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	hr = g_Device->CreateDepthStencilView(g_DepthStencilTexture, &dsvd, &g_DepthStencilView);
	if (FAILED(hr)) {
		MessageBox(hWnd, "デプスステンシルビューの作成に失敗しました", "エラー", MB_OK);
		return false;
	}

	g_BufferWidth = width;
	g_BufferHeight = height;
	g_Letterbox = LetterboxCalc((float)width, (float)height, (float)SCREEN_WIDTH, (float)SCREEN_HEIGHT);
	g_DeviceContext->OMSetRenderTargets(1, &g_RenderTargetView, g_DepthStencilView);
	return true;
}


bool DirectXInitialize(HWND hWnd)
{
	// デバイス、スワップチェイン、コンテキスト生成
	DXGI_SWAP_CHAIN_DESC sd = {};
	sd.BufferCount = 1;
	// バックバッファは今のクライアント領域と同じ大きさ（あとは WM_SIZE の DirectXResize で合わせる）。
	RECT client{};
	GetClientRect(hWnd, &client);
	const UINT clientWidth = client.right > client.left ? (UINT)(client.right - client.left) : SCREEN_WIDTH;
	const UINT clientHeight = client.bottom > client.top ? (UINT)(client.bottom - client.top) : SCREEN_HEIGHT;
	sd.BufferDesc.Width = clientWidth;
	sd.BufferDesc.Height = clientHeight;
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

	// Alt+Enter は main.cpp の枠なし全画面だけで扱い、DXGI の排他全画面を止める。
	IDXGIDevice* dxgiDevice = nullptr;
	IDXGIAdapter* adapter = nullptr;
	IDXGIFactory* factory = nullptr;
	hr = g_Device->QueryInterface(__uuidof(IDXGIDevice), (void**)&dxgiDevice);
	if (SUCCEEDED(hr)) hr = dxgiDevice->GetAdapter(&adapter);
	if (SUCCEEDED(hr)) hr = adapter->GetParent(__uuidof(IDXGIFactory), (void**)&factory);
	if (SUCCEEDED(hr)) hr = factory->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER);
	SAFE_RELEASE(factory);
	SAFE_RELEASE(adapter);
	SAFE_RELEASE(dxgiDevice);
	if (FAILED(hr)) {
		MessageBox(hWnd, "全画面の切り替えの設定に失敗しました", "エラー", MB_OK);
		return false;
	}

	g_DeviceContext->QueryInterface(__uuidof(ID3D11DeviceContext1), (void**)&g_DeviceContext1);
	if (!CreateTargets(hWnd, clientWidth, clientHeight)) return false;
	SetViewport(0.0f, 0.0f, (float)SCREEN_WIDTH, (float)SCREEN_HEIGHT);

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
	SAFE_RELEASE(g_DeviceContext1);
	SAFE_RELEASE(g_SwapChain);
	SAFE_RELEASE(g_DeviceContext);
	SAFE_RELEASE(g_Device);
}

void DirectXResize(HWND hWnd, UINT width, UINT height)
{
	// その場では作り直さず、次のフレームの描き始め（Clear）でまとめて行う。
	if (width == 0 || height == 0) return;
	g_PendingWindow = hWnd;
	g_PendingWidth = width;
	g_PendingHeight = height;
}

// 待っている大きさの変更があれば、バックバッファと深度バッファを作り直す（失敗したら次のフレームでもう一度）。
static void ApplyPendingResize(void)
{
	if (g_SwapChain == NULL || g_PendingWidth == 0 || g_PendingHeight == 0) return;
	if (g_PendingWidth == g_BufferWidth && g_PendingHeight == g_BufferHeight) {
		g_PendingWidth = g_PendingHeight = 0;
		return;
	}
	// バックバッファを持っているビューをすべて外して放し、GPU に残った命令を流してから作り直す。
	g_DeviceContext->OMSetRenderTargets(0, NULL, NULL);
	SAFE_RELEASE(g_DepthStencilView);
	SAFE_RELEASE(g_DepthStencilTexture);
	SAFE_RELEASE(g_RenderTargetView);
	g_DeviceContext->Flush();
	if (SUCCEEDED(g_SwapChain->ResizeBuffers(0, g_PendingWidth, g_PendingHeight, DXGI_FORMAT_UNKNOWN, 0))) {
		CreateTargets(g_PendingWindow, g_PendingWidth, g_PendingHeight);
		g_PendingWidth = g_PendingHeight = 0;
	}
	else {
		CreateTargets(g_PendingWindow, g_BufferWidth, g_BufferHeight);
	}
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


// 描く範囲（ビューポート）の設定
void SetViewport(float x, float y, float width, float height)
{
	D3D11_VIEWPORT vp;
	// ゲームの 1280×720 の座標を、黒い帯の内側のバックバッファの座標に直す。
	vp.Width = width * g_Letterbox.Scale;
	vp.Height = height * g_Letterbox.Scale;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = g_Letterbox.OffsetX + x * g_Letterbox.Scale;
	vp.TopLeftY = g_Letterbox.OffsetY + y * g_Letterbox.Scale;
	g_DeviceContext->RSSetViewports(1, &vp);
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
	ApplyPendingResize();
	// 全体を黒（帯）で消してから、ゲームの範囲だけを空の色で消す。
	const float black[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
	float clearColor[4] = { 0.4f, 0.6f, 0.9f, 1.0f };
	if (g_DeviceContext1 != NULL) {
		const D3D11_RECT game = { (LONG)g_Letterbox.OffsetX, (LONG)g_Letterbox.OffsetY,
			(LONG)(g_Letterbox.OffsetX + SCREEN_WIDTH * g_Letterbox.Scale + 0.5f), (LONG)(g_Letterbox.OffsetY + SCREEN_HEIGHT * g_Letterbox.Scale + 0.5f) };
		g_DeviceContext->ClearRenderTargetView(g_RenderTargetView, black);
		g_DeviceContext1->ClearView(g_RenderTargetView, clearColor, &game, 1);
	}
	else {
		g_DeviceContext->ClearRenderTargetView(g_RenderTargetView, clearColor);
	}
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
