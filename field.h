#pragma once

#include "dx11.h"

/// @file field.h
/// @brief 地面（くぼ地のある地形）
/// @details 原点を中心に砂のくぼ地があり、まわりはなだらかな丘になっている（HEW2026GitTest の湖の地形から水をなくしたもの。企画に合わせて作り直してよい）

/// @brief 地形の頂点・インデックスバッファを作る
void FieldInitialize(void);

/// @brief 地形のバッファを解放する
void FieldFinalize(void);

/// @brief 地形の更新（今は何もしない）
void FieldUpdate(void);

/// @brief 地形を描く
void FieldDraw(void);

/// @brief 地形の高さを取得する
/// @param x 位置の x（ワールド座標）
/// @param z 位置の z（ワールド座標）
/// @return その位置の地面の y
float FieldGetHeight(float x, float z);

/// @brief 高さに応じた地面の色を取得する（砂浜 → 草 → 濃い草）
/// @param height 地面の高さ
/// @return 地面の色（RGB）
XMFLOAT3 FieldGetColor(float height);
