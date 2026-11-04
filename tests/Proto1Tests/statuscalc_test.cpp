#include "CppUnitTest.h"
#include "physics/statuscalc.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace Proto1Tests
{
	static constexpr float STATUS_DT = 1.0f / 60.0f;

	TEST_CLASS(StatusCalcTest)
	{
	public:
		// しびれは時間で切れ、その間は歩けない。遅いは倍率を返す。
		TEST_METHOD(StunAndSlow_ExpireAndScale)
		{
			StatusCalc status;
			status.Apply(STATUS_STUN, 0.5f, 0.0f, 1);
			status.Apply(STATUS_SLOW, 2.0f, 0.4f, 1);
			Assert::AreEqual(0.0f, status.GetMoveScale());
			for (int f = 0; f < 31; ++f) status.Step(STATUS_DT);
			Assert::IsFalse(status.IsActive(STATUS_STUN));
			Assert::AreEqual(0.4f, status.GetMoveScale(), 0.0001f);
		}

		// 2秒燃えると 1秒あたりの % × 2 だけ増え、かけ直すと長い方が残る。
		TEST_METHOD(Burn_TotalAndRefresh)
		{
			StatusCalc status;
			status.Apply(STATUS_BURN, 2.0f, 2.0f, 3);
			status.Apply(STATUS_BURN, 1.0f, 2.0f, 3);
			float total = 0.0f;
			int attacker = -1;
			for (int f = 0; f < 180; ++f) {
				const STATUS_STEP step = status.Step(STATUS_DT);
				total += step.Damage;
				if (step.Attacker >= 0) attacker = step.Attacker;
			}
			Assert::AreEqual(4.0f, total, 0.01f);
			Assert::AreEqual(3, attacker);
			Assert::IsFalse(status.IsActive(STATUS_BURN));
		}
	};
}
