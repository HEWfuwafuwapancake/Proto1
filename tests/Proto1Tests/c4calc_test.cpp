// physics/c4calc.h（着弾した瞬間に爆発する C4）の単体テスト
#include <gtest/gtest.h>
#include "physics/c4calc.h"

namespace Proto1Tests
{
	static const STAGE C4_STAGE{ 0.0f, 0.0f, 15.0f, 15.0f, 0.0f, 2.0f, -15.0f };
	static const PROJECTILE_PARAMS C4_PARAMS{ 0.2f, 20.0f, 5.0f, 0.0f, false };
	static constexpr float C4_DT = 1.0f / 60.0f;

	// 足場に着いたフレームだけ場所を返し、弾を消す。
	TEST(C4CalcTest, StageImpactReturnsPositionAndRemovesBomb)
	{
		C4Calc bombs;
		ASSERT_TRUE(bombs.Throw({ 2.0f, 1.0f, 0.0f }, {}, C4_PARAMS, false));
		C4_IMPACT impacts[PROJECTILE_MAX];
		int count = 0;
		for (int i = 0; i < 120 && count == 0; ++i) count = bombs.Step(C4_DT, nullptr, 0, C4_STAGE, C4_PARAMS, impacts, PROJECTILE_MAX);
		ASSERT_EQ(1, count);
		EXPECT_NEAR(2.0f, impacts[0].Position.X, 0.001f);
		EXPECT_NEAR(0.0f, impacts[0].Position.Y, 0.001f);
		EXPECT_EQ(0, bombs.GetThrown().GetCount());
		EXPECT_EQ(0, bombs.Step(C4_DT, nullptr, 0, C4_STAGE, C4_PARAMS, impacts, PROJECTILE_MAX));
	}

	// 空中の相手に当たったら、その空中の位置ですぐ着弾を返す（地面まで落とさない）。
	TEST(C4CalcTest, FighterImpactExplodesInAir)
	{
		C4Calc bombs;
		const TENTACLE_TARGET enemy{ { { 0.0f, 3.0f, 4.0f }, { 0.0f, 4.0f, 4.0f }, 0.5f }, true };
		const PROJECTILE_PARAMS straight{ 0.2f, 0.0f, 5.0f, 0.0f, false };
		ASSERT_TRUE(bombs.Throw({ 0.0f, 3.5f, 0.0f }, { 0.0f, 0.0f, 12.0f }, straight, false));
		C4_IMPACT impacts[PROJECTILE_MAX];
		int count = 0;
		for (int i = 0; i < 120 && count == 0; ++i) count = bombs.Step(C4_DT, &enemy, 1, C4_STAGE, straight, impacts, PROJECTILE_MAX);
		ASSERT_EQ(1, count);
		EXPECT_NEAR(3.5f, impacts[0].Position.Y, 0.001f);
		EXPECT_NEAR(3.4f, impacts[0].Position.Z, 0.3f);
		EXPECT_EQ(0, bombs.GetThrown().GetCount());
	}

	// 浮島の外へ落ちた弾は爆発を返さず消える。
	TEST(C4CalcTest, FallingOutDisappearsWithoutExplosion)
	{
		C4Calc bombs;
		ASSERT_TRUE(bombs.Throw({ 30.0f, 1.0f, 0.0f }, {}, C4_PARAMS, false));
		C4_IMPACT impacts[PROJECTILE_MAX];
		int count = 0;
		for (int i = 0; i < 300 && bombs.GetThrown().GetCount() > 0; ++i) count += bombs.Step(C4_DT, nullptr, 0, C4_STAGE, C4_PARAMS, impacts, PROJECTILE_MAX);
		EXPECT_EQ(0, bombs.GetThrown().GetCount());
		EXPECT_EQ(0, count);
	}

	// 同時に飛んでいる通常と大型の弾は、着弾時にも大きさを区別できる。
	TEST(C4CalcTest, SimultaneousBombsKeepTheirSizes)
	{
		C4Calc bombs;
		ASSERT_TRUE(bombs.Throw({ -2.0f, 1.0f, 0.0f }, {}, C4_PARAMS, false));
		ASSERT_TRUE(bombs.Throw({ 2.0f, 1.0f, 0.0f }, {}, C4_PARAMS, true));
		C4_IMPACT impacts[PROJECTILE_MAX];
		int count = 0;
		for (int i = 0; i < 120 && count == 0; ++i) count = bombs.Step(C4_DT, nullptr, 0, C4_STAGE, C4_PARAMS, impacts, PROJECTILE_MAX);
		ASSERT_EQ(2, count);
		EXPECT_FALSE(impacts[0].Large);
		EXPECT_TRUE(impacts[1].Large);
		EXPECT_NEAR(-2.0f, impacts[0].Position.X, 0.001f);
		EXPECT_NEAR(2.0f, impacts[1].Position.X, 0.001f);
	}
}
