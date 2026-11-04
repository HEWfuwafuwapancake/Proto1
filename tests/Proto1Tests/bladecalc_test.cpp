#include "CppUnitTest.h"
#include "physics/bladecalc.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

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

	TEST_CLASS(BladeCalcTest)
	{
	public:
		// 押し続けると 1・2・3 振り目が当たり、そのあと休んで待機に戻る。
		TEST_METHOD(Step_ThreePressesGiveThreeSlashes)
		{
			BladeCalc blade;
			int hits[8] = {};
			const int count = RunBlade(blade, 40, 6, hits, 8); // 3振り目のあとの休みの間に押しても始まらない
			Assert::AreEqual(3, count);
			Assert::AreEqual(1, hits[0]);
			Assert::AreEqual(3, hits[2]);
			RunBlade(blade, 60, 0, hits, 8);
			Assert::AreEqual((int)BLADE_IDLE, (int)blade.GetState());
		}

		// 受け付ける間に押さなければ休みに入り、次の入力は1振り目から。
		TEST_METHOD(Step_WindowExpiresAndRestartsAtFirst)
		{
			BladeCalc blade;
			int hits[4] = {};
			blade.Step(true, false, BLADE_DT, BLADE_TEST_PARAMS);
			Assert::AreEqual(1, RunBlade(blade, 60, 0, hits, 4));
			Assert::AreEqual((int)BLADE_IDLE, (int)blade.GetState());
			blade.Step(true, false, BLADE_DT, BLADE_TEST_PARAMS);
			Assert::AreEqual(1, blade.GetSlash());
		}
	};
}
