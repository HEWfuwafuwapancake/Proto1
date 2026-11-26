// 攻撃で倒れる柱（コロシアム。仕様書 02 の「柱の倒壊」。DirectX なし）
#include <gtest/gtest.h>
#include "physics/mapcalc.h"
#include "physics/pillarcalc.h"

namespace Proto1Tests
{
	// 足元 (x, z) に立つ体（高さ 1.8・半径 0.5 のカプセル）。
	static CAPSULE BodyAt(float x, float z)
	{
		return { { x, 0.5f, z }, { x, 1.3f, z }, 0.5f };
	}

	// 中央の柱：同じ手の攻撃は続けて当たらず、耐久が尽きると最後の攻撃の向き（+X）へ倒れる。
	// 倒れる間は、倒した本人も含めて通り道の体に1回ずつ当たり、反対側の体には当たらない。倒れたあとは横たわった箱になる。
	TEST(PillarCalcTest, BrokenPillarFallsTowardLastHitAndHitsBodiesOnce)
	{
		STAGE stage{};
		ASSERT_TRUE(MapCalcBuild(STAGE_MAP_COLISEUM, 1.0f, stage));
		PillarCalc pillars;
		pillars.Setup(stage);
		ASSERT_EQ(5, pillars.GetCount());
		CAPSULE body;
		ASSERT_TRUE(pillars.GetBody(0, body));
		EXPECT_NEAR(0.0f, body.Start.X, 0.001f);
		EXPECT_NEAR(0.0f, body.Start.Z, 0.001f);

		EXPECT_TRUE(pillars.Hit(0, 0, 0, 30.0f, { 0.0f, 0.0f, 1.0f }));
		EXPECT_FALSE(pillars.Hit(0, 0, 0, 30.0f, { 0.0f, 0.0f, 1.0f })); // 同じ手の続けての当たり
		EXPECT_TRUE(pillars.Hit(0, 1, 0, 30.0f, { 0.0f, 0.0f, 1.0f }));
		EXPECT_EQ(PILLAR_STANDING, pillars.GetState(0));
		EXPECT_TRUE(pillars.Hit(0, 2, 1, 30.0f, { 1.0f, 0.5f, 0.0f })); // 最後は +X へ（上下は使わない）
		EXPECT_EQ(PILLAR_WARNING, pillars.GetState(0));
		EXPECT_FALSE(pillars.GetBody(0, body));

		// 0：倒した人（+X の通り道）、1：+X の通り道、2：反対側、3：当たらない状態。
		const CAPSULE bodies[FIGHTER_COUNT] = { BodyAt(3.0f, 0.0f), BodyAt(6.0f, 0.5f), BodyAt(-4.0f, 0.0f), BodyAt(5.0f, -0.5f) };
		const bool hittable[FIGHTER_COUNT] = { true, true, true, false };
		int hits[FIGHTER_COUNT] = {};
		for (int frame = 0; frame < 120; ++frame) {
			PILLAR_IMPACT impacts[8];
			const int count = pillars.Step(1.0f / 60.0f, bodies, hittable, stage, impacts, 8);
			for (int i = 0; i < count; ++i) {
				++hits[impacts[i].Victim];
				EXPECT_EQ(1, impacts[i].Attacker);
				EXPECT_NEAR(1.0f, impacts[i].Direction.X, 0.001f);
			}
		}
		EXPECT_EQ(1, hits[0]);
		EXPECT_EQ(1, hits[1]);
		EXPECT_EQ(0, hits[2]);
		EXPECT_EQ(0, hits[3]);
		EXPECT_EQ(PILLAR_FALLEN, pillars.GetState(0));
		// 倒れた柱は +X に長さ 7.5・高さ 2.04 の箱。根元の反対側と横は床のまま。
		float height = 0.0f;
		ASSERT_TRUE(StageCalcGroundHeight(stage, 6.0f, 0.0f, height));
		EXPECT_NEAR(2.04f, height, 0.001f);
		ASSERT_TRUE(StageCalcGroundHeight(stage, -2.0f, 0.0f, height));
		EXPECT_NEAR(0.0f, height, 0.001f);
		ASSERT_TRUE(StageCalcGroundHeight(stage, 4.0f, 1.5f, height));
		EXPECT_NEAR(0.0f, height, 0.001f);
	}
}
