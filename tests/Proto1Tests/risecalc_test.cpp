// physics/risecalc.h（腕で場外から戻る動き）の単体テスト
#include <math.h>
#include <gtest/gtest.h>
#include "physics/risecalc.h"

namespace Proto1Tests
{
	static constexpr float RISE_EPSILON = 1.0e-3f;	// float の比較で許す誤差
	static constexpr float RISE_DT = 1.0f / 60.0f;	// 1フレームの秒数（ゲームと同じ）

	// ゲームと同じ形の浮島（30 × 30、上の面 y = 0）と、高台1つ（中心 (-8, 8)、6 × 6、上の面 y = 4）
	static STAGE MakeRiseStage(void)
	{
		STAGE stage{};
		stage.HalfWidth = 15.0f;
		stage.HalfDepth = 15.0f;
		stage.TopY = 0.0f;
		stage.Thickness = 2.0f;
		stage.OutY = -15.0f;
		stage.Platforms[0] = { -8.0f, 8.0f, 3.0f, 3.0f, 4.0f, 0.6f };
		stage.PlatformCount = 1;
		return stage;
	}

	// 浮島の横の下に落ちているときは、浮島のふちから少し内側の点が返る
	TEST(RiseCalcTest, NearestLanding_PicksIslandEdgeFromOutside)
	{
		VEC3 landing{};
		EXPECT_TRUE(RiseCalcNearestLanding(MakeRiseStage(), { 18.0f, -3.0f, 2.0f }, 1.0f, 25.0f, landing));
		EXPECT_NEAR(14.0f, landing.X, RISE_EPSILON);
		EXPECT_NEAR(0.0f, landing.Y, RISE_EPSILON);
		EXPECT_NEAR(2.0f, landing.Z, RISE_EPSILON);
	}

	// 高台のすぐ近くの上にいるときは高台が返り、遠すぎるときは見つからない
	TEST(RiseCalcTest, NearestLanding_PicksPlatformWhenCloser)
	{
		VEC3 landing{};
		EXPECT_TRUE(RiseCalcNearestLanding(MakeRiseStage(), { -8.0f, 5.0f, 12.0f }, 1.0f, 25.0f, landing));
		EXPECT_NEAR(4.0f, landing.Y, RISE_EPSILON);
		EXPECT_NEAR(10.0f, landing.Z, RISE_EPSILON);
		EXPECT_FALSE(RiseCalcNearestLanding(MakeRiseStage(), { 60.0f, -3.0f, 0.0f }, 1.0f, 25.0f, landing));
	}

	// 斜め上への突進は、決めた秒数で決めた距離と高さだけ進んで終わる
	TEST(RiseCalcTest, Rise_SlopeReachesDistanceAndHeight)
	{
		const float pitch = 3.14159265f / 6.0f; // 30°
		const VEC3 offset = RiseCalcSlopeOffset({ 0.0f, 0.0f, 2.0f }, pitch, 10.0f);
		RiseCalc calc;
		calc.Start({ 1.0f, -2.0f, 3.0f }, offset, 0.35f, false);
		VEC3 position{};
		for (int i = 0; i < 12; i++) position = calc.Step(RISE_DT);
		EXPECT_TRUE(calc.IsActive()); // 0.2 秒：まだ途中
		EXPECT_NEAR(-2.0f + 5.0f * (0.2f / 0.35f), position.Y, 0.05f);
		for (int i = 0; i < 12; i++) position = calc.Step(RISE_DT);
		EXPECT_FALSE(calc.IsActive());
		EXPECT_NEAR(1.0f, position.X, RISE_EPSILON);
		EXPECT_NEAR(-2.0f + 5.0f, position.Y, RISE_EPSILON);
		EXPECT_NEAR(3.0f + 10.0f * cosf(pitch), position.Z, RISE_EPSILON);
	}

	// 上への跳ねは、はじめが速く、決めた秒数で決めた高さに着く
	TEST(RiseCalcTest, Rise_HopSlowsDownAndReachesHeight)
	{
		RiseCalc calc;
		calc.Start({ 0.0f, 0.0f, 0.0f }, { 0.0f, 4.0f, 0.0f }, 0.3f, true);
		VEC3 position{};
		for (int i = 0; i < 9; i++) position = calc.Step(RISE_DT);
		EXPECT_TRUE(position.Y > 2.5f); // 半分の時間で、半分より高い
		for (int i = 0; i < 12; i++) position = calc.Step(RISE_DT);
		EXPECT_FALSE(calc.IsActive());
		EXPECT_NEAR(4.0f, position.Y, RISE_EPSILON);
	}
}
