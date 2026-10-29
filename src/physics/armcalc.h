#pragma once

#include "physics/tentaclecalc.h"

/// @file armcalc.h
/// @brief 使える腕の選択・共通の取付位置と構え（DirectX と Windows を使わない）

/// @brief 指定した番号の腕が使えるか調べる
/// @param number 選びたい腕の番号
/// @param available 腕ごとの使用可否（true：使える、false：使えない）の並び
/// @param count 腕の数
/// @return 使えるなら number。使えない腕・範囲外・不正な引数は -1
int ArmCalcSelect(int number, const bool* available, int count);

/// @brief キャラの足元と水平の向きから、全種類に共通の肩の位置を求める
/// @param position キャラの足元のワールド座標
/// @param yaw キャラの水平の向き（ラジアン。0 は +Z）
/// @param hand 左 0・右 1
/// @param sideOffset キャラの中心から片側の肩までの横の距離
/// @param height 足元から肩までの高さ
/// @param forwardOffset 正面へのずれ（負なら後ろ）
/// @return 肩のワールド座標
VEC3 ArmCalcShoulder(const VEC3& position, float yaw, int hand, float sideOffset, float height, float forwardOffset);

/// @brief キャラの正面から左右へ開き、上下に傾けた待機の単位方向を求める
/// @param yaw キャラの水平の向き（ラジアン。0 は +Z）
/// @param hand 左 0・右 1
/// @param pitch 上下の角度（ラジアン。負なら下）
/// @param spread 片側の開き角（ラジアン。正なら左右それぞれ外側）
/// @return 左右に開いた長さ 1 の方向
VEC3 ArmCalcRestDirection(float yaw, int hand, float pitch, float spread);

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
