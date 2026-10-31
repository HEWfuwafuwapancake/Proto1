#pragma once // インクルードガード

#define NOMINMAX
#include <Windows.h>
#include <d3d11.h>
#include <DirectXMath.h>
using namespace DirectX;

/// @file dx11.h
/// @brief DirectX 11 の初期化と、画面のクリア・表示（デバイス、スワップチェイン、深度バッファ）

/// @brief COM オブジェクトを Release して NULL にする（NULL のときは何もしない）
#define SAFE_RELEASE(o)      do { if (o) { (o)->Release(); (o) = NULL; } } while (0)

/// @brief DirectX を初期化する（デバイス、スワップチェイン、深度バッファ、各ステート）
/// @param hWnd 描画するウィンドウ
/// @return 成功したら true（失敗したときはメッセージボックスを出す）
bool DirectXInitialize(HWND hWnd);

/// @brief DirectX のオブジェクトをすべて解放する
void DirectXFinalize(void);

/// @brief デバイスを取得する（バッファやテクスチャを作るときに使う）
/// @return デバイス（Release しないこと）
ID3D11Device* DirectXGetDevice(void);

/// @brief デバイスコンテキストを取得する（描画の命令を出すときに使う）
/// @return デバイスコンテキスト（Release しないこと）
ID3D11DeviceContext* DirectXGetDeviceContext(void);

/// @brief 画面（バックバッファ）と深度バッファを描画先に設定する
void SetRenderTarget(void);

/// @brief 深度テストを切り替える
/// @param enable 3D描画は true、2D（UI）描画は false
void SetDepthEnable(bool enable);

/// @brief 描く範囲（ビューポート）を設定する（ピクセル。左上が原点）
/// @param x 左端
/// @param y 上端
/// @param width 幅
/// @param height 高さ
void SetViewport(float x, float y, float width, float height);

/// @brief 画面（空の色）と深度バッファを消す（毎フレーム、描く前に呼ぶ）
void Clear(void);

/// @brief 描いた画面を表示する（毎フレーム、描き終わったら呼ぶ）
/// @details dx11.cpp の VSYNC が true なら、モニターの書き換え（垂直同期）を待ってから表示する
void Present(void);

#define SCREEN_WIDTH	(1280)		///< ウインドウの幅
#define SCREEN_HEIGHT	(720)		///< ウインドウの高さ

/// @brief 頂点の形（シェーダーの VS_IN と並びを合わせる）
struct VERTEX_3D
{
	XMFLOAT3 Position;	///< 頂点座標
	XMFLOAT3 Normal;	///< 法線ベクトル（ライトの計算に使う）
	XMFLOAT4 Diffuse;	///< 頂点カラー
	XMFLOAT2 TexCoord;	///< UV座標
};
