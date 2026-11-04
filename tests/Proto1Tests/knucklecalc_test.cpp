#include "CppUnitTest.h"
#include "physics/knucklecalc.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace Proto1Tests
{
	static constexpr KNUCKLE_PARAMS KNUCKLE_TEST_PARAMS{ 1.0f, { 10.0f, 2.2f, false }, { 14.0f, 2.8f, true } };

	TEST_CLASS(KnuckleCalcTest)
	{
	public:
		// 至近は杭で強く、離れていれば普通の拳。
		TEST_METHOD(Pick_PileWhenClose)
		{
			const KNUCKLE_HIT pile = KnuckleCalcPick(0.8f, KNUCKLE_TEST_PARAMS);
			Assert::IsTrue(pile.Pile);
			Assert::AreEqual(14.0f, pile.Damage);
			const KNUCKLE_HIT normal = KnuckleCalcPick(1.5f, KNUCKLE_TEST_PARAMS);
			Assert::IsFalse(normal.Pile);
			Assert::AreEqual(2.2f, normal.Power);
		}
	};
}
