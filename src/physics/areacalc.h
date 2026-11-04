#pragma once

#include "physics/collision.h"
#include "physics/tentaclecalc.h"

/// @file areacalc.h
/// @brief 腕の攻撃の範囲（扇・線）と、吹っ飛ばす向きの共通の計算（DirectX と Windows は使わない）

/// @brief 水平の扇（上下に幅がある）に体が入っているか調べる
/// @details 体の線分の中で origin にいちばん近い点を使い、体の半径の分だけ広げて比べる
/// @param origin 扇の中心（肩など）
/// @param forward 扇の中心の向き（上下は使わない）
/// @param minRadius これより近い所は入らない（0 なら中心から）
/// @param radius 扇の半径
/// @param halfAngle 左右それぞれの開き（ラジアン）
/// @param halfHeight origin から上下それぞれの幅
/// @param body 体のカプセル
/// @return 入っていれば true
bool AreaCalcInSector(const VEC3& origin, const VEC3& forward, float minRadius, float radius,
	float halfAngle, float halfHeight, const CAPSULE& body);

/// @brief 向きを水平にして上を足し、長さ 1 にする（吹っ飛ばす向き）
/// @param direction 元の向き（上下は使わない）
/// @param up 水平の単位方向に足す上向きの量
/// @return 長さ 1 の向き（水平がほぼ 0 なら真上）
VEC3 AreaCalcLaunchDirection(const VEC3& direction, float up);

/// @brief 中心から体の中心への水平の向きに上を足し、長さ 1 にする（爆発・まわりへの押し出し）
/// @param center 中心（爆心など）
/// @param body 体のカプセル
/// @param up 水平の単位方向に足す上向きの量
/// @param fallback 体が中心の真上・真下にあるときの水平の向き
/// @return 長さ 1 の向き
VEC3 AreaCalcRadialDirection(const VEC3& center, const CAPSULE& body, float up, const VEC3& fallback);

/// @brief 向きを Y 軸のまわりに回す（正の角度は yaw が増える向き。+Z から +X へ）
/// @param direction 元の向き
/// @param angle 回す角度（ラジアン）
/// @return 回した向き（Y はそのまま）
VEC3 AreaCalcRotateYaw(const VEC3& direction, float angle);

/// @brief 太さのある線に最初に当たる相手を探す
/// @param origin 線の始まり
/// @param direction 線の単位方向
/// @param length 線の長さ
/// @param radius 線の太さ（半径）
/// @param targets 相手の並び（Available が false の相手は見ない）
/// @param count 相手の数
/// @param distance 当たった相手の、始まりから「線の上で体の軸にいちばん近い点」までの距離（当たらなければ書き換えない。nullptr なら書かない）
/// @return 当たった相手の番号。いなければ -1
int AreaCalcFirstOnRay(const VEC3& origin, const VEC3& direction, float length, float radius,
	const TENTACLE_TARGET* targets, int count, float* distance);

/// @brief 水平の扇に等しい間隔で並べた線のうち、何本が体に当たるか数える（ショットガン）
/// @param origin 線の始まり
/// @param direction 扇の中心の単位方向
/// @param length 線の長さ
/// @param radius 線の太さ（半径）
/// @param rayCount 線の本数（1 なら中心だけ）
/// @param spreadAngle 扇の左右それぞれの開き（ラジアン。端の線がこの角度）
/// @param body 体のカプセル
/// @return 当たった線の本数
int AreaCalcCountRays(const VEC3& origin, const VEC3& direction, float length, float radius,
	int rayCount, float spreadAngle, const CAPSULE& body);
