#include <gtest/gtest.h>
#include "render/menucalc.h"

namespace Proto1Tests
{
	// 背景のカメラ：0 秒で +X 側、1周の 1/4 で +Z 側にいて、高さはいつも中心＋height
	TEST(MenuCalcTest, OrbitEye_GoesAroundCenterAtFixedHeight)
	{
		const MENU_POINT center{ 1.0f, 2.0f, 3.0f };
		const MENU_POINT start = MenuCalcOrbitEye(0.0f, center, 10.0f, 5.0f, 40.0f);
		EXPECT_NEAR(start.X, 11.0f, 0.001f);
		EXPECT_NEAR(start.Y, 7.0f, 0.001f);
		EXPECT_NEAR(start.Z, 3.0f, 0.001f);
		const MENU_POINT quarter = MenuCalcOrbitEye(10.0f, center, 10.0f, 5.0f, 40.0f);
		EXPECT_NEAR(quarter.X, 1.0f, 0.001f);
		EXPECT_NEAR(quarter.Z, 13.0f, 0.001f);
		// 1周すると元の場所へ戻る。
		const MENU_POINT round = MenuCalcOrbitEye(40.0f, center, 10.0f, 5.0f, 40.0f);
		EXPECT_NEAR(round.X, 11.0f, 0.001f);
	}

	// 弾み：0 で始まり、途中で 1 を超えて、1 で止まる
	TEST(MenuCalcTest, EaseOutBack_OvershootsThenSettles)
	{
		EXPECT_NEAR(MenuCalcEaseOutBack(0.0f), 0.0f, 0.0001f);
		EXPECT_GT(MenuCalcEaseOutBack(0.6f), 1.0f);
		EXPECT_NEAR(MenuCalcEaseOutBack(1.0f), 1.0f, 0.0001f);
	}
}
