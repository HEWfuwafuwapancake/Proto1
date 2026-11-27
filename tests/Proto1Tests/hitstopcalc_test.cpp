#include <gtest/gtest.h>
#include "physics/hitstopcalc.h"

namespace
{
	const HITSTOP_PARAMS PARAMS{ 3.0f, 6.0f, 11.0f, 24.0f, 3, 6, 10, 14 };
}

// ダメージ%で弱・中・強に分かれ、速く吹っ飛ぶ一撃は決め手になり、小さな当たりは止めない。
TEST(HitstopCalcTest, FramesFollowHitStrength)
{
	EXPECT_EQ(0, HitstopCalcFrames(2.0f, 30.0f, PARAMS));
	EXPECT_EQ(3, HitstopCalcFrames(5.0f, 5.0f, PARAMS));
	EXPECT_EQ(6, HitstopCalcFrames(9.0f, 10.0f, PARAMS));
	EXPECT_EQ(10, HitstopCalcFrames(18.0f, 20.0f, PARAMS));
	EXPECT_EQ(14, HitstopCalcFrames(5.0f, 24.0f, PARAMS));
}

// 決めたフレーム数だけ止まってから動き出し、重なったときは長い方を残す（足さない）。
TEST(HitstopCalcTest, CountsDownAndKeepsLongerStop)
{
	HitstopCalc calc;
	calc.Start(3);
	calc.Start(2);
	EXPECT_TRUE(calc.Step());
	EXPECT_TRUE(calc.Step());
	EXPECT_TRUE(calc.Step());
	EXPECT_FALSE(calc.Step());
	calc.Start(6);
	EXPECT_EQ(6, calc.GetFrames());
}
