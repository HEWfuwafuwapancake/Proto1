// 対戦マップの配置台帳と地形の判定（設計書 第13節の受入例。DirectX なし）
#include <gtest/gtest.h>
#include "physics/mapcalc.h"

namespace Proto1Tests
{
	static STAGE Build(STAGE_MAP map, float scale = 1.0f)
	{
		STAGE stage{};
		EXPECT_TRUE(MapCalcBuild(map, scale, stage));
		return stage;
	}

	// その場所の床の高さが期待どおりか（床がないなら found が false）。
	static void ExpectHeight(const STAGE& stage, float x, float z, float expected)
	{
		float height = 0.0f;
		ASSERT_TRUE(StageCalcGroundHeight(stage, x, z, height)) << x << ", " << z;
		EXPECT_NEAR(expected, height, 0.001f) << x << ", " << z;
	}

	static void ExpectNoFloor(const STAGE& stage, float x, float z)
	{
		float height = 0.0f;
		EXPECT_FALSE(StageCalcGroundHeight(stage, x, z, height)) << x << ", " << z;
	}

	// 郵便局：四隅の切り落とし・仕分け台・坂の高さ。
	TEST(MapCalcTest, PostOffice_HeightsFollowLedger)
	{
		const STAGE stage = Build(STAGE_MAP_POST_OFFICE);
		ExpectHeight(stage, 23.0f, 23.0f, 0.0f);
		ExpectNoFloor(stage, 23.01f, 23.0f);
		ExpectHeight(stage, 26.0f, 20.0f, 0.0f);
		ExpectNoFloor(stage, 26.01f, 20.0f);
		ExpectHeight(stage, 7.0f, 7.0f, 1.2f);
		ExpectHeight(stage, 7.01f, 7.0f, 0.0f);
		ExpectHeight(stage, 0.0f, 8.0f, 1.2f);
		ExpectHeight(stage, -20.0f, 17.0f, 0.0f);
		ExpectHeight(stage, -20.0f, 12.0f, 3.0f);
		ExpectHeight(stage, -20.0f, 7.0f, 6.0f);
		ExpectHeight(stage, 20.0f, -12.0f, 3.0f);
	}

	// 市場：低い橋と地割れ・高い橋・搬入路と高架の坂・基礎・外周。
	TEST(MapCalcTest, Market_HeightsFollowLedger)
	{
		const STAGE stage = Build(STAGE_MAP_MARKET);
		ExpectHeight(stage, 0.0f, 0.0f, -3.0f);
		ExpectHeight(stage, 0.0f, 4.0f, -3.0f);
		ExpectNoFloor(stage, 0.0f, 4.01f);
		ExpectNoFloor(stage, 0.0f, 9.99f);
		ExpectHeight(stage, 0.0f, 10.0f, 3.0f);
		ExpectHeight(stage, 0.0f, 14.0f, 3.0f);
		ExpectNoFloor(stage, 0.0f, 14.01f);
		ExpectHeight(stage, 4.0f, 7.0f, 0.0f);
		ExpectNoFloor(stage, 3.99f, 7.0f);
		ExpectHeight(stage, 10.0f, 0.0f, -3.0f);
		ExpectHeight(stage, 13.0f, 0.0f, -1.5f);
		ExpectHeight(stage, 16.0f, 0.0f, 0.0f);
		ExpectHeight(stage, 4.0f, 12.0f, 3.0f);
		ExpectHeight(stage, 8.0f, 12.0f, 1.5f);
		ExpectHeight(stage, 12.0f, 12.0f, 0.0f);
		ExpectHeight(stage, -24.0f, 7.0f, 1.2f);
		ExpectHeight(stage, 24.0f, -7.0f, 1.2f);
		ExpectHeight(stage, 24.0f, 7.0f, 0.0f);
		ExpectHeight(stage, 30.0f, 24.0f, 0.0f);
		ExpectNoFloor(stage, 30.01f, 24.0f);
	}

	// 地割れの上の橋は、下から上昇して通り抜け、落ちてくると上面に着地できる。
	TEST(MapCalcTest, Market_BridgePassesFromBelowAndLandsFromAbove)
	{
		const STAGE stage = Build(STAGE_MAP_MARKET);
		float landing = 0.0f;
		EXPECT_FALSE(StageCalcFindLanding(stage, 0.0f, -12.0f, 2.0f, 3.5f, landing)); // 上昇中は通り抜ける
		ASSERT_TRUE(StageCalcFindLanding(stage, 0.0f, -12.0f, 3.5f, 2.9f, landing));
		EXPECT_FLOAT_EQ(3.0f, landing);
		// 橋の下を横に動いても、片面の橋は壁にならない。
		float x = 0.0f, z = 0.0f;
		EXPECT_EQ(0, StageCalcMoveBody(stage, 0.0f, 0.0f, -11.0f, 0.0f, -2.0f, false, x, z));
	}

