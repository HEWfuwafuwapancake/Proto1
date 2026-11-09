// physics/c4calc.h（C4 の投げた弾・落ちる弾・設置済み）の単体テスト
#include <gtest/gtest.h>
#include "physics/c4calc.h"

namespace Proto1Tests
{
	static const STAGE C4_STAGE{ 0.0f, 0.0f, 15.0f, 15.0f, 0.0f, 2.0f, -15.0f };
	static const PROJECTILE_PARAMS C4_PARAMS{ 0.2f, 20.0f, 5.0f, 0.0f, false };
	static const float C4_DT = 1.0f / 60.0f;

	static void StepUntilPlaced(C4Calc& bombs, int placed, TENTACLE_TARGET* targets = nullptr, int count = 0)
	{
		for (int i = 0; i < 300 && bombs.GetPlacedCount() != placed; ++i) {
			bombs.Step(C4_DT, targets, count, C4_STAGE, C4_PARAMS);
		}
	}

	// 真上から落として、(x, 0) に設置する。
	static void DropAt(C4Calc& bombs, float x, int placedAfter)
	{
		bombs.Throw({ x, 1.0f, 0.0f }, {}, C4_PARAMS);
		StepUntilPlaced(bombs, placedAfter);
	}

	// 投げた C4 は足場に着いたら、その場所に設置される
	TEST(C4CalcTest, Thrown_PlacedWhereItLands)
	{
		C4Calc bombs;
		bombs.Throw({ 0.0f, 1.0f, 0.0f }, { 0.0f, 4.0f, 5.0f }, C4_PARAMS);
		StepUntilPlaced(bombs, 1);
		ASSERT_EQ(1, bombs.GetPlacedCount());
		EXPECT_NEAR(C4_STAGE.TopY, bombs.GetPlaced(0).Y, 0.001f);
		EXPECT_GT(bombs.GetPlaced(0).Z, 1.0f);
		EXPECT_EQ(0, bombs.GetThrown().GetCount());
	}

	// 4個目を置くと、いちばん古いものが消える
	TEST(C4CalcTest, FourthReplacesOldest)
	{
		C4Calc bombs;
		DropAt(bombs, 1.0f, 1);
		DropAt(bombs, 2.0f, 2);
		DropAt(bombs, 3.0f, 3);
		bombs.Throw({ 4.0f, 1.0f, 0.0f }, {}, C4_PARAMS);
		for (int i = 0; i < 300 && bombs.GetThrown().GetCount() > 0; ++i) {
			bombs.Step(C4_DT, nullptr, 0, C4_STAGE, C4_PARAMS);
		}
		ASSERT_EQ(C4_MAX, bombs.GetPlacedCount());
		EXPECT_NEAR(2.0f, bombs.GetPlaced(0).X, 0.001f);
		EXPECT_NEAR(3.0f, bombs.GetPlaced(1).X, 0.001f);
		EXPECT_NEAR(4.0f, bombs.GetPlaced(2).X, 0.001f);
	}

	// 空中の敵に当たると、飛んでいた向きへは進まず、その場に落ちて設置される
	TEST(C4CalcTest, HittingAirborneEnemy_DropsStraightDown)
	{
		C4Calc bombs;
		TENTACLE_TARGET enemy{ { { 0.0f, 3.0f, 4.0f }, { 0.0f, 4.0f, 4.0f }, 0.5f }, true };
		const PROJECTILE_PARAMS straight{ 0.2f, 0.0f, 5.0f, 0.0f, false };
		bombs.Throw({ 0.0f, 3.5f, 0.0f }, { 0.0f, 0.0f, 12.0f }, straight);
		for (int i = 0; i < 120 && bombs.GetFalling().GetCount() == 0; ++i) {
			bombs.Step(C4_DT, &enemy, 1, C4_STAGE, straight);
		}
		EXPECT_EQ(1, bombs.GetFalling().GetCount());
		EXPECT_EQ(0, bombs.GetThrown().GetCount());
		StepUntilPlaced(bombs, 1, &enemy, 1);
		ASSERT_EQ(1, bombs.GetPlacedCount());
		EXPECT_NEAR(C4_STAGE.TopY, bombs.GetPlaced(0).Y, 0.001f);
		EXPECT_NEAR(3.4f, bombs.GetPlaced(0).Z, 0.3f);
	}

	// 起爆すると設置済みの位置を返して空になる
	TEST(C4CalcTest, Detonate_ReturnsPlacedAndClears)
	{
		C4Calc bombs;
		DropAt(bombs, 1.0f, 1);
		DropAt(bombs, 2.0f, 2);
		VEC3 positions[C4_MAX];
		ASSERT_EQ(2, bombs.Detonate(positions));
		EXPECT_NEAR(1.0f, positions[0].X, 0.001f);
		EXPECT_NEAR(2.0f, positions[1].X, 0.001f);
		EXPECT_EQ(0, bombs.GetPlacedCount());
	}
}
