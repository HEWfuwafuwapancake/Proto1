#pragma once

#include "physics/stagecalc.h"

/// @file breachcalc.h
/// @brief 刀の長押しで一直線に飛ぶ先（着地点）の計算（DirectX と Windows は使わない）

/// @brief 見ている線の先で、飛んで着地できる足元の位置を探す
/// @details 線が足場の上の面の高さを通る点が足場の上なら、いちばん手前をそこにする（下から見上げた高台も選べる）。
///          どの面にも当たらなければ、線の先から手前へ戻りながら、真下に足場がある最初の点へ下ろす
/// @param stage ステージ
/// @param origin 線の始まり（頭）
/// @param direction 線の単位方向（カメラの向き）
/// @param range 線の長さ
/// @param landing 見つかったときに書き込む足元の位置
/// @return 着地できる場所があれば true
bool BreachCalcFindLanding(const STAGE& stage, const VEC3& origin, const VEC3& direction, float range, VEC3& landing);
