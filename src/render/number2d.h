#pragma once

#include "core/dx11.h"
#include "render/glyphcalc.h"

/// @file number2d.h
/// @brief 画面（2D）に四角・数字（7 セグ）・英字（5×7 ドット）を描く部品（フォントは読み込まない）
/// @details どの関数も、四角の一覧を頂点バッファに書いて1回で描く（1回に描ける四角は NUMBER2D_MAX_RECTS 個まで。超えた分は描かない）。
///          深度テストを切り、ライトを消してから呼ぶ。座標は画面の左上が原点のピクセル。四角の計算は render/glyphcalc.h

/// @brief 1回に描ける四角の数
static constexpr int NUMBER2D_MAX_RECTS = 512;

/// @brief 四角を描くための頂点バッファを作る
void Number2DInitialize(void);

/// @brief 頂点バッファを解放する
void Number2DFinalize(void);

/// @brief 四角の一覧を同じ色で描く
/// @param rects 四角の一覧
/// @param count 四角の数（NUMBER2D_MAX_RECTS を超えた分は描かない）
/// @param color 色（RGBA）
void Number2DDrawRects(const RECT2D rects[], int count, const XMFLOAT4& color);

/// @brief 数字を 7 セグで描く
/// @param value 数（負は 0）
/// @param minDigits 少なくともこの桁数にする（足りない上の桁は 0）
/// @param x align で決まる横の位置
/// @param y 上端
/// @param digitWidth 桁の幅（高さはこの2倍）
/// @param align x を左端・真ん中・右端のどれにするか
/// @param color 色（RGBA）
void Number2DDrawNumber(int value, int minDigits, float x, float y, float digitWidth, GLYPH_ALIGN align, const XMFLOAT4& color);

/// @brief 英字を 5×7 ドットで描く（使える文字は GlyphCalcLetterRow）
/// @param text 文字列
/// @param x align で決まる横の位置
/// @param y 上端（高さは dotSize の 7 倍）
/// @param dotSize ドット1つの大きさ
/// @param align x を左端・真ん中・右端のどれにするか
/// @param color 色（RGBA）
void Number2DDrawText(const char* text, float x, float y, float dotSize, GLYPH_ALIGN align, const XMFLOAT4& color);

/// @brief 四角を1つ描く（時間切れのおおいなど）
/// @param rect 四角
/// @param color 色（RGBA。半透明にできる）
void Number2DDrawBox(const RECT2D& rect, const XMFLOAT4& color);
