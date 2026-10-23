#pragma once

#include "physics/stagecalc.h"

/// @file cameracalc.h
/// @brief TPS カメラの計算（向きと距離から位置を出す・浮島にめり込まないようにする・なめらかに追う）
/// @details DirectX と Windows を使わないので、単体テスト（tests/Proto1Tests）からそのまま使える

/// @brief 3D の点（カメラの計算用）
struct CAMERA_VEC3
{
	float X;	///< X
	float Y;	///< Y（上がプラス）
	float Z;	///< Z
};

/// @brief 見ている点のまわりを回るカメラの位置を出す
/// @details 見ている向きは (cos(pitch)·sin(yaw), sin(pitch), cos(pitch)·cos(yaw))。カメラはその反対へ distance 下がったところ
/// @param target 見ている点
/// @param yaw 左右の向き（ラジアン。0 で +Z 方向を見る、プラスで右回り）
/// @param pitch 上下の向き（ラジアン。プラスで上を見る）
/// @param distance 見ている点からカメラまでの距離
/// @return カメラの位置
CAMERA_VEC3 CameraCalcOrbitPosition(const CAMERA_VEC3& target, float yaw, float pitch, float distance);

/// @brief 見ている点からカメラへの線が浮島にぶつかるなら、ぶつかる手前までカメラを近づける
/// @details 浮島の箱を margin だけ大きくして調べる（カメラが面にぴったり付いて、近くのものが切れて見えないように）。
///          見ている点が浮島の箱の中にあるときは、近づけようがないので camera をそのまま返す
/// @param target 見ている点
/// @param camera カメラを置きたい位置
/// @param stage 浮島
/// @param margin 浮島の箱を大きくする量（0 以上）
/// @return めり込まない位置（ぶつからなければ camera のまま）
CAMERA_VEC3 CameraCalcClipToStage(const CAMERA_VEC3& target, const CAMERA_VEC3& camera, const STAGE& stage, float margin);

/// @brief 今の点を、目標の点へなめらかに近づける
/// @details 1秒ごとに、残りの距離が e^(-speed) 倍に縮む（dt が変わっても同じ動きになり、行き過ぎない）
/// @param current 今の点
/// @param target 目標の点
/// @param speed 近づく速さ（大きいほど速く追いつく。0 なら動かない）
/// @param dt 1フレームの秒数
/// @return 近づけたあとの点
CAMERA_VEC3 CameraCalcSmooth(const CAMERA_VEC3& current, const CAMERA_VEC3& target, float speed, float dt);
