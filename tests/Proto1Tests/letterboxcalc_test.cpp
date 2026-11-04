#include "CppUnitTest.h"
#include "core/letterboxcalc.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace Proto1Tests
{
	static constexpr float LETTERBOX_EPSILON = 0.001f;

	TEST_CLASS(LetterboxCalcTest)
	{
	public:
		// 横長のウィンドウは左右に、縦長のウィンドウは上下に、同じ幅の黒い帯ができる。
		TEST_METHOD(Calc_BarsOnLongerSide)
		{
			const LETTERBOX wide = LetterboxCalc(1920.0f, 1000.0f, 1280.0f, 720.0f);
			Assert::AreEqual(1000.0f / 720.0f, wide.Scale, LETTERBOX_EPSILON);
			Assert::AreEqual((1920.0f - 1280.0f * wide.Scale) * 0.5f, wide.OffsetX, LETTERBOX_EPSILON);
			Assert::AreEqual(0.0f, wide.OffsetY, LETTERBOX_EPSILON);
			const LETTERBOX tall = LetterboxCalc(640.0f, 900.0f, 1280.0f, 720.0f);
			Assert::AreEqual(0.5f, tall.Scale, LETTERBOX_EPSILON);
			Assert::AreEqual(0.0f, tall.OffsetX, LETTERBOX_EPSILON);
			Assert::AreEqual((900.0f - 360.0f) * 0.5f, tall.OffsetY, LETTERBOX_EPSILON);
		}
	};
}
