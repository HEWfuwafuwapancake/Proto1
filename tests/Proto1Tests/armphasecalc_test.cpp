#include "CppUnitTest.h"
#include "physics/armphasecalc.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace Proto1Tests
{
	static constexpr float PHASE_SECONDS[3] = { 0.1f, 0.2f, 0.3f };

	TEST_CLASS(ArmPhaseCalcTest)
	{
	public:
		// 1/60 秒刻みで、溜め 6 フレーム・攻撃 12 フレーム・休み 18 フレームで待機に戻る。
		TEST_METHOD(Step_AdvancesPhasesAtSixtyFpsAndFinishes)
		{
			ArmPhaseCalc phase;
			Assert::IsTrue(phase.IsIdle());
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
			Assert::AreEqual(6, entered[1]);
			Assert::AreEqual(18, entered[2]);
			Assert::AreEqual(36, frames);
			Assert::IsTrue(phase.IsIdle());
		}

		// 0.3 秒の段階を 0.1 秒刻みで数え、長いフレームでも上限までしか返さない。
		TEST_METHOD(Step_CountsTicksWithinPhase)
		{
			const float seconds[1] = { 0.3f };
			ArmPhaseCalc phase;
			phase.Start(0);
			int ticks = 0;
			for (int i = 0; i < 18; ++i) ticks += phase.Step(1.0f / 60.0f, seconds, 1, 0.1f).Ticks;
			Assert::AreEqual(3, ticks);
			const float longPhase[1] = { 10.0f };
			phase.Start(0);
			Assert::AreEqual(ARM_PHASE_MAX_TICKS, phase.Step(1.0f, longPhase, 1, 0.1f).Ticks);
		}
	};
}
