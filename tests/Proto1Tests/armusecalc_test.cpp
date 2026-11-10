#include "gtest/gtest.h"
#include "physics/armusecalc.h"

// 片手ずつ、腕に設定された回数から使った分だけ減る。
TEST(ArmUseCalcTest, ConsumesUsesIndependently)
{
	ArmUseCalc left, right;
	left.Reset(8);
	right.Reset(3);
	left.Consume(3);
	EXPECT_EQ(left.GetRemaining(), 5);
	EXPECT_EQ(right.GetRemaining(), 3);
	EXPECT_FALSE(left.IsExhausted());
}

// 回数を使い切った状態は 0 で止まり、装備し直すと補充される。
TEST(ArmUseCalcTest, ExhaustsAndResets)
{
	ArmUseCalc uses;
	uses.Reset(2);
	uses.Consume(3);
	EXPECT_EQ(uses.GetRemaining(), 0);
	EXPECT_TRUE(uses.IsExhausted());
	uses.Consume(1);
	EXPECT_EQ(uses.GetRemaining(), 0);
	uses.Reset(2);
	EXPECT_EQ(uses.GetRemaining(), 2);
	EXPECT_FALSE(uses.IsExhausted());
}
