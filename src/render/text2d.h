#pragma once

#include "core/dx11.h"
#include "render/glyphcalc.h"
#include "render/quadcalc.h"

/// @brief 文字画像に使う書体
enum TEXT2D_FONT
{
	TEXT2D_FONT_BODY, ///< ふつうの文字（メイリオ）
	TEXT2D_FONT_BOLD, ///< 太い文字（メイリオの太字。メニューの小さな見出し）
	TEXT2D_FONT_POP, ///< ポップな見出し（assets/fonts のフォント → HG創英角ポップ体 → メイリオの太字の順に、あるものを使う）
	TEXT2D_FONT_COUNT, ///< 書体の数
};

/// @brief 起動時に文字列を白い文字のテクスチャにする（同じ文字列・書体は同じ画像を使う）
/// @param text GDI に渡す文字列
/// @param font 書体（省略時はふつうの文字）
/// @return 画像の番号。作れなければ -1
int Text2DCreate(const char* text, TEXT2D_FONT font = TEXT2D_FONT_BODY);

/// @brief 文字画像の横幅÷高さを取得する（傾けて描くときの幅を決めるのに使う）
/// @param id Text2DCreate で作った画像の番号
/// @return 横幅÷高さ。画像がなければ 0
float Text2DGetAspect(int id);

/// @brief 文字画像を、4つの角を指定して貼る（傾けた文字・斜めの文字）
/// @param id Text2DCreate で作った画像の番号
/// @param quad 画像の左上・右上・左下・右下を置く場所
/// @param color 文字に掛ける色（RGBA）
void Text2DDrawQuad(int id, const QUAD2D& quad, const XMFLOAT4& color);

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
