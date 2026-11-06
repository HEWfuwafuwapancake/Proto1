#include <gtest/gtest.h>
#include "core/letterboxcalc.h"

namespace Proto1Tests
{
	static constexpr float LETTERBOX_EPSILON = 0.001f;

	// 横長のウィンドウは左右に、縦長のウィンドウは上下に、同じ幅の黒い帯ができる。
	TEST(LetterboxCalcTest, Calc_BarsOnLongerSide)
	{
		const LETTERBOX wide = LetterboxCalc(1920.0f, 1000.0f, 1280.0f, 720.0f);
		EXPECT_NEAR(1000.0f / 720.0f, wide.Scale, LETTERBOX_EPSILON);
		EXPECT_NEAR((1920.0f - 1280.0f * wide.Scale) * 0.5f, wide.OffsetX, LETTERBOX_EPSILON);
		EXPECT_NEAR(0.0f, wide.OffsetY, LETTERBOX_EPSILON);
		const LETTERBOX tall = LetterboxCalc(640.0f, 900.0f, 1280.0f, 720.0f);
		EXPECT_NEAR(0.5f, tall.Scale, LETTERBOX_EPSILON);
		EXPECT_NEAR(0.0f, tall.OffsetX, LETTERBOX_EPSILON);
		EXPECT_NEAR((900.0f - 360.0f) * 0.5f, tall.OffsetY, LETTERBOX_EPSILON);
	}
}
