#pragma once

#include "core/dx11.h"

/// @file shape.h
/// @brief 箱・球・カプセルを、位置・大きさ・色を指定して描く部品（仮の形のプレイヤー・ダミー・触腕などに使う）
/// @details 形ごとに1つだけ頂点バッファを作っておき、描くたびにワールド行列で大きさと位置を変える

/// @brief 箱・球・円柱・上面が斜めの箱の頂点バッファとインデックスバッファを作る
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

/// @brief 始点と終点を結ぶ向きに伸ばした箱を描く（ShapeDrawCylinder の箱の版）
/// @param start 始点（箱の一方の端の面の中心）
/// @param end 終点（箱のもう一方の端の面の中心）
/// @param width 箱の幅（伸ばす向きに直角で、水平に近い向きの長さ）
/// @param depth 箱の厚さ（伸ばす向きと幅の両方に直角な向きの長さ）
/// @param color 色（RGBA）
void ShapeDrawBoxAlong(const XMFLOAT3& start, const XMFLOAT3& end, float width, float depth, const XMFLOAT4& color);

/// @brief 底面が水平で、上面が斜めの平面になった箱を描く（坂の地形に使う。横から見ると台形や三角形になる）
/// @details 上面の高さは H = centerTopY + slopeX × (x − 範囲の中心の x) + slopeZ × (z − 範囲の中心の z)。
///          H が底面より低くなる所は底面の高さにそろえる。
/// @param minX 範囲の X の小さい側
/// @param maxX 範囲の X の大きい側
/// @param minZ 範囲の Z の小さい側
/// @param maxZ 範囲の Z の大きい側
/// @param bottomY 底面の高さ
/// @param centerTopY 範囲の中心での上面の高さ
/// @param slopeX 上面の高さが x 1 あたりに増える量
/// @param slopeZ 上面の高さが z 1 あたりに増える量
/// @param color 色（RGBA）
void ShapeDrawSlopedBox(float minX, float maxX, float minZ, float maxZ, float bottomY,
	float centerTopY, float slopeX, float slopeZ, const XMFLOAT4& color);
