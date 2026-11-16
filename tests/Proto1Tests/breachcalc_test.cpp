// physics/breachcalc.h（刀の長押しで飛ぶ先の着地点）の単体テスト
#include <gtest/gtest.h>
#include <math.h>
#include "physics/breachcalc.h"

namespace
{
	STAGE MakeStage()
	{
		STAGE stage{};
		stage.HalfWidth = 15.0f;
		stage.HalfDepth = 15.0f;
		stage.TopY = 0.0f;
		stage.Thickness = 2.0f;
		stage.OutY = -15.0f;
		stage.Platforms[0] = { 0.0f, 8.0f, 3.0f, 3.0f, 6.0f, 0.6f };
		stage.PlatformCount = 1;
		return stage;
	}

	VEC3 Normalized(const VEC3& v)
	{
		return Vec3Scale(v, 1.0f / Vec3Length(v));
	}

	static constexpr VEC3 HEAD{ 0.0f, 1.5f, 0.0f };
	static constexpr float FEET_Y = 0.0f;
	static constexpr float RANGE = 25.0f;
	static constexpr float INSET = 1.0f;

	// 見下ろすと、線が地面に当たる所へ着地する
	TEST(BreachCalcTest, LandsWhereViewHitsGround)
	{
		VEC3 landing{};
		ASSERT_TRUE(BreachCalcFindLanding(MakeStage(), HEAD, Normalized({ 0.0f, -1.5f, 6.0f }), RANGE, FEET_Y, INSET, landing));
		EXPECT_NEAR(6.0f, landing.Z, 0.001f);
		EXPECT_NEAR(0.0f, landing.Y, 0.001f);
	}

	// 下から高台の天井を見上げると、通り抜けずに天井の手前の下（浮島）へ着地する
	TEST(BreachCalcTest, LandsBelowCeilingInsteadOfPassingThrough)
	{
		VEC3 landing{};
		// 線は z = 6.93 で高台の下の面（高さ 5.4）に当たる。
		ASSERT_TRUE(BreachCalcFindLanding(MakeStage(), HEAD, Normalized({ 0.0f, 4.5f, 8.0f }), RANGE, FEET_Y, INSET, landing));
		EXPECT_NEAR(0.0f, landing.Y, 0.001f);
		EXPECT_GT(landing.Z, 5.0f);
		EXPECT_LT(landing.Z, 6.93f);
	}

	// 場外で浮島の壁のそばからはるか上を見ると、いちばん近い見えている崖のふちの少し内側へ着地する
	TEST(BreachCalcTest, LooksFarUpBesideWallPicksNearestVisibleEdge)
	{
		VEC3 landing{};
		ASSERT_TRUE(BreachCalcFindLanding(MakeStage(), { 17.0f, -3.0f, 0.0f }, Normalized({ -0.2f, 1.0f, 0.0f }),
			RANGE, -4.2f, INSET, landing));
		EXPECT_NEAR(14.0f, landing.X, 0.001f);
		EXPECT_NEAR(0.0f, landing.Y, 0.001f);
		EXPECT_NEAR(0.0f, landing.Z, 0.001f);
	}

	// 面に当たらず空へ向けたときは、線の先の真下の足場へ下ろす
	TEST(BreachCalcTest, DropsToGroundWhenViewHitsNothing)
	{
		VEC3 landing{};
		ASSERT_TRUE(BreachCalcFindLanding(MakeStage(), HEAD, { 1.0f, 0.0f, 0.0f }, 10.0f, FEET_Y, INSET, landing));
		EXPECT_NEAR(10.0f, landing.X, 0.001f);
		EXPECT_NEAR(0.0f, landing.Y, 0.001f);
	}

	// 浮島の外を向いていたら、手前へ戻って浮島のふち近くへ着地する
	TEST(BreachCalcTest, BacktracksToLastGroundBeforeEdge)
	{
		VEC3 landing{};
		ASSERT_TRUE(BreachCalcFindLanding(MakeStage(), HEAD, { 1.0f, 0.0f, 0.0f }, RANGE, FEET_Y, INSET, landing));
		EXPECT_LE(landing.X, 15.0f);
		EXPECT_GT(landing.X, 14.0f);
	}

	// 足場が一つもない方向なら着地点なし
	TEST(BreachCalcTest, ReturnsFalseOverVoid)
	{
		VEC3 landing{};
		EXPECT_FALSE(BreachCalcFindLanding(MakeStage(), { 40.0f, 1.5f, 0.0f }, { 1.0f, 0.0f, 0.0f }, RANGE, FEET_Y, INSET, landing));
	}
}
