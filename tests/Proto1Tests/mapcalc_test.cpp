// 対戦マップの配置台帳と地形の判定（設計書 第13節の受入例。DirectX なし）
#include <gtest/gtest.h>
#include <math.h>
#include "physics/mapcalc.h"
#include "physics/playercalc.h"

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

	// 押し合い・触手で引っぱられるなどで仕分け台の中（上面より下）に入った体は、台をすり抜けて落ちず、台の上面へ押し上げる。
	TEST(MapCalcTest, BodyInsideCenterTable_IsPushedUpInsteadOfFallingThrough)
	{
		const STAGE post = Build(STAGE_MAP_POST_OFFICE);
		PLAYER_PARAMS params{ 8.5f, 12.0f, 30.0f, 30.0f, 1 };
		// 立っている体（床の高さ 0）が、台の中へ押し込まれた。
		PlayerBodyCalc standing;
		standing.SetPosition({ 7.5f, 0.0f, 0.0f });
		standing.SetOnGround(true);
		standing.Step(0.0f, 0.0f, false, 1.0f / 60.0f, params, post);
		EXPECT_TRUE(standing.GetOnGround());
		EXPECT_NEAR(1.2f, standing.GetY(), 0.001f);
		// 空中で台の中へ引っぱられた体も、落ちるときに台の上面に着地する。
		PlayerBodyCalc falling;
		falling.SetPosition({ 6.0f, 0.5f, 0.0f });
		for (int i = 0; i < 30; ++i) falling.Step(0.0f, 0.0f, false, 1.0f / 60.0f, params, post);
		EXPECT_TRUE(falling.GetOnGround());
		EXPECT_NEAR(1.2f, falling.GetY(), 0.001f);
		EXPECT_FALSE(StageCalcIsOut(post, falling.GetY()));
	}

	// 倍率：台帳は x = 世界X ÷ s で引く。高さはそのまま。
	TEST(MapCalcTest, Scale_LooksUpLedgerByWorldDividedByScale)
	{
		const STAGE stage = Build(STAGE_MAP_MARKET, 1.5f);
		ExpectHeight(stage, 12.0f, 18.0f, 1.5f);
	}

	// 最低・最高の倍率でも、P・Ｇの始点と着地点が、意図した高さの床に載る。
	TEST(MapCalcTest, PointsStandOnIntendedFloorsAtEveryScale)
	{
		const STAGE_MAP maps[] = { STAGE_MAP_POST_OFFICE, STAGE_MAP_MARKET };
		const float scales[] = { 0.7f, 1.0f, 1.5f };
		for (STAGE_MAP map : maps) {
			for (float scale : scales) {
				const STAGE stage = Build(map, scale);
				ASSERT_EQ(4, stage.SpawnCount);
				ASSERT_EQ(4, stage.LiftCount);
				for (int i = 0; i < stage.SpawnCount; ++i) ExpectHeight(stage, stage.Spawns[i].X, stage.Spawns[i].Z, 0.0f);
				for (int i = 0; i < stage.LiftCount; ++i) {
					ExpectHeight(stage, stage.Lifts[i].StartX, stage.Lifts[i].StartZ, 0.0f);
					ExpectHeight(stage, stage.Lifts[i].Landing.X, stage.Lifts[i].Landing.Z, stage.Lifts[i].Landing.Y);
				}
			}
		}
	}

	// 市場の西岸の路面から、荷道と坂（x −16〜−4、|z| ≤ 4）を除くと、上下の2枚と左の1枚が残り、面積が合う。
	TEST(MapCalcTest, SubtractRects_LeavesShoreAroundRoadCut)
	{
		const float holes[2][4] = { { -10.0f, -4.0f, -4.0f, 4.0f }, { -16.0f, -10.0f, -4.0f, 4.0f } };
		float pieces[16][4];
		const int count = MapCalcSubtractRects(-30.0f, -4.0f, -24.0f, 24.0f, holes, 2, pieces, 16);
		float area = 0.0f;
		for (int i = 0; i < count; ++i) {
			area += (pieces[i][1] - pieces[i][0]) * (pieces[i][3] - pieces[i][2]);
			// 残りの四角は、荷道と坂に重ならない。
			EXPECT_FALSE(pieces[i][0] < -4.0f && pieces[i][1] > -16.0f && pieces[i][2] < 4.0f && pieces[i][3] > -4.0f);
		}
		EXPECT_NEAR(26.0f * 48.0f - 12.0f * 8.0f, area, 0.01f);
	}

	// 追加の3面：どの倍率でも、P1〜P4 の開始位置が床の上にあり、障害物に重ならない（少し歩いても壁に止められない）。
	TEST(MapCalcTest, NewMaps_SpawnsStandOnFloorAtEveryScale)
	{
		const STAGE_MAP maps[] = { STAGE_MAP_NEON, STAGE_MAP_COLISEUM, STAGE_MAP_STATION };
		const float scales[] = { 0.7f, 1.0f, 1.5f };
		for (STAGE_MAP map : maps) {
			for (float scale : scales) {
				const STAGE stage = Build(map, scale);
				ASSERT_EQ(4, stage.SpawnCount);
				for (int i = 0; i < stage.SpawnCount; ++i) {
					const VEC3& spawn = stage.Spawns[i];
					const int name = static_cast<int>(map);
					float ground = 0.0f;
					ASSERT_TRUE(StageCalcFollowGround(stage, spawn.X, spawn.Y, spawn.Z, ground)) << name << " " << i;
					EXPECT_NEAR(spawn.Y, ground, 0.001f) << name << " " << i;
					float x = 0.0f, z = 0.0f;
					EXPECT_EQ(0, StageCalcMoveBody(stage, spawn.X, spawn.Y, spawn.Z, 0.3f, 0.3f, true, x, z)) << name << " " << i;
				}
			}
		}
	}

	// ネオンビル群：道路・歩道・ビル・車・ジャンプ台の高さ。外周から先は床がない。
	TEST(MapCalcTest, Neon_HeightsFollowSpec)
	{
		const STAGE stage = Build(STAGE_MAP_NEON);
		ExpectHeight(stage, 0.0f, 0.0f, 0.0f);
		ExpectHeight(stage, 0.0f, 20.0f, 0.0f);		// 横断歩道（歩道は |X| 5.3 から外）
		ExpectHeight(stage, 10.0f, 11.0f, 0.18f);	// ビルの前の歩道
		ExpectHeight(stage, -9.2f, -18.1f, 16.5f);	// NOVA の屋上
		ExpectHeight(stage, -17.0f, -18.4f, 14.84f);	// VINYL の屋上の設備
		ExpectHeight(stage, 14.0f, 6.5f, 1.425f);	// 車室
		ExpectHeight(stage, -15.0f, 9.0f, 0.2f);
		EXPECT_EQ(STAGE_SURFACE_PAD, stage.Regions[StageCalcFindRegion(stage, -15.0f, 9.0f)].Surface);
		ExpectNoFloor(stage, 28.1f, 0.0f);
	}

	// コロシアム：円い床（半径 21）と、斜めに置いた低い石壁。
	TEST(MapCalcTest, Coliseum_RoundFloorAndRotatedWalls)
	{
		const STAGE stage = Build(STAGE_MAP_COLISEUM);
		ExpectHeight(stage, 20.9f, 0.0f, 0.0f);
		ExpectNoFloor(stage, 15.0f, 15.0f);	// 四角の角は円の外
		ExpectHeight(stage, 0.0f, 0.0f, 7.5f);	// 中央の柱
		// 石壁（中心 (−4.7, −17.1)、向き −0.25）の長い辺の端は、回した位置にある。
		const float c = cosf(-0.25f), s = sinf(-0.25f);
		ExpectHeight(stage, -4.7f + 1.8f * c, -17.1f - 1.8f * s, 1.16f);
		ExpectHeight(stage, -4.7f + 1.85f, -17.1f + 0.55f, 0.0f);	// 回さない箱の角なら入っていた所
	}

	// 廃駅：線路とホームは坂でつながり、坂のない所のホームのふちは壁になる。
	TEST(MapCalcTest, Station_AccessRampsConnectTrackAndPlatform)
	{
		const STAGE stage = Build(STAGE_MAP_STATION);
		float y = 0.0f;
		for (float z = 0.0f; z <= 10.0f; z += 0.1f) {
			float ground = 0.0f;
			ASSERT_TRUE(StageCalcFollowGround(stage, -1.8f, y, z, ground)) << z;
			y = ground;
		}
		EXPECT_NEAR(1.5f, y, 0.001f);
		float x = 0.0f, z = 0.0f;
		EXPECT_EQ(STAGE_STOP_X | STAGE_STOP_Z, StageCalcMoveBody(stage, -10.0f, 0.0f, 7.0f, 0.0f, 0.6f, true, x, z));
	}

	// 廃駅の倒壊ビル：片方のホームから歩いて登り、いちばん高い所（6.4）を越えて反対のホームへ渡れる。
	// 線路の上では、下をくぐる体は線路に立ち、上から落ちてきた体は足場に着地する。
	TEST(MapCalcTest, Station_CollapsedBuildingIsWalkableAndPassableBelow)
	{
		const STAGE stage = Build(STAGE_MAP_STATION);
		const float s = sinf(0.435f), c = cosf(0.435f);
		const auto onLine = [&](float along) { return VEC3{ 10.0f + along * s, 0.0f, along * c }; };
		float y = 1.5f, highest = 0.0f;
		for (float along = -18.0f; along <= 18.0f; along += 0.1f) {
			const VEC3 p = onLine(along);
			float ground = 0.0f;
			ASSERT_TRUE(StageCalcFollowGround(stage, p.X, y, p.Z, ground)) << along;
			y = ground;
			highest = fmaxf(highest, y);
		}
		EXPECT_NEAR(6.4f, highest, 0.05f);
		EXPECT_NEAR(1.5f, y, 0.05f);

		const VEC3 over = onLine(-6.0f);	// 線路の上（足場の高さは約 4）
		float ground = 0.0f;
		ASSERT_TRUE(StageCalcFollowGround(stage, over.X, 0.0f, over.Z, ground));
		EXPECT_NEAR(0.0f, ground, 0.001f);
		float x = 0.0f, z = 0.0f;
		EXPECT_EQ(0, StageCalcMoveBody(stage, over.X, 0.0f, over.Z, 0.5f, 0.0f, true, x, z));
		float landing = 0.0f;
		ASSERT_TRUE(StageCalcFindLanding(stage, over.X, over.Z, 6.0f, 3.0f, landing));
		EXPECT_GT(landing, 3.5f);
		EXPECT_FALSE(StageCalcFindLanding(stage, over.X, over.Z, 3.0f, 2.0f, landing));	// 足場より下を落ちる体は足場に乗らない
		ASSERT_TRUE(StageCalcFindLanding(stage, over.X, over.Z, 0.2f, -0.1f, landing));
		EXPECT_NEAR(0.0f, landing, 0.001f);
	}
}
