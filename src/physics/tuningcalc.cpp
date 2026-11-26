#include "physics/tuningcalc.h"
#include <algorithm>
#include <cmath>

// 調整用の値（項目・段の並びは通信でも共通）
static constexpr float SCALE_VALUES[] = { 0.5f, 0.75f, 1.0f, 1.25f, 1.5f, 2.0f };
static constexpr float STUN_VALUES[] = { 0.0f, 0.5f, 0.8f, 1.2f, 1.6f, 2.0f, 3.0f };
static constexpr float MAP_VALUES[] = { 0.7f, 0.85f, 1.0f, 1.2f, 1.5f };
static constexpr float USE_VALUES[] = { 0.5f, 1.0f, 1.5f, 2.0f, 1.0f }; // 最後は無制限として別判定する。
static constexpr int CAPSULE_COUNTS[] = { 4, 8, 12, 16 };
static constexpr float CAPSULE_SECONDS[] = { 3.0f, 1.5f, 1.0f, 0.6f };
// 横の場外の壁：ふつうは、ダメージ 100% の相手にハンマーの長押しを端で当てると届く距離。
// （速さ 24 で上へ約 39 度。空気の抵抗・重力で、何もできない 0.5 秒のあとすぐ戻る向きに入れても横に約 14.6 m 飛ぶ）
static constexpr float RING_OUT_NORMAL = 14.0f;
static constexpr float RING_OUT_RATES[] = { 0.0f, 0.7f, 1.0f, 1.5f, 2.0f }; // 0 は端ぴったりの極端な段
static constexpr int LEVEL_COUNTS[] = { 6, 6, 7, 6, 5, 4, 5, 5 };
static constexpr int DEFAULT_LEVELS[] = { 2, 2, 3, 2, 2, 1, 1, 2 };
static_assert(sizeof(LEVEL_COUNTS) / sizeof(LEVEL_COUNTS[0]) == TUNING_ITEM_COUNT, "LEVEL_COUNTS must match TUNING_ITEM");
static_assert(sizeof(DEFAULT_LEVELS) / sizeof(DEFAULT_LEVELS[0]) == TUNING_ITEM_COUNT, "DEFAULT_LEVELS must match TUNING_ITEM");
static constexpr int UNLIMITED_LEVEL = 4;

int TuningCalcLevelCount(TUNING_ITEM item)
{
	return item < TUNING_ITEM_COUNT ? LEVEL_COUNTS[item] : 0;
}

int TuningCalcDefaultLevel(TUNING_ITEM item)
{
	return item < TUNING_ITEM_COUNT ? DEFAULT_LEVELS[item] : 0;
}

MATCH_TUNING TuningCalcDefault()
{
	MATCH_TUNING tuning;
	for (int i = 0; i < TUNING_ITEM_COUNT; ++i) tuning.Levels[i] = static_cast<uint8_t>(DEFAULT_LEVELS[i]);
	return tuning;
}

bool TuningCalcIsValid(const MATCH_TUNING& tuning)
{
	for (int i = 0; i < TUNING_ITEM_COUNT; ++i) {
		if (tuning.Levels[i] >= LEVEL_COUNTS[i]) return false;
	}
	return true;
}

static int Level(const MATCH_TUNING& tuning, TUNING_ITEM item)
{
	return tuning.Levels[item] < LEVEL_COUNTS[item] ? tuning.Levels[item] : DEFAULT_LEVELS[item];
}

float TuningCalcValue(const MATCH_TUNING& tuning, TUNING_ITEM item)
{
	if (item >= TUNING_ITEM_COUNT) return 1.0f;
	const int level = Level(tuning, item);
	switch (item) {
	case TUNING_HAMMER_STUN: return STUN_VALUES[level];
	case TUNING_MAP_SIZE: return MAP_VALUES[level];
	case TUNING_ARM_USES: return USE_VALUES[level];
	case TUNING_RING_OUT: return RING_OUT_NORMAL * RING_OUT_RATES[level];
	case TUNING_CAPSULES: return static_cast<float>(CAPSULE_COUNTS[level]);
	default: return SCALE_VALUES[level];
	}
}

int TuningCalcCapsuleCount(const MATCH_TUNING& tuning)
{
	return CAPSULE_COUNTS[Level(tuning, TUNING_CAPSULES)];
}

float TuningCalcCapsuleSeconds(const MATCH_TUNING& tuning)
{
	return CAPSULE_SECONDS[Level(tuning, TUNING_CAPSULES)];
}

bool TuningCalcIsUnlimitedUses(const MATCH_TUNING& tuning)
{
	return Level(tuning, TUNING_ARM_USES) == UNLIMITED_LEVEL;
}

int TuningCalcDurabilityCost(int baseCost, const MATCH_TUNING& tuning)
{
	return baseCost <= 0 ? 0 : (std::max)(1, static_cast<int>(std::ceil(baseCost / TuningCalcValue(tuning, TUNING_ARM_USES))));
}
