#pragma once

#include "core/dx11.h"

/// @file camera.h
/// @brief カメラ（仮：プレイヤーの後ろ上から追いかける。きちんとした TPS カメラは No.9 で作る）
/// @details
///   - マウス・矢印キー・右スティック：プレイヤーのまわりを回る（感度は action.cpp の「調整用の値」）
///   - 位置はプレイヤーの頭のあたりから、見ている向きの反対へ一定の距離だけ下がったところ（camera.cpp の FOLLOW_DISTANCE）
///   - 浮島やほかの物とはぶつからない（めり込むことがある）

/// @brief カメラの向きを最初の状態にし、プレイヤーの後ろに置く（PlayerInitialize のあとに呼ぶ）
void CameraInitialize(void);

/// @brief カメラの終了処理（今は何もしない）
void CameraFinalize(void);

/// @brief 「カメラを回す」行動で向きを変え、プレイヤーの後ろ上に置く
/// @details 毎フレーム、PlayerUpdate のあとに呼ぶ（動いたあとのプレイヤーを追いかける）
void CameraUpdate(void);

/// @brief ビュー行列とプロジェクション行列をシェーダーに設定する
/// @details 3D描画の前に呼ぶ
void CameraDraw(void);

/// @brief カメラの位置を取得する
/// @return カメラの位置（ワールド座標）
XMFLOAT3 CameraGetPosition(void);

/// @brief カメラの左右の向きを取得する（プレイヤーの移動の向きに使う）
/// @return 左右の向き（ラジアン。0 で +Z 方向、プラスで右回り）
float CameraGetYaw(void);
