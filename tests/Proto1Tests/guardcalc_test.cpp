#include "CppUnitTest.h"
#include "physics/guardcalc.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace Proto1Tests
{
	TEST_CLASS(GuardCalcTest)
	{
	public:
		// 前だけのガードは前からの攻撃だけ防ぎ、横・後ろからは防がない。全方向は全部防ぐ。
		TEST_METHOD(Blocks_FrontOnlyAndAll)
		{
			GUARD front;
			front.Kind = GUARD_FRONT;
			front.Forward = { 0.0f, 0.0f, 1.0f };
			front.HalfAngle = 3.14159265f / 3.0f;
			const VEC3 center{ 0.0f, 1.0f, 0.0f };
			Assert::IsTrue(GuardCalcBlocks(front, center, { 0.5f, 1.0f, 3.0f }));
			Assert::IsFalse(GuardCalcBlocks(front, center, { 3.0f, 1.0f, 0.0f }));
			Assert::IsFalse(GuardCalcBlocks(front, center, { 0.0f, 1.0f, -3.0f }));
			GUARD all;
			all.Kind = GUARD_ALL;
			Assert::IsTrue(GuardCalcBlocks(all, center, { 0.0f, 1.0f, -3.0f }));
			Assert::IsFalse(GuardCalcBlocks(GUARD{}, center, { 0.0f, 1.0f, 3.0f }));
			Assert::AreEqual((int)GUARD_ALL, (int)GuardCalcStronger(front, all).Kind);
		}
	};
}
