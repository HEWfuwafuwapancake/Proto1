#pragma once

#include "physics/stagecalc.h"
#include "physics/vec3.h"

/// @file cameracalc.h
/// @brief TPS カメラの計算（位置・浮島との接触・フォーカス・左右角の補助・視点の範囲）
/// @details DirectX と Windows を使わないので、単体テスト（tests/Proto1Tests）からそのまま使える

/// @brief 見ている点のまわりを回るカメラの位置を出す
/// @details 見ている向きは (cos(pitch)·sin(yaw), sin(pitch), cos(pitch)·cos(yaw))。カメラはその反対へ distance 下がったところ
/// @param target 見ている点
/// @param yaw 左右の向き（ラジアン。0 で +Z 方向を見る、プラスで右回り）
/// @param pitch 上下の向き（ラジアン。プラスで上を見る）
/// @param distance 見ている点からカメラまでの距離
/// @return カメラの位置
VEC3 CameraCalcOrbitPosition(const VEC3& target, float yaw, float pitch, float distance);

/// @brief 見ている点からカメラへの線が浮島にぶつかるなら、ぶつかる手前までカメラを近づける
/// @details 浮島の箱を margin だけ大きくして調べる（カメラが面にぴったり付いて、近くのものが切れて見えないように）。
///          見ている点が浮島の箱の中にあるときは、近づけようがないので camera をそのまま返す
/// @param target 見ている点
/// @param camera カメラを置きたい位置
/// @param stage 浮島
/// @param margin 浮島の箱を大きくする量（0 以上）
/// @return めり込まない位置（ぶつからなければ camera のまま）
VEC3 CameraCalcClipToStage(const VEC3& target, const VEC3& camera, const STAGE& stage, float margin);

/// @brief 今の点を、目標の点へなめらかに近づける
/// @details 1秒ごとに、残りの距離が e^(-speed) 倍に縮む（dt が変わっても同じ動きになり、行き過ぎない）
/// @param current 今の点
/// @param target 目標の点
/// @param speed 近づく速さ（大きいほど速く追いつく。0 なら動かない）
/// @param dt 1フレームの秒数
/// @return 近づけたあとの点
VEC3 CameraCalcSmooth(const VEC3& current, const VEC3& target, float speed, float dt);

/// @brief フォーカス候補（配列の添字がダミー番号）
struct CAMERA_FOCUS_TARGET
{
	VEC3 Position; ///< 敵の中心位置
	bool Available; ///< true：選べる、false：選べない
};

/// @brief from から to への水平角を返す（水平距離がほぼ0なら fallbackYaw）
float CameraCalcYawTo(const VEC3& from, const VEC3& to, float fallbackYaw);
/// @brief 最短方向へ毎秒 speed ラジアンまで回す（dt は秒、0以下なら動かない）
float CameraCalcTurnYaw(float current, float target, float speed, float dt);
/// @brief yaw から maxAngle ラジアン・3D距離 maxDistance 以内の最も中央の敵を選ぶ。current は bonus ラジアン優先（なしは -1）
int CameraCalcPickFocus(float yaw, const VEC3& player, const CAMERA_FOCUS_TARGET* targets,
	int count, int current, float maxAngle, float maxDistance, float bonus);
/// @brief player から enemy へ rate の割合だけ視点をずらす。縦横の視野とプレイヤーを囲む半径で上限を決める
/// @param player プレイヤーの注視点（頭付近）
/// @param enemy 敵の中心
/// @param yaw 左右角（ラジアン）
/// @param pitch 上下角（ラジアン）
/// @param distance カメラ距離
/// @param fov 縦視野角（ラジアン）
/// @param aspect 画面の横幅/高さ
/// @param margin 画面端に残す割合（0〜1、各半画面に対する割合）
/// @param radius 注視点を中心にプレイヤー全身を囲む半径
/// @param rate 敵へずらす割合（0〜1）
VEC3 CameraCalcFocusPoint(const VEC3& player, const VEC3& enemy, float yaw, float pitch,
	float distance, float fov, float aspect, float margin, float radius, float rate);
