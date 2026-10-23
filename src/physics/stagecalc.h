#pragma once

/// @file stagecalc.h
/// @brief ステージ（浮島）と場外の計算
/// @details DirectX と Windows を使わないので、単体テスト（tests/Proto1Tests）からそのまま使える。
///          場外の線は下だけ（横や上に飛ばされても、落ちて下の線を越えたらアウト）

/// @brief 浮島と場外の線（四角い平らな足場が1つ）
struct STAGE
{
	float CenterX;		///< 浮島の中心の X
	float CenterZ;		///< 浮島の中心の Z
	float HalfWidth;	///< 浮島の横幅（X）の半分
	float HalfDepth;	///< 浮島の奥行き（Z）の半分
	float TopY;			///< 浮島の上の面の高さ（ここに立つ）
	float Thickness;	///< 浮島の厚さ（見た目だけ。上の面から下へこの分だけある）
	float OutY;			///< 場外の下の線の高さ（これより下に落ちたら場外）
};

/// @brief その場所の真下（または真上）に浮島があるか調べる
/// @details 高さは見ない。あれば上の面（TopY）に着地できる。端ちょうどは「ある」
/// @param stage ステージ
/// @param x 調べる場所の X
/// @param z 調べる場所の Z
/// @return 浮島の範囲の中なら true
bool StageCalcIsOnPlatform(const STAGE& stage, float x, float z);

/// @brief 場外の下の線より下に落ちたか調べる
/// @param stage ステージ
/// @param y 調べる高さ
/// @return 下の線（OutY）より下なら true（ちょうどの高さはまだ場外でない）
bool StageCalcIsOut(const STAGE& stage, float y);
