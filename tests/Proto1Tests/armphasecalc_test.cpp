#include <gtest/gtest.h>
#include "physics/armphasecalc.h"

namespace Proto1Tests
{
	static constexpr float PHASE_SECONDS[3] = { 0.1f, 0.2f, 0.3f };

	// 1/60 秒刻みで、溜め 6 フレーム・攻撃 12 フレーム・休み 18 フレームで待機に戻る。
	TEST(ArmPhaseCalcTest, Step_AdvancesPhasesAtSixtyFpsAndFinishes)
	{
		ArmPhaseCalc phase;
		EXPECT_TRUE(phase.IsIdle());
		phase.Start(0);
		int entered[3] = { 0, 0, 0 };
		int frames = 0;
		bool finished = false;
		while (!finished && frames < 100) {
			const ARM_PHASE_STEP step = phase.Step(1.0f / 60.0f, PHASE_SECONDS, 3);
			++frames;
			if (step.Entered >= 0) entered[step.Entered] = frames;
			finished = step.Finished;
		}
		EXPECT_EQ(6, entered[1]);
		EXPECT_EQ(18, entered[2]);
		EXPECT_EQ(36, frames);
		EXPECT_TRUE(phase.IsIdle());
	}

	// 0.3 秒の段階を 0.1 秒刻みで数え、長いフレームでも上限までしか返さない。
	TEST(ArmPhaseCalcTest, Step_CountsTicksWithinPhase)
	{
		const float seconds[1] = { 0.3f };
		ArmPhaseCalc phase;
		phase.Start(0);
		int ticks = 0;
		for (int i = 0; i < 18; ++i) ticks += phase.Step(1.0f / 60.0f, seconds, 1, 0.1f).Ticks;
		EXPECT_EQ(3, ticks);
		const float longPhase[1] = { 10.0f };
		phase.Start(0);
		EXPECT_EQ(ARM_PHASE_MAX_TICKS, phase.Step(1.0f, longPhase, 1, 0.1f).Ticks);
	}

	// 止めた段階は秒数を過ぎても進まず、離すと次の Step で（ためが残っていても）すぐ次の段階へ進む。
	TEST(ArmPhaseCalcTest, Held_WaitsUntilRelease)
	{
		const float seconds[2] = { 0.5f, 0.2f };
		ArmPhaseCalc phase;
		phase.Start(0);
		phase.SetHeld(true);
		for (int i = 0; i < 120; ++i) EXPECT_EQ(-1, phase.Step(1.0f / 60.0f, seconds, 2).Entered);
		EXPECT_EQ(0, phase.GetPhase());
		EXPECT_NEAR(0.5f, phase.GetTime(), 0.0001f);
		phase.Release();
		EXPECT_EQ(1, phase.Step(1.0f / 60.0f, seconds, 2).Entered);

		phase.Start(0);
		phase.SetHeld(true);
		phase.Step(0.1f, seconds, 2);
		phase.Release();
		EXPECT_EQ(1, phase.Step(1.0f / 60.0f, seconds, 2).Entered);
	}
}
