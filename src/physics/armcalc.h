#pragma once

#include "physics/tentaclecalc.h"

/// @file armcalc.h
/// @brief 使える腕の巡回・直接選択（DirectX と Windows を使わない）

/// @brief 指定した向きへ次の使える腕を選ぶ（端は反対の端へ回る）
/// @param current 今の腕の番号（0〜count-1）
/// @param direction 次は +1、前は -1
/// @param available 腕ごとの使用可否（true：使える、false：使えない）の並び
/// @param count 腕の数
/// @return 次の使える番号。今の1つしか使えなければ current。不正な引数・使える腕なしは -1
int ArmCalcCycle(int current, int direction, const bool* available, int count);

/// @brief 指定した番号の腕が使えるか調べる
/// @param number 選びたい腕の番号
/// @param available 腕ごとの使用可否（true：使える、false：使えない）の並び
/// @param count 腕の数
/// @return 使えるなら number。使えない腕・範囲外・不正な引数は -1
int ArmCalcSelect(int number, const bool* available, int count);

/// @brief キャラの水平の正面を上下へ傾けた単位方向を求める
/// @param forward 正面の向き（Y は使わず XZ を正規化する。水平の長さがほぼ 0 なら +Z）
/// @param pitch 上下の角度（ラジアン。負なら下）
/// @return 傾けた長さ 1 の方向
VEC3 ArmCalcPitchDirection(const VEC3& forward, float pitch);

/// @brief 2つの方向を線形にまぜ、長さ 1 にする
/// @param a 始めの方向
/// @param b 終わりの方向
/// @param ratio まぜる割合（0〜1 に収める。0 は a、1 は b）
/// @return まぜた単位方向。長さがほぼ 0 なら b を正規化し、b もほぼ 0 なら +Z
VEC3 ArmCalcBlendDirection(const VEC3& a, const VEC3& b, float ratio);

/// @brief 腕の長さ方向を保ち、X・Z 軸をひねる（X' = X cosθ + Z sinθ、Z' = Z cosθ - X sinθ）
/// @param axes ひねる前の直交単位軸
/// @param roll ひねりの角度（ラジアン。0 なら元の軸をそのまま返す）
/// @return Y 軸を変えずにひねった軸
TENTACLE_PIECE_AXES ArmCalcRollAxes(const TENTACLE_PIECE_AXES& axes, float roll);
