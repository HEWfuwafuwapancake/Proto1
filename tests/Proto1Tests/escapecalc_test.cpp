#include <gtest/gtest.h>
#include "physics/escapecalc.h"

// 必要な6回で抜け、掴まれ直すと前の連打を引き継がない。
TEST(EscapeCalcTest, ButtonsEscapeAtRequiredCountAndReset)
{
	EscapeCalc calc;
	for (int i = 0; i < 5; ++i) EXPECT_FALSE(calc.Step(1, 0.0f, 0.0f));
	EXPECT_TRUE(calc.Step(1, 0.0f, 0.0f));
	calc.Reset();
	EXPECT_FALSE(calc.Step(1, 0.0f, 0.0f));
}

// % に応じて回数が切り上げで増え、高い % でも14回で抜ける。
TEST(EscapeCalcTest, HigherDamageRequiresMorePressesWithLimit)
{
	EscapeCalc calc;
	EXPECT_FALSE(calc.Step(7, 0.0f, 26.0f));
	EXPECT_TRUE(calc.Step(1, 0.0f, 26.0f));
	calc.Reset();
	EXPECT_FALSE(calc.Step(13, 0.0f, 500.0f));
	EXPECT_TRUE(calc.Step(1, 0.0f, 500.0f));
}

// 倒しっぱなしでは連打にならず、戻して倒し直すと数える。
TEST(EscapeCalcTest, StickCountsOnlyAfterReturningToCenter)
{
	EscapeCalc calc;
	EXPECT_FALSE(calc.Step(0, 1.0f, 0.0f));
	for (int i = 0; i < 10; ++i) EXPECT_FALSE(calc.Step(0, 1.0f, 0.0f));
	EXPECT_FALSE(calc.Step(0, 0.5f, 0.0f));
	EXPECT_FALSE(calc.Step(0, 1.0f, 0.0f));
	EXPECT_FALSE(calc.Step(0, 0.3f, 0.0f));
	EXPECT_TRUE(calc.Step(4, 1.0f, 0.0f));
}
