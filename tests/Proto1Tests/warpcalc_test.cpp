#include "CppUnitTest.h"
#include "physics/warpcalc.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace Proto1Tests
{
	static constexpr STAGE WARP_STAGE{ 0.0f, 0.0f, 5.0f, 5.0f, 0.0f, 1.0f, -10.0f };

	TEST_CLASS(WarpCalcTest)
	{
	public:
		// 相手の手前 1.2 に止まり、もう近ければ動かない。
		TEST_METHOD(NearTarget_StopsBeforeTarget)
		{
			const VEC3 p = WarpCalcNearTarget({ 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 4.0f }, 1.2f);
			Assert::AreEqual(2.8f, p.Z, 0.0001f);
			const VEC3 stay = WarpCalcNearTarget({ 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, 1.2f);
			Assert::AreEqual(0.0f, stay.Z, 0.0001f);
		}

		// 浮島の外へは、ふちまで縮める。浮島の外からは縮めない。
		TEST_METHOD(Destination_ClampsToPlatformEdge)
		{
			const VEC3 edge = WarpCalcDestination({ 3.0f, 0.0f, 0.0f }, { 8.0f, 0.0f, 0.0f }, WARP_STAGE);
			Assert::AreEqual(5.0f, edge.X, 0.01f);
			Assert::IsTrue(edge.X <= 5.0f);
			const VEC3 inside = WarpCalcDestination({ 0.0f, 0.0f, 0.0f }, { 3.0f, 0.0f, 0.0f }, WARP_STAGE);
			Assert::AreEqual(3.0f, inside.X, 0.0001f);
			const VEC3 back = WarpCalcDestination({ 8.0f, -2.0f, 0.0f }, { 13.0f, -2.0f, 0.0f }, WARP_STAGE);
			Assert::AreEqual(13.0f, back.X, 0.0001f);
		}
	};
}
