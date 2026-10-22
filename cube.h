#pragma once

#include "dx11.h"

/// @file cube.h
/// @brief 立方体（3D描画のサンプル。インデックスバッファを使った描画）

/// @brief 立方体の頂点バッファとインデックスバッファを作る
void CubeInitialize(void);

/// @brief 立方体のバッファを解放する
void CubeFinalize(void);

/// @brief 立方体を回転させる（毎フレーム呼ぶ）
void CubeUpdate(void);

/// @brief 立方体を描く
void CubeDraw(void);

/// @brief 立方体のワールド行列を取得する
/// @return 拡大縮小 → 回転 → 移動 の順に掛けたワールド行列
XMMATRIX CubeGetWorldMatrix(void);

/// @brief 立方体の色を取得する
/// @return 立方体の色（RGBA）
XMFLOAT4 CubeGetColor(void);

/// @brief 立方体の中心の位置を取得する（カメラの当たり判定に使う）
/// @return 立方体の中心（ワールド座標）
XMFLOAT3 CubeGetPosition(void);

/// @brief 球が立方体と重なっているか調べる
/// @details 立方体の回転と大きさを含めて、箱の形そのもので判定する
/// @param center 球の中心（ワールド座標）
/// @param radius 球の半径
/// @return 重なっていたら true
bool CubeOverlapsSphere(const XMFLOAT3& center, float radius);

/// @brief 球が立方体と重なっていたら、球の中心を立方体の外へ押し出す
/// @details 立方体の回転と大きさを含めて、箱の形そのもので判定する
/// @param[in,out] center 球の中心（ワールド座標）。押し出したときは、押し出したあとの位置に書き換える
/// @param radius 球の半径
/// @return 押し出したら true
bool CubePushOutSphere(XMFLOAT3& center, float radius);