	// 側面：仕分け台（高さ1.2）へは歩いて入れず、坂は高さに沿って上れる。地割れに落ちた体は岸の側面で止まる。
	TEST(MapCalcTest, MoveBody_StopsAtStepsAndFollowsSlopes)
	{
		const STAGE post = Build(STAGE_MAP_POST_OFFICE);
		float x = 0.0f, z = 0.0f;
		EXPECT_EQ(STAGE_STOP_X | STAGE_STOP_Z, StageCalcMoveBody(post, 9.0f, 0.0f, 0.0f, -0.6f, 0.0f, true, x, z));
		EXPECT_FLOAT_EQ(9.0f, x);
		// 斜めに当たったら、止まらない軸の分だけ壁に沿って動く。
		EXPECT_EQ(STAGE_STOP_X, StageCalcMoveBody(post, 9.0f, 0.0f, 0.0f, -0.6f, 0.3f, true, x, z));
		EXPECT_FLOAT_EQ(9.0f, x);
		EXPECT_FLOAT_EQ(0.3f, z);
		// 西の坂の下の端から上って、坂の高さに合わせる。
		EXPECT_EQ(0, StageCalcMoveBody(post, -20.0f, 0.0f, 17.4f, 0.0f, -0.4f, true, x, z));
		float ground = 0.0f;
		ASSERT_TRUE(StageCalcFollowGround(post, x, 0.0f, z, ground));
		EXPECT_NEAR(0.0f, ground, 0.001f);
		ASSERT_TRUE(StageCalcFollowGround(post, -20.0f, 2.8f, 12.3f, ground));
		EXPECT_NEAR(2.82f, ground, 0.001f);
		// 仕分け台のふちから降りると落ちる。
		EXPECT_FALSE(StageCalcFollowGround(post, 9.0f, 1.2f, 0.0f, ground));

		const STAGE market = Build(STAGE_MAP_MARKET);
		EXPECT_EQ(STAGE_STOP_X | STAGE_STOP_Z, StageCalcMoveBody(market, 3.0f, -4.0f, 7.0f, 0.6f, 0.0f, false, x, z));
	}

	// 倍率：台帳は x = 世界X ÷ s で引く。高さはそのまま。
	TEST(MapCalcTest, Scale_LooksUpLedgerByWorldDividedByScale)
	{
		const STAGE stage = Build(STAGE_MAP_MARKET, 1.5f);
		ExpectHeight(stage, 12.0f, 18.0f, 1.5f);
	}

	// 最低・最高の倍率でも、P・Ｇの始点と着地点・補給の地点が、意図した高さの床に載る。
	TEST(MapCalcTest, PointsStandOnIntendedFloorsAtEveryScale)
	{
		const STAGE_MAP maps[] = { STAGE_MAP_POST_OFFICE, STAGE_MAP_MARKET };
		const float scales[] = { 0.7f, 1.0f, 1.5f };
		for (STAGE_MAP map : maps) {
			for (float scale : scales) {
				const STAGE stage = Build(map, scale);
				ASSERT_EQ(4, stage.SpawnCount);
				ASSERT_EQ(4, stage.LiftCount);
				ASSERT_EQ(16, stage.SupplyCount);
				for (int i = 0; i < stage.SpawnCount; ++i) ExpectHeight(stage, stage.Spawns[i].X, stage.Spawns[i].Z, 0.0f);
				for (int i = 0; i < stage.LiftCount; ++i) {
					ExpectHeight(stage, stage.Lifts[i].StartX, stage.Lifts[i].StartZ, 0.0f);
					ExpectHeight(stage, stage.Lifts[i].Landing.X, stage.Lifts[i].Landing.Z, stage.Lifts[i].Landing.Y);
				}
				for (int i = 0; i < stage.SupplyCount; ++i) ExpectHeight(stage, stage.Supplies[i].X, stage.Supplies[i].Z, stage.Supplies[i].Y);
			}
		}
	}

	// 空き缶：2人が同じ更新で乗っても1回。鳴ってから120更新は鳴らず、乗り続けても鳴らない。
	TEST(MapCalcTest, CanBell_RingsOncePerEntryWithInterval)
	{
		CanBellCalc cans;
		cans.Reset();
		bool on[4] = { true, true, false, false };
		EXPECT_TRUE(cans.Step(100, on, 4));
		EXPECT_FALSE(cans.Step(101, on, 4)); // 居続けるだけでは鳴らない
		EXPECT_NE(0.0f, cans.GetTilt(103));
		EXPECT_EQ(0.0f, cans.GetTilt(160));
		bool off[4] = {};
		bool third[4] = { false, false, true, false };
		cans.Step(218, off, 4);
		EXPECT_FALSE(cans.Step(219, third, 4));
		cans.Step(219, off, 4);
		EXPECT_TRUE(cans.Step(220, third, 4));
	}
}
