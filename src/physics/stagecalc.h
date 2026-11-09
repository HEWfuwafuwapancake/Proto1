#pragma once

#include "physics/vec3.h"

/// @file stagecalc.h
/// @brief ステージ（浮島と高台）と場外の計算
/// @details DirectX と Windows を使わないので、単体テスト（tests/Proto1Tests）からそのまま使える。
///          場外の線は下だけ（横や上に飛ばされても、落ちて下の線を越えたらアウト）

/// @brief ステージに置ける高台の最大数
static constexpr int STAGE_PLATFORM_MAX = 8;

/// @brief 下から通り抜けられる四角い高台
struct STAGE_PLATFORM
{
	float CenterX;		///< 高台の中心の X
	float CenterZ;		///< 高台の中心の Z
	float HalfWidth;	///< 高台の横幅（X）の半分
	float HalfDepth;	///< 高台の奥行き（Z）の半分
	float TopY;			///< 高台の上の面の高さ（ここに立つ）
	float Thickness;	///< 高台の描画上の厚さ
};

/// @brief 浮島、高台、場外の線
struct STAGE
{
	float CenterX;		///< 浮島の中心の X
	float CenterZ;		///< 浮島の中心の Z
	float HalfWidth;	///< 浮島の横幅（X）の半分
	float HalfDepth;	///< 浮島の奥行き（Z）の半分
	float TopY;			///< 浮島の上の面の高さ（ここに立つ）
	float Thickness;	///< 浮島の厚さ（見た目だけ。上の面から下へこの分だけある）
	float OutY;			///< 場外の下の線の高さ（これより下に落ちたら場外）
	STAGE_PLATFORM Platforms[STAGE_PLATFORM_MAX]{};	///< 高台の並び
	int PlatformCount = 0;	///< 高台の個数
};

/// @brief その場所の真下（または真上）に浮島があるか調べる
/// @details 高さは見ない。あれば上の面（TopY）に着地できる。端ちょうどは「ある」
/// @param stage ステージ
/// @param x 調べる場所の X
/// @param z 調べる場所の Z
/// @return 浮島の範囲の中なら true
bool StageCalcIsOnPlatform(const STAGE& stage, float x, float z);

/// @brief 指定の高さに浮島または高台の上面があるか調べる
/// @param stage ステージ
/// @param x 足元の X
/// @param y 足元の高さ
/// @param z 足元の Z
/// @return 足元に上面があれば true
bool StageCalcHasSupport(const STAGE& stage, float x, float y, float z);

/// @brief 上から通過した浮島と高台の上面のうち、最も高い上面を探す
/// @param stage ステージ
/// @param x 移動後の X
/// @param z 移動後の Z
/// @param previousY 移動前の高さ
/// @param y 移動後の高さ
/// @param landingY 上面が見つかったときに書き込む高さ
/// @return 上面が見つかったら true（同じ高さでの接触を含む）
bool StageCalcFindLanding(const STAGE& stage, float x, float z, float previousY, float y, float& landingY);

/// @brief 触手がつかむ足場のふち
struct STAGE_GRIP
{
	VEC3 Point;				///< ふちの点（上の面の高さ）
	STAGE_PLATFORM Surface;	///< そのふちを持つ足場の形（浮島なら浮島の形）
};

/// @brief 浮島の形を高台と同じ形にして取得する
/// @param stage ステージ
/// @return 浮島の中心・半幅・半奥行き・上の面の高さ・厚さ
STAGE_PLATFORM StageCalcIslandSurface(const STAGE& stage);

/// @brief 正面の水平の扇の中で、いちばん近い足場のふちを探す
/// @details 立っている高台は選ばない。浮島は、浮島の上に立っていないとき（場外・浮島より下）だけ選ぶ
/// @param stage ステージ
/// @param origin 距離と向きを測る点（肩）
/// @param feet 足元の位置（立っている足場を除くのに使う）
/// @param forward 扇の中心の向き（上下は使わない）
/// @param maxDistance 上下も含めて、これより遠いふちは選ばない
/// @param halfAngle 扇の左右それぞれの開き（ラジアン）
/// @param grip 見つかったときに書き込むふち
/// @return 見つかったら true
bool StageCalcFindGrip(const STAGE& stage, const VEC3& origin, const VEC3& feet, const VEC3& forward,
	float maxDistance, float halfAngle, STAGE_GRIP& grip);

/// @brief 場外の下の線より下に落ちたか調べる
/// @param stage ステージ
/// @param y 調べる高さ
/// @return 下の線（OutY）より下なら true（ちょうどの高さはまだ場外でない）
bool StageCalcIsOut(const STAGE& stage, float y);
