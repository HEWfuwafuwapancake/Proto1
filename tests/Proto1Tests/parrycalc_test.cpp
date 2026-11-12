#include <gtest/gtest.h>
#include "physics/parrycalc.h"

// 1/60秒で進め、構えの出だしだけパリィになり、そのあともガードは続く。
TEST(ParryCalcTest, StartParriesThenNormalGuardBreaksArm)
{
	ParryCalc calc;
	const float dt = 1.0f / 60.0f;
	calc.Step(true, dt);
	EXPECT_EQ(PARRY_SUCCESS, calc.OnHit(true));
	for (int i = 1; i < 12; ++i) calc.Step(true, dt);
	EXPECT_EQ(PARRY_SUCCESS, calc.OnHit(true));
	calc.Step(true, dt);
	EXPECT_EQ(PARRY_BREAK_ARM, calc.OnHit(true));
	EXPECT_TRUE(calc.IsGuarding());
	EXPECT_EQ(PARRY_NONE, calc.OnHit(false));
}

// すぐ構え直してもパリィを繰り返せず、解いて0.5秒経つと受け付ける。
TEST(ParryCalcTest, RepeatedGuardNeedsTimeBeforeNextParry)
{
	ParryCalc calc;
	const float dt = 1.0f / 60.0f;
	calc.Step(true, dt);
	calc.Step(false, dt);
	EXPECT_EQ(PARRY_NONE, calc.OnHit(true));
	calc.Step(true, dt);
	EXPECT_EQ(PARRY_BREAK_ARM, calc.OnHit(true));
	for (int i = 0; i < 30; ++i) calc.Step(false, dt);
	calc.Step(true, dt);
	EXPECT_EQ(PARRY_SUCCESS, calc.OnHit(true));
}
