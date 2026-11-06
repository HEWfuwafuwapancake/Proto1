// core/mathutil.h（Saturate・SmoothStep01・Lerp）の単体テスト
#include <gtest/gtest.h>
#include "core/mathutil.h"

namespace Proto1Tests
{
	static constexpr float EPSILON = 1.0e-6f;	// float の比較で許す誤差

	// カメラとプレイヤーで共用する回転：速さの上限と、±180° をまたぐ最短方向
	TEST(MathUtilTest, TurnYaw_LimitsSpeedAndCrossesPiByShortestPath)
	{
		const float pi = 3.14159265358979323846f;
		EXPECT_NEAR(0.1f, TurnYaw(0, 1, 0.2f, 0.5f), EPSILON);
		EXPECT_NEAR(-pi + 0.05f,
			TurnYaw(pi - 0.05f, -pi + 0.2f, 0.1f, 1), EPSILON);
		EXPECT_NEAR(pi - 0.05f,
			TurnYaw(-pi + 0.05f, pi - 0.2f, 0.1f, 1), EPSILON);
	}

	TEST(MathUtilTest, TurnYaw_DoesNotOvershootOrMoveWithoutTimeOrSpeed)
	{
		EXPECT_NEAR(0.2f, TurnYaw(0, 0.2f, 2, 1), EPSILON);
		EXPECT_NEAR(0.3f, TurnYaw(0.3f, 1, 2, 0), EPSILON);
		EXPECT_NEAR(0.3f, TurnYaw(0.3f, 1, 0, 1), EPSILON);
		EXPECT_NEAR(0.3f, TurnYaw(0.3f, 1, -2, 1), EPSILON);
		EXPECT_NEAR(0.3f, TurnYaw(0.3f, 1, 2, -1), EPSILON);
	}

	TEST(MathUtilTest, Saturate_ClampsOutOfRange)
	{
		EXPECT_NEAR(0.0f, Saturate(-3.0f), EPSILON);
		EXPECT_NEAR(1.0f, Saturate(5.0f), EPSILON);
	}

	// SmoothStep01：両端と真ん中の値、端の近くでは変化がゆっくり
	TEST(MathUtilTest, SmoothStep01_EndsAndMiddle)
	{
		EXPECT_NEAR(0.0f, SmoothStep01(0.0f), EPSILON);
		EXPECT_NEAR(0.5f, SmoothStep01(0.5f), EPSILON);
		EXPECT_NEAR(1.0f, SmoothStep01(1.0f), EPSILON);
	}

	// Lerp：t = 0 で a、t = 1 で b、t = 0.5 で真ん中
	TEST(MathUtilTest, Lerp_EndsAndMiddle)
	{
		EXPECT_NEAR(2.0f, Lerp(2.0f, 6.0f, 0.0f), EPSILON);
		EXPECT_NEAR(6.0f, Lerp(2.0f, 6.0f, 1.0f), EPSILON);
		EXPECT_NEAR(4.0f, Lerp(2.0f, 6.0f, 0.5f), EPSILON);
	}
}
