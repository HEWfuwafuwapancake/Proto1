// physics/randomcalc.h（決まった種から作る乱数）の単体テスト
#include <gtest/gtest.h>
#include "physics/randomcalc.h"

// 同じ種なら、整数と実数を交互に引いても同じ並びになる。
TEST(RandomCalcTest, SameSeedProducesSameSequence)
{
	RandomCalc first, second;
	first.SetSeed(42u);
	second.SetSeed(42u);
	for (int i = 0; i < 32; ++i) {
		EXPECT_EQ(first.NextInt(12), second.NextInt(12));
		EXPECT_FLOAT_EQ(first.NextFloat(-11.0f, 11.0f), second.NextFloat(-11.0f, 11.0f));
	}
}

// 種に 0 を渡しても乱数が止まらず、整数と実数は指定の範囲に収まる。
TEST(RandomCalcTest, ZeroSeedProducesValuesWithinBounds)
{
	RandomCalc random;
	random.SetSeed(0u);
	const int first = random.NextInt(12);
	EXPECT_GE(first, 0);
	EXPECT_LT(first, 12);
	bool sawDifferentValue = false;
	for (int i = 0; i < 32; ++i) {
		const int index = random.NextInt(12);
		EXPECT_GE(index, 0);
		EXPECT_LT(index, 12);
		if (index != first) sawDifferentValue = true;
		const float position = random.NextFloat(-11.0f, 11.0f);
		EXPECT_GE(position, -11.0f);
		EXPECT_LE(position, 11.0f);
	}
	EXPECT_TRUE(sawDifferentValue);
}
