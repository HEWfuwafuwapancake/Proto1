#pragma once

#include "physics/stagecalc.h"
#include "physics/vec3.h"

/// @file warpcalc.h
/// @brief ワープの行き先（相手の手前・浮島の外は縮める）の計算（DirectX と Windows は使わない）

/// @brief 相手の手前の行き先を求める（高さは start のまま）
/// @param start 今の足元
/// @param target 相手の足元
/// @param distance 相手の手前に残す水平の距離
/// @return 相手から start の方へ distance だけ戻った点。もう distance より近ければ start
VEC3 WarpCalcNearTarget(const VEC3& start, const VEC3& target, float distance);

/// @brief 行き先が浮島の外なら、線の上で浮島の上にいられるいちばん遠い所まで縮める
/// @details start が浮島の外なら縮めない（場外から戻るときに邪魔をしない）
/// @param start 今の足元
/// @param desired 行きたい点
/// @param stage 浮島
/// @return 行き先
VEC3 WarpCalcDestination(const VEC3& start, const VEC3& desired, const STAGE& stage);
