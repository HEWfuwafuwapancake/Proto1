#include <gtest/gtest.h>
#include "physics/combocalc.h"

namespace Proto1Tests
{
	// 受付の間に押すと 1段目 → 2段目 → 3段目 と進み、3段目のあとは1段目に戻る。
	TEST(ComboCalcTest, Start_AdvancesWithinWindowAndWrapsAfterLast)
	{
		ComboCalc combo;
		EXPECT_EQ(0, combo.Start(3));
		combo.Open(0.3f);
		combo.Step(0.1f);
		EXPECT_EQ(1, combo.Start(3));
		combo.Open(0.3f);
		EXPECT_EQ(2, combo.Start(3));
		combo.Open(0.3f);
		EXPECT_EQ(0, combo.Start(3));
	}

	// 受付が閉じてから押すと、途中の段からでも1段目に戻る。
	TEST(ComboCalcTest, Start_RestartsAfterWindowCloses)
	{
		ComboCalc combo;
		combo.Start(3);
		combo.Open(0.3f);
		combo.Start(3);
		combo.Open(0.3f);
		combo.Step(0.35f);
		EXPECT_FALSE(combo.IsOpen());
		EXPECT_EQ(-1, combo.GetStep());
		EXPECT_EQ(0, combo.Start(3));
	}

	// 2段のハンマーは、受付の間に押し続けると 1段目と2段目をくり返す。
	TEST(ComboCalcTest, Start_LoopsTwoSteps)
	{
		ComboCalc combo;
		int steps[5];
		for (int& step : steps) {
			step = combo.Start(2);
			combo.Open(0.3f);
		}
		EXPECT_EQ(0, steps[0]);
		EXPECT_EQ(1, steps[1]);
		EXPECT_EQ(0, steps[2]);
		EXPECT_EQ(1, steps[3]);
		EXPECT_EQ(0, steps[4]);
	}

	static STAGE MakeStage(void)
	{
		STAGE stage{};
		stage.HalfWidth = 10.0f;
		stage.HalfDepth = 10.0f;
		return stage;
	}

	// 浮島の上からは、ふちの内側で止まる。中ほどなら全部進む。
	TEST(ComboCalcTest, PierceEnd_StopsInsidePlatformEdge)
	{
		const STAGE stage = MakeStage();
		const VEC3 inside = ComboCalcPierceEnd(stage, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 2.0f }, 4.0f, 0.5f);
		EXPECT_NEAR(4.0f, inside.Z, 0.0001f);
		EXPECT_NEAR(1.0f, inside.Y, 0.0001f);
		const VEC3 edge = ComboCalcPierceEnd(stage, { 8.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f }, 4.0f, 0.5f);
		EXPECT_NEAR(9.5f, edge.X, 0.0001f);
	}

	// 浮島の外（空中）からは縮めずに進む。
	TEST(ComboCalcTest, PierceEnd_DoesNotClampOffStage)
	{
		const STAGE stage = MakeStage();
		const VEC3 end = ComboCalcPierceEnd(stage, { 12.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f }, 4.0f, 0.5f);
		EXPECT_NEAR(16.0f, end.X, 0.0001f);
	}
}
