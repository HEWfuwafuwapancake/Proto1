// physics/gunlancecalc.h（ガンランスの段階のためと、当たる間隔）の単体テスト
#include <gtest/gtest.h>
#include "physics/gunlancecalc.h"

namespace Proto1Tests
{
	static const GUNLANCE_PARAMS GUNLANCE_PARAMS_VALUE{ 1.0f, 0.5f };
	static const float GUNLANCE_DT = 1.0f / 60.0f;

	// 3回押すと、3回目のフレームに3段階で発射する
	TEST(GunlanceCalcTest, ThreeTaps_FireAtMaxLevel)
	{
		GunlanceCalc gun;
		EXPECT_EQ(0, gun.Step(true, GUNLANCE_DT, GUNLANCE_PARAMS_VALUE));
		EXPECT_EQ(1, gun.GetLevel());
		EXPECT_EQ(0, gun.Step(true, GUNLANCE_DT, GUNLANCE_PARAMS_VALUE));
		EXPECT_EQ(2, gun.GetLevel());
		EXPECT_EQ(3, gun.Step(true, GUNLANCE_DT, GUNLANCE_PARAMS_VALUE));
		EXPECT_EQ(0, gun.GetLevel());
	}

	// 押したあと 1 秒押さないと、その段階で発射する（1/60 秒刻み）
	TEST(GunlanceCalcTest, Waiting_FiresCurrentLevelAfterOneSecond)
	{
		GunlanceCalc gun;
		gun.Step(true, GUNLANCE_DT, GUNLANCE_PARAMS_VALUE);
		int frames = 0, fired = 0;
		while (fired == 0 && frames < 120) {
			fired = gun.Step(false, GUNLANCE_DT, GUNLANCE_PARAMS_VALUE);
			++frames;
		}
		EXPECT_EQ(1, fired);
		EXPECT_NEAR(60, frames, 1);
	}

	// 押すと待ち時間が最初に戻る
	TEST(GunlanceCalcTest, TappingAgain_RestartsWait)
	{
		GunlanceCalc gun;
		gun.Step(true, GUNLANCE_DT, GUNLANCE_PARAMS_VALUE);
		EXPECT_EQ(0, gun.Step(false, 0.9f, GUNLANCE_PARAMS_VALUE));
		gun.Step(true, GUNLANCE_DT, GUNLANCE_PARAMS_VALUE);
		EXPECT_EQ(0, gun.Step(false, 0.9f, GUNLANCE_PARAMS_VALUE));
		EXPECT_EQ(2, gun.Step(false, 0.2f, GUNLANCE_PARAMS_VALUE));
	}

	// 押していないときは何も起きない
	TEST(GunlanceCalcTest, NoTap_NeverFires)
	{
		GunlanceCalc gun;
		EXPECT_EQ(0, gun.Step(false, 5.0f, GUNLANCE_PARAMS_VALUE));
		EXPECT_EQ(0, gun.GetLevel());
	}

	// 同じ相手には 0.5 秒あけないと当たらず、別の相手には当たる
	TEST(GunlanceCalcTest, Rehit_WaitsPerTarget)
	{
		GunlanceCalc gun;
		EXPECT_TRUE(gun.TryHit(1, GUNLANCE_PARAMS_VALUE));
		EXPECT_FALSE(gun.TryHit(1, GUNLANCE_PARAMS_VALUE));
		EXPECT_TRUE(gun.TryHit(2, GUNLANCE_PARAMS_VALUE));
		gun.Step(false, 0.4f, GUNLANCE_PARAMS_VALUE);
		EXPECT_FALSE(gun.TryHit(1, GUNLANCE_PARAMS_VALUE));
		gun.Step(false, 0.2f, GUNLANCE_PARAMS_VALUE);
		EXPECT_TRUE(gun.TryHit(1, GUNLANCE_PARAMS_VALUE));
	}
}
