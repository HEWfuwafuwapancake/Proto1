#pragma once

#include "core/dx11.h"
#include "render/glyphcalc.h"

/// @brief 起動時に文字列を白い文字のテクスチャにする（同じ文字列は同じ画像を使う）
/// @param text GDI に渡す文字列
/// @return 画像の番号。作れなければ -1
int Text2DCreate(const char* text);

/// @brief GDI の描画用オブジェクトとすべての文字画像・頂点バッファを解放する
void Text2DFinalize(void);

/// @brief 文字画像を指定の色で画面に貼る
/// @param id Text2DCreate で作った画像の番号
/// @param x 横の揃えの基準
/// @param y 上端
/// @param height 描く高さ
/// @param align 左・中央・右揃え
/// @param color 文字に掛ける色（RGBA）
/// @param fullHeight 横幅を求めるときの高さ（縦だけつぶすときに指定。省略時は height）
void Text2DDraw(int id, float x, float y, float height, GLYPH_ALIGN align, const XMFLOAT4& color, float fullHeight = 0.0f);
