// physics/gunlancecalc.h（ガンランスの単発射撃と撃ち込み）の単体テスト
#include <gtest/gtest.h>
#include "physics/gunlancecalc.h"

namespace Proto1Tests
{
	static constexpr float FRAME_DT = 1.0f / 60.0f;

	// 単発射撃は 0.25 秒あける。入力判定では、このフレームの時間も含める。
	TEST(GunlanceCalcTest, SingleShotWaitsForCooldown)
	{
		GunlanceCalc gun;
		EXPECT_TRUE(gun.CanFire());
		gun.RecordShot();
		for (int frame = 1; frame < 15; ++frame) {
			EXPECT_FALSE(gun.CanFire(FRAME_DT));
			EXPECT_EQ(0, gun.Step(FRAME_DT, false).Shots);
		}
		EXPECT_FALSE(gun.CanFire());
		EXPECT_TRUE(gun.CanFire(FRAME_DT));
		gun.Step(FRAME_DT, false);
		EXPECT_TRUE(gun.CanFire());
	}

	// 相手をつかむ前の突進は、長押しを離すか 2 秒たつと終わる。
	TEST(GunlanceCalcTest, RushEndsOnReleaseOrTimeout)
	{
		GunlanceCalc gun;
		gun.StartRush();
		gun.Step(0.5f, true);
		EXPECT_TRUE(gun.IsRushing());
		EXPECT_FALSE(gun.CanFire());
		gun.Step(FRAME_DT, false);
		EXPECT_TRUE(gun.IsIdle());

		gun.StartRush();
		for (int frame = 1; frame < 120; ++frame) gun.Step(FRAME_DT, true);
		EXPECT_TRUE(gun.IsRushing());
		EXPECT_EQ(0, gun.Step(FRAME_DT, true).Shots);
		EXPECT_TRUE(gun.IsIdle());
	}

	// 突き上げは 0.35 秒。離しても撃ち込みを続け、0.2 秒間隔で3発撃つ。
	TEST(GunlanceCalcTest, ImpaleLiftsThenFiresThreeShots)
	{
		GunlanceCalc gun;
		gun.StartRush();
		gun.StartImpale(-1);
		for (int frame = 1; frame <= 21; ++frame) {
			EXPECT_EQ(0, gun.Step(FRAME_DT, false).Shots);
			EXPECT_TRUE(gun.IsHolding());
		}
		EXPECT_FLOAT_EQ(1.0f, gun.GetLiftProgress());
		for (int frame = 1; frame <= 36; ++frame) {
			const GUNLANCE_STEP step = gun.Step(FRAME_DT, false);
			EXPECT_EQ(frame % 12 == 0 ? 1 : 0, step.Shots);
			EXPECT_EQ(frame == 36, step.Finished);
			EXPECT_EQ(frame != 36, gun.IsHolding());
		}
		EXPECT_TRUE(gun.IsIdle());
	}

	// 残弾が1発または2発なら、その数で撃ち込みを終える。
	TEST(GunlanceCalcTest, ImpaleEndsWithLastAvailableShot)
	{
		for (int remaining = 1; remaining <= 2; ++remaining) {
			GunlanceCalc gun;
			gun.StartRush();
			gun.StartImpale(remaining);
			EXPECT_EQ(0, gun.Step(0.35f, false).Shots);
			const GUNLANCE_STEP step = gun.Step(0.2f * remaining, false);
			EXPECT_EQ(remaining, step.Shots);
			EXPECT_TRUE(step.Finished);
			EXPECT_TRUE(gun.IsIdle());
			EXPECT_EQ(0, gun.Step(0.2f, false).Shots);
		}
	}
}
