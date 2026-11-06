#include <gtest/gtest.h>
#include "physics/knucklecalc.h"

namespace Proto1Tests
{
	static constexpr KNUCKLE_PARAMS KNUCKLE_TEST_PARAMS{ 1.0f, { 10.0f, 2.2f, false }, { 14.0f, 2.8f, true } };

	// 至近は杭で強く、離れていれば普通の拳。
	TEST(KnuckleCalcTest, Pick_PileWhenClose)
	{
		const KNUCKLE_HIT pile = KnuckleCalcPick(0.8f, KNUCKLE_TEST_PARAMS);
		EXPECT_TRUE(pile.Pile);
		EXPECT_EQ(14.0f, pile.Damage);
		const KNUCKLE_HIT normal = KnuckleCalcPick(1.5f, KNUCKLE_TEST_PARAMS);
		EXPECT_FALSE(normal.Pile);
		EXPECT_EQ(2.2f, normal.Power);
	}
}
