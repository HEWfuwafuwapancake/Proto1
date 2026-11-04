#pragma once

#include "physics/tentaclecalc.h"

/// @file thundercalc.h
/// @brief サンダーの雷が連鎖する相手の選び方（DirectX と Windows は使わない）

/// @brief 最初の相手から、まだ当てていない近い相手へ順に連鎖させる
/// @details 直前に当てた相手の体の中心から、range 以内でいちばん近い Available な相手を選ぶ。同じ相手には2回いかない
/// @param targets 相手の並び
/// @param count 相手の数
/// @param first 最初に当たった相手の番号（範囲外なら 0 を返す）
/// @param range 連鎖が届く距離
/// @param maxCount 最初の相手を含めた最大の人数
/// @param order 当たる順の相手の番号を書く並び（maxCount 個以上）
/// @return 書いた人数
int ThunderCalcChain(const TENTACLE_TARGET* targets, int count, int first, float range, int maxCount, int* order);
