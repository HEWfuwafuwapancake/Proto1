#pragma once

#include <cstdint>
#include "physics/stagecalc.h"

/// @file mapcalc.h
/// @brief 対戦マップ（ソラツギ中央郵便局・汐跡市場の三本橋）の配置台帳と、南の橋の空き缶の合図
/// @details 座標は docs/plan/08_対戦マップ案/実装基準.md の第08・09節。DirectX と Windows を使わないので単体テストできる

/// @brief 試合で使うマップ
enum STAGE_MAP : uint8_t
{
	STAGE_MAP_POST_OFFICE,	///< ソラツギ中央郵便局（近未来都市）
	STAGE_MAP_MARKET,		///< 汐跡市場の三本橋（荒廃都市）
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

/// @brief 地形のマップ（郵便局・市場）の配置台帳を、指定の倍率でステージに書き込む
/// @details 区画・開始位置・Ｇ・補給の地点・ベルト・空き缶の XZ を倍率倍にする（高さはそのまま）。
///          浮島（STAGE_MAP_ISLAND）は何もしない（game/stage.cpp の値で作る）
/// @param map マップ
/// @param scale マップの広さの倍率（設計書のサイズ設定 s）
/// @param stage 書き込む先
/// @return 地形のマップを書き込んだら true（浮島・範囲外は false）
bool MapCalcBuild(STAGE_MAP map, float scale, STAGE& stage);

/// @brief 南の橋の空き缶（新しく乗った人がいると鳴って揺れる。鳴ったあとしばらくは鳴らない）
class CanBellCalc
{
public:
	/// @brief 鳴ってから次に鳴れるまでの更新数
	static constexpr int INTERVAL_FRAMES = 120;
	/// @brief 揺れが続く更新数
	static constexpr int SHAKE_FRAMES = 60;
	/// @brief 試合の最初に戻す（だれも乗っていない・まだ鳴っていない）
	void Reset();
	/// @brief 1更新の終わりに、各人が南の橋の上面に立っているかを渡して進める
	/// @param frame 試合の更新番号
	/// @param onBridge 人ごとに、この更新の終わりに南の橋の上面に立っているか（FIGHTER_COUNT 人分）
	/// @param count 人数
	/// @return この更新で鳴ったら true（同じ更新に何人乗っても1回）
	bool Step(int frame, const bool* onBridge, int count);
	/// @brief 揺れの傾きを取得する（ラジアン。鳴ってから SHAKE_FRAMES 更新のあとは 0）
	/// @param frame 試合の更新番号
	float GetTilt(int frame) const;
	/// @brief 最後に鳴った更新番号を取得する（まだなら -1）
	int GetLastFrame() const { return m_LastFrame; }

private:
	bool m_OnBridge[8] = {};
	int m_LastFrame = -1;
};
