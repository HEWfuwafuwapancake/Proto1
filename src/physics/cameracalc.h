#pragma once

#include "physics/stagecalc.h"
#include "physics/vec3.h"

/// @file cameracalc.h
/// @brief TPS カメラの計算（位置・浮島との接触・臨界減衰バネ・No.33 用のフォーカス）
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

/// @brief 臨界減衰バネで目標へ近づけ、行き過ぎるときは目標で止める
/// @details 速度は呼ぶ側で値ごとに持ち、目標が変わっても初期化せず次の呼び出しへ渡す
/// @param current 今の値
/// @param target 目標の値
/// @param[in,out] velocity 1秒あたりの速度（初回・瞬間移動時は0）
/// @param smoothTime 追いつくまでのおよその秒数（0以下は0.0001秒として扱う）
/// @param dt 1フレームの秒数（0以下なら値も速度も変えない）
/// @return 近づけたあとの値
float CameraCalcSmoothDamp(float current, float target, float& velocity, float smoothTime, float dt);

/// @brief 角度の差を最短方向に直し、臨界減衰バネで目標へ近づける
/// @param current 今の角度（ラジアン）
/// @param target 目標の角度（ラジアン）
/// @param[in,out] velocity 角速度（ラジアン/秒。初回・瞬間移動時は0）
/// @param smoothTime 追いつくまでのおよその秒数（0以下は0.0001秒として扱う）
/// @param dt 1フレームの秒数（0以下なら値も速度も変えない）
/// @return 近づけたあとの角度（-π〜π。dtが0以下ならcurrentのまま）
float CameraCalcSmoothDampAngle(float current, float target, float& velocity, float smoothTime, float dt);

/// @brief フォーカス候補（配列の添字がダミー番号）
struct CAMERA_FOCUS_TARGET
{
	VEC3 Position; ///< 敵の中心位置
	bool Available; ///< true：選べる、false：選べない
};

/// @brief from から to への水平角を返す（水平距離がほぼ0なら fallbackYaw）
/// @param from 向きの始点
/// @param to 向きの終点
/// @param fallbackYaw 水平距離がほぼ0のときの角度（ラジアン）
/// @return 水平角（ラジアン。0で+Z方向、プラスで右回り）
float CameraCalcYawTo(const VEC3& from, const VEC3& to, float fallbackYaw);
/// @brief yaw から maxAngle ラジアン・3D距離 maxDistance 以内の最も中央の敵を選ぶ。current は bonus ラジアン優先（なしは -1）
/// @param yaw カメラの水平角（ラジアン）
/// @param player プレイヤーの注視点
/// @param targets 候補の配列（countが0ならnullptrでもよい）
/// @param count 候補の数
/// @param current 今の候補番号（なしは-1）
/// @param maxAngle 中央から選べる角度（ラジアン）
/// @param maxDistance 選べる3D距離
/// @param bonus 今の候補を優先する角度（ラジアン）
/// @return 選んだ候補の添字（選べる敵がなければ-1）
int CameraCalcPickFocus(float yaw, const VEC3& player, const CAMERA_FOCUS_TARGET* targets,
	int count, int current, float maxAngle, float maxDistance, float bonus);

/// @brief 今のターゲットへの向きを基準に、指定した側で角度が最も近い敵へ切り替える
/// @param player プレイヤーの位置（高さは使わない）
/// @param targets 候補の配列（countが0ならnullptrでもよい）
/// @param count 候補の数
/// @param current 今のターゲット番号
/// @param direction 切り替える側（負：左、正：右、0：切り替えない）
/// @return 次の敵の添字。その側に敵がいなければcurrent、currentが無効・選択不可なら-1
int CameraCalcNextTarget(const VEC3& player, const CAMERA_FOCUS_TARGET* targets,
	int count, int current, int direction);

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
/// @return プレイヤー全身が画面に収まる範囲で敵へずらした注視点
VEC3 CameraCalcFocusPoint(const VEC3& player, const VEC3& enemy, float yaw, float pitch,
	float distance, float fov, float aspect, float margin, float radius, float rate);
