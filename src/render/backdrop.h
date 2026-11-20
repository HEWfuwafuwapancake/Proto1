#pragma once

#include "core/dx11.h"

/// @file backdrop.h
/// @brief メニューの背景に使う「ぼかした3Dの景色」を作って貼る部品
/// @details 小さな画面（BACKDROP_WIDTH×BACKDROP_HEIGHT）に3Dを描き、半分ずつ縮めて ぼかしてから、画面いっぱいに引き伸ばして貼る。
///          使い方：BackdropBegin → 3D を描く → BackdropEnd → SetViewport で画面全体へ戻す → BackdropDraw

/// @brief ぼかす前に3Dを描く画面の幅（ピクセル）
static constexpr UINT BACKDROP_WIDTH = 640;
/// @brief ぼかす前に3Dを描く画面の高さ（ピクセル）
static constexpr UINT BACKDROP_HEIGHT = 360;

/// @brief 描き込む画面・縮める画面・頂点バッファを作る（作り済みなら何もしない）
/// @return 作れたら true
bool BackdropInitialize(void);

/// @brief 作った画面と頂点バッファを解放する
void BackdropFinalize(void);

/// @brief ここから描く3Dを、背景用の小さな画面に描くようにする（空の色で消し、深度テストを有効にする）
/// @return 描き込めるなら true（false のときは BackdropEnd・BackdropDraw を呼ばない）
bool BackdropBegin(void);

/// @brief 描いた3Dを縮めてぼかし、描き込む先をゲームの画面へ戻す（ビューポートは呼んだ側で戻す）
void BackdropEnd(void);

/// @brief ぼかした景色を画面いっぱいに貼る（深度テストを切り、ライトを消してから呼ぶ）
/// @param tint 景色に掛ける色（RGBA。暗くしたり色を寄せたりする）
void BackdropDraw(const XMFLOAT4& tint);
