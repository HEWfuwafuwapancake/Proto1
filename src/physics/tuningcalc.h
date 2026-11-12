#pragma once

#include <array>
#include <cstdint>

/// @brief 試合の前にホストが決める項目
enum TUNING_ITEM : uint8_t
{
	TUNING_RUSH_DISTANCE, ///< 長押しの移動距離の倍率
	TUNING_KNOCKBACK, ///< 吹っ飛び力の倍率
	TUNING_HAMMER_STUN, ///< ハンマーの長押しのしびれの秒数
	TUNING_DAMAGE, ///< ダメージの倍率
	TUNING_MAP_SIZE, ///< マップの広さの倍率
	TUNING_CAPSULES, ///< 腕カプセルの出現量の段
	TUNING_ARM_USES, ///< 腕の使用回数の倍率（最後は無制限）
	TUNING_ITEM_COUNT, ///< 項目数
};

/// @brief 試合の設定。通信には項目ごとの段だけを送る
struct MATCH_TUNING
{
	std::array<uint8_t, TUNING_ITEM_COUNT> Levels{}; ///< 項目ごとの段（0から）
};

/// @brief 全項目がふつうの段の設定を取得する
MATCH_TUNING TuningCalcDefault();
/// @brief 項目の段の数を取得する（項目が範囲外なら0）
int TuningCalcLevelCount(TUNING_ITEM item);
/// @brief 項目のふつうの段を取得する（項目が範囲外なら0）
int TuningCalcDefaultLevel(TUNING_ITEM item);
/// @brief 全項目の段が範囲内ならtrueを返す
bool TuningCalcIsValid(const MATCH_TUNING& tuning);
/// @brief 段の倍率・秒を取得する（段が範囲外ならふつう。カプセルと無制限の使用回数は専用関数を使う）
float TuningCalcValue(const MATCH_TUNING& tuning, TUNING_ITEM item);
/// @brief 試合の最初に出すカプセルの数を取得する
int TuningCalcCapsuleCount(const MATCH_TUNING& tuning);
/// @brief カプセルが1つ出る間隔の秒を取得する
float TuningCalcCapsuleSeconds(const MATCH_TUNING& tuning);
/// @brief 腕の使用回数が無制限の段ならtrueを返す
bool TuningCalcIsUnlimitedUses(const MATCH_TUNING& tuning);
/// @brief 元の使用回数に倍率をかけて切り上げる（素手は0、それ以外は1以上。無制限の段は元の回数）
int TuningCalcArmUses(int baseUses, const MATCH_TUNING& tuning);
