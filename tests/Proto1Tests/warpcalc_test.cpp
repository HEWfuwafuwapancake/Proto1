#include <gtest/gtest.h>
#include "physics/warpcalc.h"

namespace Proto1Tests
{
	static constexpr STAGE WARP_STAGE{ 0.0f, 0.0f, 5.0f, 5.0f, 0.0f, 1.0f, -10.0f };

	// 相手の手前 1.2 に止まり、もう近ければ動かない。
	TEST(WarpCalcTest, NearTarget_StopsBeforeTarget)
	{
		const VEC3 p = WarpCalcNearTarget({ 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 4.0f }, 1.2f);
		EXPECT_NEAR(2.8f, p.Z, 0.0001f);
		const VEC3 stay = WarpCalcNearTarget({ 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, 1.2f);
		EXPECT_NEAR(0.0f, stay.Z, 0.0001f);
	}

	// 浮島の外へは、ふちまで縮める。浮島の外からは縮めない。
	TEST(WarpCalcTest, Destination_ClampsToPlatformEdge)
	{
		const VEC3 edge = WarpCalcDestination({ 3.0f, 0.0f, 0.0f }, { 8.0f, 0.0f, 0.0f }, WARP_STAGE);
		EXPECT_NEAR(5.0f, edge.X, 0.01f);
		EXPECT_TRUE(edge.X <= 5.0f);
		const VEC3 inside = WarpCalcDestination({ 0.0f, 0.0f, 0.0f }, { 3.0f, 0.0f, 0.0f }, WARP_STAGE);
		EXPECT_NEAR(3.0f, inside.X, 0.0001f);
		const VEC3 back = WarpCalcDestination({ 8.0f, -2.0f, 0.0f }, { 13.0f, -2.0f, 0.0f }, WARP_STAGE);
		EXPECT_NEAR(13.0f, back.X, 0.0001f);
	}
}
