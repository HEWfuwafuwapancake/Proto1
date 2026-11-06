#include <gtest/gtest.h>
#include "physics/bladecalc.h"

namespace Proto1Tests
{
	static constexpr BLADE_PARAMS BLADE_TEST_PARAMS{ 0.08f, 0.08f, 0.35f, 0.3f, 3 };
	static constexpr float BLADE_DT = 1.0f / 60.0f;

	// 指定フレーム数だけ進め、当たった振りの番号を順に記録する。
	static int RunBlade(BladeCalc& blade, int frames, int pressEvery, int* hits, int maxHits)
	{
		int count = 0;
		for (int f = 0; f < frames; ++f) {
			const bool trigger = pressEvery > 0 && f % pressEvery == 0;
			const int slash = blade.Step(trigger, false, BLADE_DT, BLADE_TEST_PARAMS);
			if (slash > 0 && count < maxHits) hits[count++] = slash;
		}
		return count;
	}

	// 押し続けると 1・2・3 振り目が当たり、そのあと休んで待機に戻る。
	TEST(BladeCalcTest, Step_ThreePressesGiveThreeSlashes)
	{
		BladeCalc blade;
		int hits[8] = {};
		const int count = RunBlade(blade, 40, 6, hits, 8); // 3振り目のあとの休みの間に押しても始まらない
		EXPECT_EQ(3, count);
		EXPECT_EQ(1, hits[0]);
		EXPECT_EQ(3, hits[2]);
		RunBlade(blade, 60, 0, hits, 8);
		EXPECT_EQ((int)BLADE_IDLE, (int)blade.GetState());
	}

	// 受け付ける間に押さなければ休みに入り、次の入力は1振り目から。
	TEST(BladeCalcTest, Step_WindowExpiresAndRestartsAtFirst)
	{
		BladeCalc blade;
		int hits[4] = {};
		blade.Step(true, false, BLADE_DT, BLADE_TEST_PARAMS);
		EXPECT_EQ(1, RunBlade(blade, 60, 0, hits, 4));
		EXPECT_EQ((int)BLADE_IDLE, (int)blade.GetState());
		blade.Step(true, false, BLADE_DT, BLADE_TEST_PARAMS);
		EXPECT_EQ(1, blade.GetSlash());
	}
}
