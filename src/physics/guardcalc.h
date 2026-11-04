#pragma once

#include "physics/vec3.h"

/// @file guardcalc.h
/// @brief ガード（前だけ・全方向）で攻撃を防ぐかの判定（DirectX と Windows は使わない）

/// @brief ガードの種類
enum GUARD_KIND
{
	GUARD_NONE, ///< ガードしていない
	GUARD_FRONT, ///< 前だけ防ぐ（デカ盾）
	GUARD_ALL ///< 全方向を防ぐ（バリア・ワープのよけ）
};

/// @brief 1体のガードの状態
struct GUARD
{
	GUARD_KIND Kind = GUARD_NONE; ///< ガードの種類
	VEC3 Forward = { 0.0f, 0.0f, 1.0f }; ///< 前の水平の単位方向（前だけのときに使う）
	float HalfAngle = 0.0f; ///< 前だけのときの左右それぞれの角度（ラジアン）
};

/// @brief ガードが攻撃を防ぐか調べる
/// @param guard ガードの状態
/// @param center ガードしている体の中心
/// @param from 攻撃が来た位置
/// @return 防ぐなら true（前だけのときは、水平に見て from が前の範囲にあれば true。真上・真下は防ぐ）
bool GuardCalcBlocks(const GUARD& guard, const VEC3& center, const VEC3& from);

/// @brief 2つのガードのうち強い方を選ぶ（全方向 ＞ 前だけ ＞ なし）
GUARD GuardCalcStronger(const GUARD& a, const GUARD& b);
