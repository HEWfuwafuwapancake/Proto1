/*==============================================================================

   シェーダー [shader.h]
                                                         Author : Youhei Sato
                                                         Date   : 2025/05/15
--------------------------------------------------------------------------------
   授業のひな形（DX21）をもとに、3D用に World / View / Projection・
   マテリアル・ライトを渡せるように拡張したもの
==============================================================================*/
#pragma once

#include "core/dx11.h"

/// @file shader.h
/// @brief シェーダーの読み込みと、行列・マテリアル・ライトをシェーダーに渡す関数

/// @brief マテリアル（物体の色）
/// @details 定数バッファは16バイト単位なので Dummy で詰めている。shaderCommon.hlsli と並びを合わせること
struct MATERIAL
{
	XMFLOAT4 Diffuse;		///< 拡散色（ライトが当たっている部分の色）
	XMFLOAT4 Ambient;		///< 環境色（影の部分の色）
	BOOL     TextureEnable;	///< テクスチャを使うか
	float    Dummy[3];		///< 使わない（定数バッファを16バイト単位にそろえるための詰め物）
};

/// @brief 平行光源（太陽のような、一方向から当たる光）
struct LIGHT
{
	BOOL     Enable;		///< ライトの計算をするか（2D描画のときは FALSE）
	float    Dummy[3];		///< 使わない（Direction を16バイトの区切りから始めるための詰め物）
	XMFLOAT4 Direction;		///< 光の進む向き
	XMFLOAT4 Diffuse;		///< 光の色
	XMFLOAT4 Ambient;		///< 環境光の色（光が当たらない面の明るさ）
};

/// @brief 霧（マップの景観を、カメラから遠いほど霧の色に寄せる）
/// @details カメラの位置は Shader_SetViewMatrix で設定したビュー行列から求める
struct FOG
{
	XMFLOAT4 Color;			///< rgb：霧の色、w：いちばん遠くで霧の色に寄せる割合（0 なら霧なし）
	float    Distance;		///< 霧がいちばん濃くなる、カメラからの距離
	float    Gamma;			///< 色を計算するときのガンマ（1：頂点の色のまま、0.55 など：その値で明るく持ち上げて焼き込んだ景観）
	float    GlowStrength;	///< 原点のまわりに足す赤い光の強さ（0 なら足さない）
	float    GlowRadius;	///< 赤い光が弱まる距離（0 より大きくする）
};

/// @brief スキニングの頂点レイアウト
struct SKINNED_VERTEX
{
	XMFLOAT3 Position; ///< メッシュ空間の座標
	XMFLOAT3 Normal; ///< メッシュ空間の法線
	UINT Bones[4]; ///< 頂点に効く骨の番号
	float Weights[4]; ///< 各骨の重み（合計1）
};

/// @brief 1メッシュの定数バッファに収める骨の最大数
constexpr UINT SHADER_MAX_BONES = 64;

/// @brief b5 に送る骨のパレット
struct SKINNED_BONES
{
	XMFLOAT4X4 Matrices[SHADER_MAX_BONES]; ///< HLSL の列優先の並びにした行列
};

/// @brief シェーダーと定数バッファを作る
/// @param pDevice デバイス
/// @param pContext デバイスコンテキスト
/// @return 成功したら true（.cso が見つからないときはメッセージボックスを出して false）
bool Shader_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);

/// @brief シェーダーと定数バッファを解放する
void Shader_Finalize();

/// @brief ワールド行列を設定する（物の位置・回転・大きさ）
/// @param matrix ワールド行列
void Shader_SetWorldMatrix(const DirectX::XMMATRIX& matrix);

/// @brief ビュー行列を設定する（カメラの位置と向き）
/// @param matrix ビュー行列
void Shader_SetViewMatrix(const DirectX::XMMATRIX& matrix);

/// @brief プロジェクション行列を設定する（遠近の付け方）
/// @param matrix プロジェクション行列
void Shader_SetProjectionMatrix(const DirectX::XMMATRIX& matrix);

/// @brief 2D描画用（左上が原点・ピクセル単位）の行列をまとめて設定する
void Shader_Set2DMatrix();

/// @brief 2D描画で、今のビューポート全体に当てはめる座標の幅と高さを設定する（はじめは画面の大きさ）
/// @param width 右端の x 座標
/// @param height 下端の y 座標
void Shader_Set2DSize(float width, float height);

/// @brief マテリアル（物体の色）を設定する
/// @param material マテリアル
void Shader_SetMaterial(const MATERIAL& material);

/// @brief ライトを設定する
/// @param light 平行光源（2D描画のときは Enable を FALSE にする）
void Shader_SetLight(const LIGHT& light);

/// @brief 最後に設定したライトの向き・色のまま、ライトの計算をするかだけを切り替える
/// @details 頂点の色にライトを焼き込んだモデル（マップの景観）を描くときに、いったん切って、描いたら戻す
/// @param enable ライトの計算を設定する（true：する、false：しない（頂点の色 × マテリアルの色をそのまま出す））
void Shader_SetLightEnable(bool enable);

/// @brief 霧を設定する（マップの景観を描く前に設定し、描いたら Color.w が 0 の霧で切る）
/// @param fog 霧（Color.w が 0 なら霧を掛けない）
void Shader_SetFog(const FOG& fog);

/// @brief ふつうの3D／2D用のシェーダーに切り替える
void Shader_Begin();

/// @brief スキニング用の頂点シェーダーとレイアウトに切り替える
void Shader_BeginSkinned();

/// @brief 今描くメッシュの骨のパレットを設定する
void Shader_SetSkinnedBones(const SKINNED_BONES& bones);
