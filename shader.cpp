/*==============================================================================

   シェーダー [shader.cpp]
                                                         Author : Youhei Sato
                                                         Date   : 2025/05/15
--------------------------------------------------------------------------------
   授業のひな形（DX21）をもとに、3D用に World / View / Projection・
   マテリアル・ライトを渡せるように拡張したもの
==============================================================================*/
#include "shader.h"
#include "debug_ostream.h"
#include <fstream>
#include <vector>


static ID3D11VertexShader* g_pVertexShader = nullptr;
static ID3D11InputLayout* g_pInputLayout = nullptr;
static ID3D11PixelShader* g_pPixelShader = nullptr;
static ID3D11SamplerState* g_SamplerState = nullptr;

// 定数バッファ（番号は shaderCommon.hlsli の register(b0)〜(b4) と合わせる）
static ID3D11Buffer* g_pWorldBuffer = nullptr;		// b0
static ID3D11Buffer* g_pViewBuffer = nullptr;		// b1
static ID3D11Buffer* g_pProjectionBuffer = nullptr;	// b2
static ID3D11Buffer* g_pMaterialBuffer = nullptr;	// b3
static ID3D11Buffer* g_pLightBuffer = nullptr;		// b4

// 注意！初期化で外部から設定されるもの。Release不要。
static ID3D11Device* g_pDevice = nullptr;
static ID3D11DeviceContext* g_pContext = nullptr;


// 事前コンパイル済みシェーダー（.cso）をまるごと読み込む
static bool LoadShaderFile(const char* filename, std::vector<unsigned char>& binary)
{
	std::ifstream ifs(filename, std::ios::binary);
	if (!ifs) {
		return false;
	}

	ifs.seekg(0, std::ios::end); // ファイルポインタを末尾に移動
	std::streamsize filesize = ifs.tellg(); // ファイルポインタの位置を取得（つまりファイルサイズ）
	ifs.seekg(0, std::ios::beg); // ファイルポインタを先頭に戻す

	binary.resize((size_t)filesize);
	ifs.read((char*)binary.data(), filesize); // バイナリデータを読み込む
	return true;
}

// 定数バッファを作る
static ID3D11Buffer* CreateConstantBuffer(UINT size)
{
	D3D11_BUFFER_DESC buffer_desc{};
	buffer_desc.ByteWidth = size; // バッファのサイズ（16の倍数）
	buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER; // バインドフラグ

	ID3D11Buffer* buffer = nullptr;
	g_pDevice->CreateBuffer(&buffer_desc, nullptr, &buffer);
	return buffer;
}

// 行列を転置して定数バッファにセットする（HLSL側は列優先で受け取るため）
static void SetMatrixBuffer(ID3D11Buffer* buffer, const XMMATRIX& matrix)
{
	XMFLOAT4X4 transpose;
	XMStoreFloat4x4(&transpose, XMMatrixTranspose(matrix));
	g_pContext->UpdateSubresource(buffer, 0, nullptr, &transpose, 0, 0);
}


