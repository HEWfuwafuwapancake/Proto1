#pragma once

#include "physics/stagecalc.h"

/// @file breachcalc.h
/// @brief 刀の長押しで一直線に飛ぶ先（着地点）の計算（DirectX と Windows は使わない）

/// @brief 見ている線の先で、飛んで着地できる足元の位置を探す
/// @details 浮島・高台を厚さのある箱として、線がいちばん手前で当たる面を調べる。
///          - 上の面に当たれば、そこに着地する。
///          - 天井（下の面）・壁（横の面）に当たれば通り抜けず、当たった所の手前から戻りながら、真下に見える足場がある最初の点へ下ろす。
///          - 何にも当たらず上を見ていれば、足元より高い足場のふちのうち、見ている向きにあって見えている、いちばん近い所の少し内側へ。
///          - 何にも当たらず水平・下を見ていれば、線の先から手前へ戻りながら、真下に見える足場がある最初の点へ下ろす。
///          見つからなければ、残りの探し方（崖のふち・線の上を戻る）も試す
/// @param stage ステージ
/// @param origin 線の始まり（頭）
/// @param direction 線の単位方向（カメラの向き）
/// @param range 線の長さ（崖のふちもこの距離まで）
/// @param feetY 今の足元の高さ（崖のふちは、これより高い足場だけを選ぶ）
/// @param inset 崖のふちから内側へ入れる距離（足場がこれより小さければ中心）
/// @param landing 見つかったときに書き込む足元の位置
/// @return 着地できる場所があれば true
bool BreachCalcFindLanding(const STAGE& stage, const VEC3& origin, const VEC3& direction, float range,
	float feetY, float inset, VEC3& landing);
