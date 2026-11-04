#include "CppUnitTest.h"
#include "physics/thundercalc.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace Proto1Tests
{
	// 足元が (x, 0, z) の高さ 1.8・半径 0.5 の体。
	static TENTACLE_TARGET ThunderTarget(float x, float z, bool available = true)
	{
		return { { { x, 0.5f, z }, { x, 1.3f, z }, 0.5f }, available };
	}

	TEST_CLASS(ThunderCalcTest)
	{
	public:
		// 近い順に連鎖し、遠すぎる・つかめない相手と同じ相手には行かず、最大人数で止まる。
		TEST_METHOD(Chain_NearestUnusedWithinRange)
		{
			const TENTACLE_TARGET targets[5] = {
				ThunderTarget(0.0f, 0.0f), ThunderTarget(3.0f, 0.0f), ThunderTarget(2.0f, 0.0f),
				ThunderTarget(1.0f, 1.0f, false), ThunderTarget(20.0f, 0.0f)
			};
			int order[3] = { -1, -1, -1 };
			Assert::AreEqual(3, ThunderCalcChain(targets, 5, 0, 4.0f, 3, order));
			Assert::AreEqual(0, order[0]);
			Assert::AreEqual(2, order[1]);
			Assert::AreEqual(1, order[2]);
			int two[3] = { -1, -1, -1 };
			Assert::AreEqual(2, ThunderCalcChain(targets, 5, 4, 30.0f, 2, two));
			Assert::AreEqual(1, two[1]);
		}
	};
}
