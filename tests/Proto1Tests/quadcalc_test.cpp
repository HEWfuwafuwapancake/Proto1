#include <gtest/gtest.h>
#include "render/quadcalc.h"

namespace Proto1Tests
{
	// 斜めのボタン：上の辺は右へ、下の辺は左へ半分ずつずれ、高さは変わらない
	TEST(QuadCalcTest, Skew_ShiftsTopRightAndBottomLeft)
	{
		const QUAD2D quad = QuadCalcSkew({ 100.0f, 50.0f, 200.0f, 40.0f }, 20.0f);
		EXPECT_NEAR(quad.X[0], 110.0f, 0.0001f);
		EXPECT_NEAR(quad.X[1], 310.0f, 0.0001f);
		EXPECT_NEAR(quad.X[2], 90.0f, 0.0001f);
		EXPECT_NEAR(quad.X[3], 290.0f, 0.0001f);
		EXPECT_NEAR(quad.Y[0], 50.0f, 0.0001f);
		EXPECT_NEAR(quad.Y[3], 90.0f, 0.0001f);
	}

	// 傾き：プラスの角度で画面の時計回り（右の角が下へ動く）に回る
	TEST(QuadCalcTest, Rotate_TurnsClockwiseOnScreen)
	{
		const QUAD2D quad = QuadCalcRect({ -10.0f, -5.0f, 20.0f, 10.0f });
		const QUAD2D turned = QuadCalcRotate(quad, 0.0f, 0.0f, 3.14159265f * 0.5f);
		// 右上（10, -5）は、時計回りに 90 度で（5, 10）になる。
		EXPECT_NEAR(turned.X[1], 5.0f, 0.0001f);
		EXPECT_NEAR(turned.Y[1], 10.0f, 0.0001f);
	}
}
