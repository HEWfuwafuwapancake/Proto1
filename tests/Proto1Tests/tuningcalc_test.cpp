#include <gtest/gtest.h>
#include "physics/tuningcalc.h"

namespace Proto1Tests
{
	// ふつうの設定は現在のゲームの値と一致する。
	TEST(TuningCalcTest, DefaultsPreserveCurrentGame)
	{
		const MATCH_TUNING tuning = TuningCalcDefault();
		EXPECT_TRUE(TuningCalcIsValid(tuning));
		for (TUNING_ITEM item : { TUNING_RUSH_DISTANCE, TUNING_KNOCKBACK, TUNING_DAMAGE, TUNING_MAP_SIZE }) {
			EXPECT_FLOAT_EQ(1.0f, TuningCalcValue(tuning, item));
		}
		EXPECT_FLOAT_EQ(1.2f, TuningCalcValue(tuning, TUNING_HAMMER_STUN));
		EXPECT_EQ(8, TuningCalcCapsuleCount(tuning));
		EXPECT_FLOAT_EQ(1.5f, TuningCalcCapsuleSeconds(tuning));
		EXPECT_EQ(3, TuningCalcArmUses(3, tuning));
		EXPECT_FALSE(TuningCalcIsUnlimitedUses(tuning));
	}

	// 回数は切り上げ、素手を増やさず、無制限の段を区別する。
	TEST(TuningCalcTest, UsesRoundUpAndSupportUnlimited)
	{
		MATCH_TUNING tuning = TuningCalcDefault();
		tuning.Levels[TUNING_ARM_USES] = 0;
		EXPECT_EQ(2, TuningCalcArmUses(3, tuning));
		tuning.Levels[TUNING_ARM_USES] = 3;
		EXPECT_EQ(16, TuningCalcArmUses(8, tuning));
		EXPECT_EQ(0, TuningCalcArmUses(0, tuning));
		tuning.Levels[TUNING_ARM_USES] = 4;
		EXPECT_TRUE(TuningCalcIsUnlimitedUses(tuning));
	}

	// 各項目の段を表どおり選べ、範囲外の通信値は拒否する。
	TEST(TuningCalcTest, LevelsValidateEveryItem)
	{
		const int counts[] = { 6, 6, 7, 6, 5, 4, 5 };
		for (int i = 0; i < TUNING_ITEM_COUNT; ++i) {
			const auto item = static_cast<TUNING_ITEM>(i);
			MATCH_TUNING tuning = TuningCalcDefault();
			EXPECT_EQ(counts[i], TuningCalcLevelCount(item));
			EXPECT_EQ(tuning.Levels[i], TuningCalcDefaultLevel(item));
			tuning.Levels[i] = static_cast<uint8_t>(counts[i] - 1);
			EXPECT_TRUE(TuningCalcIsValid(tuning));
			tuning.Levels[i] = static_cast<uint8_t>(counts[i]);
			EXPECT_FALSE(TuningCalcIsValid(tuning));
		}
	}

	// 選んだ段で移動・攻撃・ステージ・カプセルの値が変わる。
	TEST(TuningCalcTest, SelectedLevelsChangeMatchValues)
	{
		MATCH_TUNING tuning = TuningCalcDefault();
		tuning.Levels[TUNING_RUSH_DISTANCE] = 5;
		tuning.Levels[TUNING_KNOCKBACK] = 0;
		tuning.Levels[TUNING_DAMAGE] = 3;
		tuning.Levels[TUNING_HAMMER_STUN] = 0;
		tuning.Levels[TUNING_MAP_SIZE] = 4;
		tuning.Levels[TUNING_CAPSULES] = 3;
		EXPECT_FLOAT_EQ(2.0f, TuningCalcValue(tuning, TUNING_RUSH_DISTANCE));
		EXPECT_FLOAT_EQ(0.5f, TuningCalcValue(tuning, TUNING_KNOCKBACK));
		EXPECT_FLOAT_EQ(1.25f, TuningCalcValue(tuning, TUNING_DAMAGE));
		EXPECT_FLOAT_EQ(0.0f, TuningCalcValue(tuning, TUNING_HAMMER_STUN));
		EXPECT_FLOAT_EQ(1.5f, TuningCalcValue(tuning, TUNING_MAP_SIZE));
		EXPECT_EQ(16, TuningCalcCapsuleCount(tuning));
		EXPECT_FLOAT_EQ(0.6f, TuningCalcCapsuleSeconds(tuning));
	}
}
