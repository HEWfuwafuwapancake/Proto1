#pragma once

/// @file sespatialcalc.h
/// @brief 戦闘の効果音の大きさと左右を、聞き手（この PC で遊ぶ人）と音の出た場所から決める計算（Windows・DirectX 不使用）
/// @details 同じ PC で遊ぶ人が何人いても、音は1回だけ鳴らす。音の出た場所にいちばん近い人を聞き手にし、
///          その人からの距離で小さくし、その人のカメラの右向きで左右を決める。
///          当てた人・当てられた人がこの PC で遊ぶ人なら、第三者どうしの音より大きくする

#include "physics/vec3.h"

/// @brief 同時に扱う聞き手の最大数（同じ PC で遊ぶ人の最大数）
static constexpr int SE_LISTENER_MAX = 4;

/// @brief 聞き手（この PC で遊ぶ人）1人分
struct SE_LISTENER
{
	VEC3 Position = {}; ///< 聞く位置（その人の体の位置）
	VEC3 Right = { 1.0f, 0.0f, 0.0f }; ///< その人のカメラの右向き（上下の成分は使わない）
	int Slot = -1; ///< その人のスロット番号（0〜3）
};

/// @brief 1つの音の鳴らし方
struct SE_MIX
{
	float Volume = 1.0f; ///< 表の音量に掛ける倍率（距離と当事者かどうかで決まる）
	float Pan = 0.0f; ///< 左右（-1：左だけ、0：真ん中、+1：右だけ）
	int Listener = -1; ///< 基準にした聞き手の番号（聞き手がいないときは -1）
};

/// @brief 音の出た場所と、音に関わる人から鳴らし方を決める
/// @param listeners 聞き手の並び（スロットの小さい順。距離が同じなら前の人を選ぶ）
/// @param count 聞き手の数（0 なら位置を使わず、真ん中・倍率 1.0）
/// @param source 音の出た場所
/// @param slotA 音に関わる人のスロット（攻撃した人など。いなければ -1）
/// @param slotB 音に関わるもう1人のスロット（当てられた人など。いなければ -1）
/// @return 鳴らし方
SE_MIX SeSpatialCalcMix(const SE_LISTENER* listeners, int count, const VEC3& source, int slotA, int slotB);

/// @brief 左右の値から、左と右のスピーカーの大きさを求める（真ん中から端まで、聞こえる大きさが変わらない配り方）
/// @param pan 左右（-1〜+1。範囲の外は端にそろえる）
/// @param left 左の大きさを書く先（真ん中で約 0.707）
/// @param right 右の大きさを書く先（真ん中で約 0.707）
void SeSpatialCalcPanGains(float pan, float& left, float& right);
