#include <gtest/gtest.h>
#include <cmath>
#include "physics/flailcalc.h"

namespace
{
	constexpr float DT = 1.0f / 60.0f;
	constexpr FLAIL_THROW_PARAMS PARAMS{ 30.0f, 30.0f, 0.3f };
}

// ためるほど飛距離が伸び、最大で止まる。
TEST(FlailCalcTest, ChargeGrowsAndStopsAtFull)
{
	EXPECT_NEAR(0.5f, FlailCalcCharge(0.75f, 1.5f), 0.0001f);
	EXPECT_EQ(1.0f, FlailCalcCharge(3.0f, 1.5f));
	EXPECT_NEAR(10.0f, FlailCalcMix(6.0f, 14.0f, 0.5f), 0.0001f);
	EXPECT_NEAR(14.0f, FlailCalcMix(6.0f, 14.0f, 2.0f), 0.0001f);
}

// 頭の上の鉄球は、角度 0 で前（+Z）、90° で +X にある。
TEST(FlailCalcTest, SpinPositionGoesAroundCenter)
{
	const VEC3 a = FlailCalcSpinPosition({ 1.0f, 2.0f, 3.0f }, 0.0f, 2.0f);
	EXPECT_NEAR(1.0f, a.X, 0.0001f);
	EXPECT_NEAR(2.0f, a.Y, 0.0001f);
	EXPECT_NEAR(5.0f, a.Z, 0.0001f);
	const VEC3 b = FlailCalcSpinPosition({ 0.0f, 0.0f, 0.0f }, 1.5707963f, 2.0f);
	EXPECT_NEAR(2.0f, b.X, 0.0001f);
	EXPECT_NEAR(0.0f, b.Z, 0.0001f);
}

// 行きは決めた距離で止まって帰りになり、帰りは動いた手元を追って受け止める。
TEST(FlailCalcTest, ThrowGoesOutThenReturnsToMovingHand)
{
	FlailCalc flail;
	flail.Throw({ 0.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f }, 10.0f);
	for (int i = 0; i < 15; ++i) flail.Step(DT, { 0.0f, 0.0f, 0.0f }, PARAMS);
	EXPECT_EQ(FLAIL_OUT, flail.GetState());
	EXPECT_NEAR(7.5f, flail.GetPosition().X, 0.001f);
	float farthest = 0.0f;
	for (int i = 0; i < 10; ++i) {
		flail.Step(DT, { 0.0f, 0.0f, 0.0f }, PARAMS);
		farthest = fmaxf(farthest, flail.GetPosition().X);
	}
	EXPECT_NEAR(10.0f, farthest, 0.001f);
	EXPECT_EQ(FLAIL_BACK, flail.GetState());
	// 手元が横へ動いていても、そこへ戻る。
	const VEC3 hand{ 0.0f, 0.0f, 4.0f };
	for (int i = 0; i < 60 && flail.IsFlying(); ++i) flail.Step(DT, hand, PARAMS);
	EXPECT_EQ(FLAIL_IDLE, flail.GetState());
	EXPECT_NEAR(4.0f, flail.GetPosition().Z, 0.001f);
}
