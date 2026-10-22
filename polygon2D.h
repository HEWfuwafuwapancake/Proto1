#pragma once

/// @file polygon2D.h
/// @brief 2Dポリゴン（UIなど、画面に重ねて描くもののサンプル）

/// @brief 2Dポリゴンの頂点バッファを作る
void Polygon2DInitialize();

/// @brief 2Dポリゴンの頂点バッファを解放する
void Polygon2DFinalize();

/// @brief 2Dポリゴンの更新（今は何もしない）
void Polygon2DUpdate();

/// @brief 2Dポリゴンを描く（深度テストを切り、ライトを消してから呼ぶ）
void Polygon2DDraw();
