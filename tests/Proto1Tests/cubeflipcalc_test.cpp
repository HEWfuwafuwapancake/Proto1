#include <gtest/gtest.h>
#include "render/cubeflipcalc.h"

namespace Proto1Tests
{
	TEST(CubeFlipCalcTest, Flip_KeepsTotalHeightAndArrangesFacesInBothDirections)
	{
		constexpr float height = 32.0f;
		constexpr float tolerance = 0.0001f;
		const CUBE_FLIP start = CubeFlipCalc(0.0f, height, 1);
		const CUBE_FLIP down = CubeFlipCalc(0.5f, height, 1);
		const CUBE_FLIP up = CubeFlipCalc(0.5f, height, -1);
		const CUBE_FLIP end = CubeFlipCalc(1.0f, height, 1);
		EXPECT_NEAR(height, start.Previous.Height, tolerance);
		EXPECT_NEAR(0.0f, start.Next.Height, tolerance);
		EXPECT_NEAR(height * 0.5f, down.Previous.Height, tolerance);
		EXPECT_NEAR(height, down.Previous.Height + down.Next.Height, tolerance);
		EXPECT_NEAR(down.Previous.Height, down.Next.Y, tolerance);
		EXPECT_NEAR(up.Next.Height, up.Previous.Y, tolerance);
		EXPECT_NEAR(0.0f, end.Previous.Height, tolerance);
		EXPECT_NEAR(height, end.Next.Height, tolerance);
	}
}
