#pragma once

#include "core/dx11.h"

/// @file shape.h
/// @brief 箱・球・カプセルを、位置・大きさ・色を指定して描く部品（仮の形のプレイヤー・ダミー・触腕などに使う）
/// @details 形ごとに1つだけ頂点バッファを作っておき、描くたびにワールド行列で大きさと位置を変える

/// @brief 箱・球・円柱の頂点バッファとインデックスバッファを作る
void ShapeInitialize(void);

/// @brief 形のバッファを解放する
void ShapeFinalize(void);

/// @brief 箱を描く
/// @param center 箱の中心（ワールド座標）
/// @param size 箱の大きさ（X・Y・Z それぞれの長さ）
/// @param color 色（RGBA）
void ShapeDrawBox(const XMFLOAT3& center, const XMFLOAT3& size, const XMFLOAT4& color);

/// @brief 球を描く
/// @param center 球の中心（ワールド座標）
/// @param radius 球の半径
/// @param color 色（RGBA）
void ShapeDrawSphere(const XMFLOAT3& center, float radius, const XMFLOAT4& color);

/// @brief 縦向きのカプセル（円柱の上下に半球が付いた形）を描く
/// @param bottom カプセルのいちばん下の点（ワールド座標。足元）
/// @param radius カプセルの半径
/// @param height カプセルの高さ（半球も含めた全体の高さ。半径の2倍より短いときは球になる）
/// @param color 色（RGBA）
void ShapeDrawCapsule(const XMFLOAT3& bottom, float radius, float height, const XMFLOAT4& color);

/// @brief 始点と終点を結ぶ細い円柱を描く（既存の円柱の側面を使う）
/// @param start 始点のワールド座標
/// @param end 終点のワールド座標（始点と同じなら描かない）
/// @param radius 円柱の半径（0 以下なら描かない）
/// @param color 色（RGBA）
void ShapeDrawCylinder(const XMFLOAT3& start, const XMFLOAT3& end, float radius, const XMFLOAT4& color);