bool Shader_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	HRESULT hr; // 戻り値格納用

	// デバイスとデバイスコンテキストのチェック
	if (!pDevice || !pContext) {
		hal::dout << "Shader_Initialize() : 与えられたデバイスかコンテキストが不正です" << std::endl;
		return false;
	}

	// デバイスとデバイスコンテキストの保存
	g_pDevice = pDevice;
	g_pContext = pContext;


	// 事前コンパイル済み頂点シェーダーの読み込み
	std::vector<unsigned char> vsbinary;
	if (!LoadShaderFile("shaderVertex3D.cso", vsbinary)) {
		MessageBox(nullptr, "頂点シェーダーの読み込みに失敗しました\n\nshaderVertex3D.cso", "エラー", MB_OK);
		return false;
	}

	// 頂点シェーダーの作成
	hr = g_pDevice->CreateVertexShader(vsbinary.data(), vsbinary.size(), nullptr, &g_pVertexShader);
	if (FAILED(hr)) {
		hal::dout << "Shader_Initialize() : 頂点シェーダーの作成に失敗しました" << std::endl;
		return false;
	}


	// 頂点レイアウトの定義（VERTEX_3D の並びと合わせる）
	D3D11_INPUT_ELEMENT_DESC layout[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,		0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,		0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT,	0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,			0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	UINT num_elements = ARRAYSIZE(layout); // 配列の要素数を取得

	// 頂点レイアウトの作成
	hr = g_pDevice->CreateInputLayout(layout, num_elements, vsbinary.data(), vsbinary.size(), &g_pInputLayout);
	if (FAILED(hr)) {
		hal::dout << "Shader_Initialize() : 頂点レイアウトの作成に失敗しました" << std::endl;
		return false;
	}


	// 事前コンパイル済みピクセルシェーダーの読み込み
	std::vector<unsigned char> psbinary;
	if (!LoadShaderFile("shaderPixel3D.cso", psbinary)) {
		MessageBox(nullptr, "ピクセルシェーダーの読み込みに失敗しました\n\nshaderPixel3D.cso", "エラー", MB_OK);
		return false;
	}

	// ピクセルシェーダーの作成
	hr = g_pDevice->CreatePixelShader(psbinary.data(), psbinary.size(), nullptr, &g_pPixelShader);
	if (FAILED(hr)) {
		hal::dout << "Shader_Initialize() : ピクセルシェーダーの作成に失敗しました" << std::endl;
		return false;
	}


	// 定数バッファの作成
	g_pWorldBuffer = CreateConstantBuffer(sizeof(XMFLOAT4X4));
	g_pViewBuffer = CreateConstantBuffer(sizeof(XMFLOAT4X4));
	g_pProjectionBuffer = CreateConstantBuffer(sizeof(XMFLOAT4X4));
	g_pMaterialBuffer = CreateConstantBuffer(sizeof(MATERIAL));
	g_pLightBuffer = CreateConstantBuffer(sizeof(LIGHT));
	if (!g_pWorldBuffer || !g_pViewBuffer || !g_pProjectionBuffer || !g_pMaterialBuffer || !g_pLightBuffer) {
		hal::dout << "Shader_Initialize() : 定数バッファの作成に失敗しました" << std::endl;
		return false;
	}

	// 初期値（単位行列・白いマテリアル・ライトなし）
	Shader_SetWorldMatrix(XMMatrixIdentity());
	Shader_SetViewMatrix(XMMatrixIdentity());
	Shader_SetProjectionMatrix(XMMatrixIdentity());

	MATERIAL material{};
	material.Diffuse = { 1.0f, 1.0f, 1.0f, 1.0f };
	material.Ambient = { 1.0f, 1.0f, 1.0f, 1.0f };
	Shader_SetMaterial(material);

	LIGHT light{};
	Shader_SetLight(light);


	// サンプラーステート設定
	D3D11_SAMPLER_DESC samplerDesc = {};
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.MipLODBias = 0;
	samplerDesc.MaxAnisotropy = 16;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

	g_pDevice->CreateSamplerState(&samplerDesc, &g_SamplerState);

	g_pContext->PSSetSamplers(0, 1, &g_SamplerState);

	return true;
}

void Shader_Finalize()
{
	SAFE_RELEASE(g_SamplerState);
	SAFE_RELEASE(g_pLightBuffer);
	SAFE_RELEASE(g_pMaterialBuffer);
	SAFE_RELEASE(g_pProjectionBuffer);
	SAFE_RELEASE(g_pViewBuffer);
	SAFE_RELEASE(g_pWorldBuffer);
	SAFE_RELEASE(g_pPixelShader);
	SAFE_RELEASE(g_pInputLayout);
	SAFE_RELEASE(g_pVertexShader);
}

void Shader_SetWorldMatrix(const DirectX::XMMATRIX& matrix)
{
	SetMatrixBuffer(g_pWorldBuffer, matrix);
}

void Shader_SetViewMatrix(const DirectX::XMMATRIX& matrix)
{
	SetMatrixBuffer(g_pViewBuffer, matrix);
}

void Shader_SetProjectionMatrix(const DirectX::XMMATRIX& matrix)
{
	SetMatrixBuffer(g_pProjectionBuffer, matrix);
}

void Shader_Set2DMatrix()
{
	// 2Dはワールド・ビューを使わず、プロジェクションだけで画面のピクセル座標に合わせる
	Shader_SetWorldMatrix(XMMatrixIdentity());
	Shader_SetViewMatrix(XMMatrixIdentity());
	Shader_SetProjectionMatrix(XMMatrixOrthographicOffCenterLH(0.0f,
		SCREEN_WIDTH, SCREEN_HEIGHT, 0.0f, 0.0f, 1.0f));
}

void Shader_SetMaterial(const MATERIAL& material)
{
	g_pContext->UpdateSubresource(g_pMaterialBuffer, 0, nullptr, &material, 0, 0);
}

void Shader_SetLight(const LIGHT& light)
{
	g_pContext->UpdateSubresource(g_pLightBuffer, 0, nullptr, &light, 0, 0);
}

void Shader_Begin()
{
	// 頂点シェーダーとピクセルシェーダーを描画パイプラインに設定
	g_pContext->VSSetShader(g_pVertexShader, nullptr, 0);
	g_pContext->PSSetShader(g_pPixelShader, nullptr, 0);

	// 頂点レイアウトを描画パイプラインに設定
	g_pContext->IASetInputLayout(g_pInputLayout);

	// 定数バッファを描画パイプラインに設定（b0〜b4。頂点・ピクセルの両方から見えるようにする）
	ID3D11Buffer* buffers[] = { g_pWorldBuffer, g_pViewBuffer, g_pProjectionBuffer, g_pMaterialBuffer, g_pLightBuffer };
	g_pContext->VSSetConstantBuffers(0, ARRAYSIZE(buffers), buffers);
	g_pContext->PSSetConstantBuffers(0, ARRAYSIZE(buffers), buffers);
}
