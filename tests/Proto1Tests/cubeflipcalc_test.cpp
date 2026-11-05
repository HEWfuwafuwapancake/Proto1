#include "CppUnitTest.h"
#include "render/cubeflipcalc.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace Proto1Tests
{
	TEST_CLASS(CubeFlipCalcTest)
	{
	public:
		TEST_METHOD(Flip_KeepsTotalHeightAndArrangesFacesInBothDirections)
		{
			constexpr float height = 32.0f;
			constexpr float tolerance = 0.0001f;
			const CUBE_FLIP start = CubeFlipCalc(0.0f, height, 1);
			const CUBE_FLIP down = CubeFlipCalc(0.5f, height, 1);
			const CUBE_FLIP up = CubeFlipCalc(0.5f, height, -1);
			const CUBE_FLIP end = CubeFlipCalc(1.0f, height, 1);
			Assert::AreEqual(height, start.Previous.Height, tolerance);
			Assert::AreEqual(0.0f, start.Next.Height, tolerance);
			Assert::AreEqual(height * 0.5f, down.Previous.Height, tolerance);
			Assert::AreEqual(height, down.Previous.Height + down.Next.Height, tolerance);
			Assert::AreEqual(down.Previous.Height, down.Next.Y, tolerance);
			Assert::AreEqual(up.Next.Height, up.Previous.Y, tolerance);
			Assert::AreEqual(0.0f, end.Previous.Height, tolerance);
			Assert::AreEqual(height, end.Next.Height, tolerance);
		}
	};
}
