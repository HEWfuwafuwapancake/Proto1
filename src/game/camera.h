#pragma once

#include "core/dx11.h"

/// @brief 描画と同じ、上下を含むカメラの向きを取得する（長さ1）
XMFLOAT3 CameraGetDirection(void);

/// @file camera.h
/// @brief TPS カメラ（プレイヤーの背後から追いかける）
/// @details
///   - マウス・矢印キー・右スティック：プレイヤーのまわりを回る（感度は action.cpp の「調整用の値」）。歩いても自動では回り込まない
///   - 見ている点（プレイヤーの頭のあたり）は、プレイヤーを少し遅れてなめらかに追う（camera.cpp の FOLLOW_SPEED）
///   - カメラは見ている点から、見ている向きの反対へ FOLLOW_DISTANCE 下がったところ。浮島にめり込むときは手前まで近づく
///   - 計算は physics/cameracalc.h

/// @brief カメラの向きを最初の状態にし、プレイヤーの後ろに置く（PlayerInitialize のあとに呼ぶ）
void CameraInitialize(void);

/// @brief カメラの終了処理（今は何もしない）
void CameraFinalize(void);

/// @brief 「カメラを回す」行動で向きを変え、見ている点をプレイヤーへ近づけてから、カメラを置く
/// @details 毎フレーム、PlayerUpdate のあとに呼ぶ（動いたあとのプレイヤーを追いかける）。浮島にめり込むときは手前まで近づける
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
