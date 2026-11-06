#include <gtest/gtest.h>
#include "physics/guardcalc.h"

namespace Proto1Tests
{
	// 前だけのガードは前からの攻撃だけ防ぎ、横・後ろからは防がない。全方向は全部防ぐ。
	TEST(GuardCalcTest, Blocks_FrontOnlyAndAll)
	{
		GUARD front;
		front.Kind = GUARD_FRONT;
		front.Forward = { 0.0f, 0.0f, 1.0f };
		front.HalfAngle = 3.14159265f / 3.0f;
		const VEC3 center{ 0.0f, 1.0f, 0.0f };
		EXPECT_TRUE(GuardCalcBlocks(front, center, { 0.5f, 1.0f, 3.0f }));
		EXPECT_FALSE(GuardCalcBlocks(front, center, { 3.0f, 1.0f, 0.0f }));
		EXPECT_FALSE(GuardCalcBlocks(front, center, { 0.0f, 1.0f, -3.0f }));
		GUARD all;
		all.Kind = GUARD_ALL;
		EXPECT_TRUE(GuardCalcBlocks(all, center, { 0.0f, 1.0f, -3.0f }));
		EXPECT_FALSE(GuardCalcBlocks(GUARD{}, center, { 0.0f, 1.0f, 3.0f }));
		EXPECT_EQ((int)GUARD_ALL, (int)GuardCalcStronger(front, all).Kind);
	}
}
