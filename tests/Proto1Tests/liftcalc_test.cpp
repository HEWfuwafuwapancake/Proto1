// Ｇの受付・着地点の予約・27更新の軌道（設計書 第14節の受入例。DirectX なし）
#include <gtest/gtest.h>
#include "physics/liftcalc.h"
#include "physics/mapcalc.h"

namespace Proto1Tests
{
	static STAGE BuildMap(STAGE_MAP map)
	{
		STAGE stage{};
		MapCalcBuild(map, 1.0f, stage);
		return stage;
	}

	// 郵便局の北西Ｇ：n=9 で始点の真上、n=24 で着地点の真上、n=27 で着地。
	TEST(LiftCalcTest, PostOffice_TrajectoryPassesReferencePoints)
	{
		const STAGE stage = BuildMap(STAGE_MAP_POST_OFFICE);
		LiftCalc lift;
		lift.Reset();
		LIFT_FIGHTER fighters[FIGHTER_COUNT] = {};
		fighters[0] = { { -12.0f, 0.0f, 10.0f }, true, true, true };
		ASSERT_EQ(1, lift.Accept(stage, 0, fighters));
		for (int n = 1; n <= 9; ++n) lift.Step(stage, fighters);
		EXPECT_NEAR(-12.0f, lift.GetFeet(0).X, 0.01f);
		EXPECT_NEAR(6.6f, lift.GetFeet(0).Y, 0.01f);
		for (int n = 10; n <= 24; ++n) lift.Step(stage, fighters);
		EXPECT_NEAR(-16.0f, lift.GetFeet(0).X, 0.01f);
		EXPECT_NEAR(5.0f, lift.GetFeet(0).Z, 0.01f);
		for (int n = 25; n <= 27; ++n) lift.Step(stage, fighters);
		EXPECT_TRUE(lift.DidLand(0));
		EXPECT_NEAR(6.0f, lift.GetFeet(0).Y, 0.01f);
	}

	// 始点から2までは受け付け、2.01は受け付けない。再使用は72更新後から。
	TEST(LiftCalcTest, AcceptRangeAndCooldown)
	{
		const STAGE stage = BuildMap(STAGE_MAP_POST_OFFICE);
		LiftCalc lift;
		lift.Reset();
		LIFT_FIGHTER fighters[FIGHTER_COUNT] = {};
		fighters[0] = { { -12.0f, 0.0f, 12.01f }, true, true, true };
		EXPECT_EQ(0, lift.Accept(stage, 0, fighters));
		fighters[0].Feet.Z = 12.0f;
		EXPECT_EQ(1, lift.Accept(stage, 10, fighters));
		lift.Cancel(0);
		EXPECT_EQ(0, lift.Accept(stage, 81, fighters));
		EXPECT_EQ(1, lift.Accept(stage, 82, fighters));
	}

	// 市場の同じＧを2人が同時に押すと、P1 は名目点、P2 は横へずれた点を予約する。
	TEST(LiftCalcTest, Market_SimultaneousRequestsReserveInSlotOrder)
	{
		const STAGE stage = BuildMap(STAGE_MAP_MARKET);
		LiftCalc lift;
		lift.Reset();
		LIFT_FIGHTER fighters[FIGHTER_COUNT] = {};
		fighters[0] = { { -8.0f, 0.0f, 7.0f }, true, true, true };
		fighters[1] = { { -8.0f, 0.0f, 8.0f }, true, true, true };
		ASSERT_EQ(3, lift.Accept(stage, 0, fighters));
		EXPECT_NEAR(-3.0f, lift.GetTarget(0).X, 0.001f);
		EXPECT_NEAR(-1.95f, lift.GetTarget(1).X, 0.001f);
		EXPECT_NEAR(12.0f, lift.GetTarget(1).Z, 0.001f);
	}
}
