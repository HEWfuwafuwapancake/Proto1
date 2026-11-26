#pragma once

#include <cstdint>
#include "physics/stagecalc.h"

/// @file mapcalc.h
/// @brief 対戦マップの配置台帳（地形の区画・開始位置・Ｇ・ダッシュ板）
/// @details 郵便局・市場の座標は docs/plan/08_対戦マップ案/実装基準.md の第08・09節。
///          ネオンビル群・コロシアム・廃駅は docs/plan/09_追加マップ3面/ の仕様書。DirectX と Windows を使わないので単体テストできる

/// @brief 試合で使うマップ（マップ選択はこの順に並ぶ）
enum STAGE_MAP : uint8_t
{
	STAGE_MAP_POST_OFFICE,	///< ソラツギ中央郵便局（近未来都市）
	STAGE_MAP_MARKET,		///< 汐跡市場の三本橋（荒廃都市）
	STAGE_MAP_NEON,			///< ネオンビル群（交差点とビル。ジャンプ台で屋上へ上がれる）
	STAGE_MAP_COLISEUM,		///< コロシアム（円い闘技場。攻撃で倒れる柱が5本）
	STAGE_MAP_STATION,		///< 廃駅（2つのホームと線路。倒壊ビルの斜めの足場）
	STAGE_MAP_ISLAND,		///< 今までの浮島と高台（デバッグ用の標準）
	STAGE_MAP_COUNT,		///< マップの数
};

/// @brief 最初に選ばれているマップ
static constexpr STAGE_MAP STAGE_MAP_DEFAULT = STAGE_MAP_POST_OFFICE;

/// @brief 左右の入力でマップを一周させる
/// @param map 今のマップ
/// @param direction 切り替える向き（左 -1、右 +1、なし 0）
/// @return 次のマップ
STAGE_MAP MapCalcCycle(STAGE_MAP map, int direction);

/// @brief 地形のマップ（浮島以外）の配置台帳を、指定の倍率でステージに書き込む
/// @details 区画・開始位置・Ｇ・ダッシュ板の XZ を倍率倍にする（高さはそのまま）。
///          浮島（STAGE_MAP_ISLAND）は何もしない（game/stage.cpp の値で作る）
/// @param map マップ
/// @param scale マップの広さの倍率（設計書のサイズ設定 s）
/// @param stage 書き込む先
/// @return 地形のマップを書き込んだら true（浮島・範囲外は false）
bool MapCalcBuild(STAGE_MAP map, float scale, STAGE& stage);

/// @brief 地形の区画の四角から、ほかの区画の四角を除いた残りを求める（描くときに、ほかの区画の下に隠れる床を作らない）
/// @param minX 元の四角の X の最小
/// @param maxX 元の四角の X の最大
/// @param minZ 元の四角の Z の最小
/// @param maxZ 元の四角の Z の最大
/// @param holes 除く四角（[i][0..3] が minX・maxX・minZ・maxZ）
/// @param holeCount 除く四角の数
/// @param pieces 残りの四角を書き込む先（[i][0..3] が minX・maxX・minZ・maxZ）
/// @param maxPieces 書き込める四角の最大数
/// @return 残りの四角の数（maxPieces を超える分は捨てる）
int MapCalcSubtractRects(float minX, float maxX, float minZ, float maxZ, const float (*holes)[4], int holeCount,
	float (*pieces)[4], int maxPieces);
