// 腕の巡回・直接選択（DirectX なし）の単体テスト
#include "CppUnitTest.h"
#include "physics/armcalc.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace Proto1Tests
{
	TEST_CLASS(ArmCalcTest)
	{
	public:
		TEST_METHOD(Cycle_Next)
		{
			const bool available[] = { true, true, true, true, true, true };
			Assert::AreEqual(3, ArmCalcCycle(2, 1, available, 6));
		}

		TEST_METHOD(Cycle_Previous)
		{
			const bool available[] = { true, true, true, true, true, true };
			Assert::AreEqual(1, ArmCalcCycle(2, -1, available, 6));
		}

		TEST_METHOD(Cycle_WrapsBothEnds)
		{
			const bool available[] = { true, true, true, true, true, true };
			Assert::AreEqual(0, ArmCalcCycle(5, 1, available, 6));
			Assert::AreEqual(5, ArmCalcCycle(0, -1, available, 6));
		}

		TEST_METHOD(Cycle_SkipsUnavailableInBothDirections)
		{
			const bool available[] = { true, false, false, true, false, false };
			Assert::AreEqual(3, ArmCalcCycle(0, 1, available, 6));
			Assert::AreEqual(0, ArmCalcCycle(3, -1, available, 6));
			Assert::AreEqual(0, ArmCalcCycle(3, 1, available, 6));
			Assert::AreEqual(3, ArmCalcCycle(0, -1, available, 6));
		}

		TEST_METHOD(Cycle_SkipsUnavailableAtBothEnds)
		{
			const bool available[] = { false, true, false, false, true, false };
			Assert::AreEqual(1, ArmCalcCycle(4, 1, available, 6));
			Assert::AreEqual(4, ArmCalcCycle(1, -1, available, 6));
		}

		TEST_METHOD(Cycle_OnlyCurrentAvailableDoesNotChange)
		{
			const bool tentacleOnly[] = { true, false, false, false, false, false };
			Assert::AreEqual(0, ArmCalcCycle(0, 1, tentacleOnly, 6));
			Assert::AreEqual(0, ArmCalcCycle(0, -1, tentacleOnly, 6));
			const bool middleOnly[] = { false, false, true, false, false, false };
			Assert::AreEqual(2, ArmCalcCycle(2, 1, middleOnly, 6));
			Assert::AreEqual(2, ArmCalcCycle(2, -1, middleOnly, 6));
			const bool single[] = { true };
			Assert::AreEqual(0, ArmCalcCycle(0, 1, single, 1));
			Assert::AreEqual(0, ArmCalcCycle(0, -1, single, 1));
		}

		TEST_METHOD(Cycle_NoAvailableReturnsMinusOne)
		{
			const bool available[] = { false, false, false, false, false, false };
			Assert::AreEqual(-1, ArmCalcCycle(0, 1, available, 6));
			Assert::AreEqual(-1, ArmCalcCycle(0, -1, available, 6));
		}

		TEST_METHOD(Select_AvailableNumber)
		{
			const bool available[] = { true, false, true, false, false, true };
			Assert::AreEqual(0, ArmCalcSelect(0, available, 6));
			Assert::AreEqual(2, ArmCalcSelect(2, available, 6));
			Assert::AreEqual(5, ArmCalcSelect(5, available, 6));
		}

		TEST_METHOD(Select_UnavailableReturnsMinusOne)
		{
			const bool available[] = { true, false, false, false, false, false };
			for (int number = 1; number < 6; ++number) Assert::AreEqual(-1, ArmCalcSelect(number, available, 6));
		}

		TEST_METHOD(Select_OutOfRangeReturnsMinusOne)
		{
			const bool available[] = { true, true, true, true, true, true };
			Assert::AreEqual(-1, ArmCalcSelect(-1, available, 6));
			Assert::AreEqual(-1, ArmCalcSelect(6, available, 6));
			Assert::AreEqual(-1, ArmCalcSelect(100, available, 6));
		}

		TEST_METHOD(InvalidArgumentsReturnMinusOne)
		{
			const bool available[] = { true };
			Assert::AreEqual(-1, ArmCalcCycle(0, 1, nullptr, 1));
			Assert::AreEqual(-1, ArmCalcCycle(0, 1, available, 0));
			Assert::AreEqual(-1, ArmCalcCycle(-1, 1, available, 1));
			Assert::AreEqual(-1, ArmCalcCycle(1, 1, available, 1));
			Assert::AreEqual(-1, ArmCalcCycle(0, 0, available, 1));
			Assert::AreEqual(-1, ArmCalcCycle(0, 2, available, 1));
			Assert::AreEqual(-1, ArmCalcSelect(0, nullptr, 1));
			Assert::AreEqual(-1, ArmCalcSelect(0, available, 0));
			Assert::AreEqual(-1, ArmCalcSelect(0, available, -1));
		}
	};
}
