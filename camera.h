#pragma once

#include "dx11.h"

/// @file camera.h
/// @brief カメラ（ワールドの中を自由に動けるカメラ）
/// @details
///   - マウス：見回す（感度は camera.cpp の MOUSE_SENSITIVITY）
///   - W/A/S/D：前後左右に移動  Space：上昇  Ctrl：下降  Shift：速く動く
///   - ←→↑↓：キーボードで見回す

/// @brief カメラの位置と向きを最初の状態にする
void CameraInitialize(void);

/// @brief カメラの終了処理（今は何もしない）
void CameraFinalize(void);

/// @brief キーボードとマウスの入力でカメラを動かす
/// @details 立方体からの押し出しと、ワールドの端・地面の制限もここで行う。毎フレーム、KeyboardUpdate / MouseUpdate のあとに呼ぶ
void CameraUpdate(void);

/// @brief ビュー行列とプロジェクション行列をシェーダーに設定する
/// @details 3D描画の前に呼ぶ
void CameraDraw(void);

/// @brief カメラの位置を取得する
/// @return カメラの位置（ワールド座標）
XMFLOAT3 CameraGetPosition(void);
