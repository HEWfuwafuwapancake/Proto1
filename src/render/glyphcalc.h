#pragma once

/// @file glyphcalc.h
/// @brief 数字（7 セグ）と英字（5×7 ドット）を「点ける四角の一覧」に変える計算
/// @details DirectX と Windows を使わないので、単体テスト（tests/Proto1Tests）からそのまま使える。
///          1桁は「幅 W・高さ 2W・棒の太さ 0.2W・桁の間 0.3W」、文字は 5×7 ドットで文字の間は1ドット（比は glyphcalc.cpp の「調整用の値」）。
///          描くのは render/number2d.h

/// @brief 画面の四角（左上が原点、ピクセル）
struct RECT2D
{
	float X;		///< 左端
	float Y;		///< 上端
	float Width;	///< 幅
	float Height;	///< 高さ
};

/// @brief 横の位置の合わせ方（x を左端・真ん中・右端のどれにするか）
enum GLYPH_ALIGN
{
	GLYPH_ALIGN_LEFT,	///< x が左端
	GLYPH_ALIGN_CENTER,	///< x が真ん中
	GLYPH_ALIGN_RIGHT,	///< x が右端
};

/// @brief 数を桁に分ける（上の桁から順に入れる）
/// @param value 数（負なら 0 として扱う）
/// @param minDigits 少なくともこの桁数にする（足りない上の桁は 0）
/// @param[out] digits 桁を入れる配列
/// @param maxDigits digits に入れられる桁数（入りきらなければ、全部 9 にする）
/// @return 桁数
int GlyphCalcSplitDigits(int value, int minDigits, int digits[], int maxDigits);

/// @brief 数字に点ける 7 セグの棒を取得する
/// @param digit 数字（0〜9）
/// @return 点ける棒のビット（bit0=a 上, bit1=b 右上, bit2=c 右下, bit3=d 下, bit4=e 左下, bit5=f 左上, bit6=g 真ん中）。範囲外は 0
unsigned char GlyphCalcSegments(int digit);

/// @brief 7 セグの棒1本の四角を取得する
/// @param segment 棒の番号（0=a 〜 6=g）
/// @param x 桁の左端
/// @param y 桁の上端
/// @param digitWidth 桁の幅 W（高さは 2W）
/// @return 棒の四角（範囲外の番号は大きさ 0）
RECT2D GlyphCalcSegmentRect(int segment, float x, float y, float digitWidth);

/// @brief 数字全体の幅を取得する
/// @param value 数
/// @param minDigits 少なくともこの桁数にする
/// @param digitWidth 桁の幅 W
/// @return 幅（ピクセル）
float GlyphCalcNumberWidth(int value, int minDigits, float digitWidth);

/// @brief 数字全体（7 セグ）の四角の一覧を作る
/// @param value 数（負は 0。10 桁を超えるときは 9 を並べる）
/// @param minDigits 少なくともこの桁数にする
/// @param x align で決まる横の位置
/// @param y 上端
/// @param digitWidth 桁の幅 W（高さは 2W）
/// @param align x を左端・真ん中・右端のどれにするか
/// @param[out] out 四角を入れる配列
/// @param maxRects out に入れられる数（これを超えて書かない）
/// @return 書いた四角の数
int GlyphCalcNumberRects(int value, int minDigits, float x, float y, float digitWidth,
	GLYPH_ALIGN align, RECT2D out[], int maxRects);

/// @brief 5×7 ドットの文字の1行を取得する
/// @param c 文字（A C D E I J K L M N O P R S T U V W X Y 1 % / と空白。それ以外は空白）
/// @param row 行（0 が上、6 が下。範囲外は 0）
/// @return その行のドット（下位5ビット。bit4 が左端）
unsigned char GlyphCalcLetterRow(char c, int row);

/// @brief 文字列全体の幅を取得する
/// @param text 文字列
/// @param dotSize ドット1つの大きさ
/// @return 幅（ピクセル。空の文字列なら 0）
float GlyphCalcTextWidth(const char* text, float dotSize);

/// @brief 文字列（5×7 ドット）の四角の一覧を作る（ドット1つが四角1つ）
/// @param text 文字列
/// @param x align で決まる横の位置
/// @param y 上端（高さは 7 ドット）
/// @param dotSize ドット1つの大きさ
/// @param align x を左端・真ん中・右端のどれにするか
/// @param[out] out 四角を入れる配列
/// @param maxRects out に入れられる数（これを超えて書かない）
/// @return 書いた四角の数
int GlyphCalcTextRects(const char* text, float x, float y, float dotSize,
	GLYPH_ALIGN align, RECT2D out[], int maxRects);

/// @brief 画面に出すダメージ%を取得する
/// @param percent ダメージ%
/// @return 四捨五入した整数（0〜999）
int GlyphCalcDisplayPercent(float percent);
