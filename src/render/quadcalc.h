#pragma once

#include "render/glyphcalc.h"

/// @file quadcalc.h
/// @brief 画面（2D）の四角を、傾けたり斜めにしたりした4つの角を求める計算（DirectX を使わない）
/// @details メニューの斜めのボタン・傾いた文字に使う。座標は画面の左上が原点のピクセル（下がプラス）

/// @brief 画面の四角形の4つの角（並びは 0：左上、1：右上、2：左下、3：右下）
struct QUAD2D
{
	float X[4]; ///< 4つの角の横の位置
	float Y[4]; ///< 4つの角の縦の位置
};

/// @brief 傾けない四角を、4つの角にする
/// @param rect 四角
/// @return 四角の4つの角
QUAD2D QuadCalcRect(const RECT2D& rect);

/// @brief 四角を、上の辺が右へずれた平行四辺形にする（斜めのボタン）
/// @details 真ん中の高さは動かさず、上の辺を lean の半分だけ右へ、下の辺を半分だけ左へずらす
/// @param rect 元の四角
/// @param lean 上の辺が下の辺より右へずれる量（ピクセル。負なら左へ）
/// @return 平行四辺形の4つの角
QUAD2D QuadCalcSkew(const RECT2D& rect, float lean);

/// @brief 4つの角を、ある点を中心に回す
/// @param quad 元の4つの角
/// @param centerX 回す中心の横の位置
/// @param centerY 回す中心の縦の位置
/// @param angle 回す角度（ラジアン。プラスで画面の時計回り）
/// @return 回したあとの4つの角
QUAD2D QuadCalcRotate(const QUAD2D& quad, float centerX, float centerY, float angle);
