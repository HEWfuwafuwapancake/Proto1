#pragma once

#include <math.h>

/// @file mathutil.h
/// @brief 数値の小さな計算（範囲に収める・補間する・左右の角度を回す）
/// @details DirectX を使わないので、単体テスト（tests/Proto1Tests）からそのまま使える

/// @brief 値を 0〜1 の範囲に収める
/// @param v 値
/// @return 0 より小さければ 0、1 より大きければ 1、それ以外は v のまま
inline float Saturate(float v)
{
	return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
}

/// @brief 0〜1 の値を、両端がなめらかになるように変える（スムーズステップ）
/// @details t * t * (3 - 2t)。0 と 1 の近くで変化がゆっくりになる
/// @param t 0〜1 の値（範囲の外は Saturate してから渡す）
/// @return 0〜1 の値（t = 0 で 0、t = 0.5 で 0.5、t = 1 で 1）
inline float SmoothStep01(float t)
{
	return t * t * (3.0f - 2.0f * t);
}

/// @brief 2つの値の間を補間する（線形補間）
/// @param a t = 0 のときの値
/// @param b t = 1 のときの値
/// @param t 補間の割合（0〜1。範囲の外も計算はする）
/// @return a + (b - a) * t
inline float Lerp(float a, float b, float t)
{
	return a + (b - a) * t;
}

/// @brief 左右の角度を -π〜π の範囲に収める
/// @param yaw 左右の角度（ラジアン）
/// @return 同じ向きを表す -π〜π の角度（ラジアン）
inline float WrapYaw(float yaw)
{
	return atan2f(sinf(yaw), cosf(yaw));
}

/// @brief 最短方向へ毎秒 speed ラジアンまで回す
/// @param current 今の角度（ラジアン）
/// @param target 目標の角度（ラジアン）
/// @param speed 1秒に回る角度の上限（ラジアン。0以下なら動かない）
/// @param dt 1フレームの秒数（0以下なら動かない）
/// @return 回したあとの角度（-π〜π。動かないときはcurrent）
inline float TurnYaw(float current, float target, float speed, float dt)
{
	if (dt <= 0.0f || speed <= 0.0f) return current;
	const float delta = WrapYaw(target - current);
	const float step = speed * dt;
	return WrapYaw(current + fmaxf(-step, fminf(step, delta)));
}
