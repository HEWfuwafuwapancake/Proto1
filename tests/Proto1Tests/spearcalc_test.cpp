#include "gtest/gtest.h"
#include "physics/spearcalc.h"

namespace
{
	const SPEAR_THROW_PARAMS PARAMS{ 45.0f, 25.0f, 0.4f };

	// x, z の位置に立つ体（太さ 0.4）
	TENTACLE_TARGET Body(float x, float z)
	{
		return { { { x, 0.5f, z }, { x, 1.5f, z }, 0.4f }, true };
	}

	STAGE Island()
	{
		STAGE stage{};
		stage.HalfWidth = 10.0f;
		stage.HalfDepth = 10.0f;
		stage.TopY = 0.0f;
		stage.OutY = -20.0f;
		return stage;
	}
}

// 投げた槍は直線上を進み、手前の相手に刺さる（奥の相手には刺さらない）。
TEST(SpearCalcTest, ThrowSticksToNearestTargetOnLine)
{
	const TENTACLE_TARGET targets[] = { Body(0.0f, 15.0f), Body(0.0f, 8.0f), Body(3.0f, 4.0f) };
	SpearCalc spear;
	spear.Throw({ 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f });
	SPEAR_THROW_EVENT event = SPEAR_THROW_NONE;
	for (int i = 0; i < 60 && event == SPEAR_THROW_NONE; ++i) event = spear.Step(1.0f / 60.0f, PARAMS, targets, 3);
	EXPECT_EQ(SPEAR_THROW_STUCK, event);
	EXPECT_EQ(1, spear.GetStuckTarget());
	EXPECT_FALSE(spear.IsFlying());
	EXPECT_NEAR(8.0f, spear.GetTip().Z, 0.5f);
}

// 誰もいなければ、届く距離を超えたところで外れになる。
TEST(SpearCalcTest, ThrowMissesAfterRange)
{
	const TENTACLE_TARGET targets[] = { Body(5.0f, 10.0f) };
	SpearCalc spear;
	spear.Throw({ 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f });
	SPEAR_THROW_EVENT event = SPEAR_THROW_NONE;
	int frames = 0;
	for (; frames < 120 && event == SPEAR_THROW_NONE; ++frames) event = spear.Step(1.0f / 60.0f, PARAMS, targets, 1);
	EXPECT_EQ(SPEAR_THROW_MISSED, event);
	EXPECT_FALSE(spear.IsStuck());
	EXPECT_NEAR(PARAMS.Range, spear.GetTip().Z, 0.01f);
}

// 浮島の上にいる相手は落ちていない。浮島の外か、上の面より下へ出たら落ちた。
TEST(SpearCalcTest, HasFallenWhenOffIsland)
{
	const STAGE stage = Island();
	EXPECT_FALSE(SpearCalcHasFallen(stage, { 3.0f, 2.0f, -4.0f }));
	EXPECT_TRUE(SpearCalcHasFallen(stage, { 12.0f, 2.0f, 0.0f }));
	EXPECT_TRUE(SpearCalcHasFallen(stage, { 0.0f, -1.0f, 0.0f }));
}

namespace
{
	const SPEAR_THRUST_PARAMS THRUST{ 2.2f, 12.0f, 2.4f, 7.0f, 1.4f };
}

// 突きは、体の手前の面が肩から 2.2m より先なら穂先の強い当たり、近ければ手元の弱い当たりになる。
TEST(SpearCalcTest, ThrustTipHitsHarderThanBase)
{
	const VEC3 shoulder{ 0.0f, 1.0f, 0.0f };
	const VEC3 forward{ 0.0f, 0.0f, 1.0f };
	const SPEAR_THRUST_HIT tip = SpearCalcThrustHit(shoulder, forward, Body(0.0f, 2.9f).Body, THRUST);
	EXPECT_TRUE(tip.Tip);
	EXPECT_NEAR(12.0f, tip.Damage, 1e-4f);
	EXPECT_NEAR(2.4f, tip.Power, 1e-4f);
	const SPEAR_THRUST_HIT base = SpearCalcThrustHit(shoulder, forward, Body(0.0f, 1.5f).Body, THRUST);
	EXPECT_FALSE(base.Tip);
	EXPECT_NEAR(7.0f, base.Damage, 1e-4f);
	EXPECT_NEAR(1.4f, base.Power, 1e-4f);
}

// 踏み込みは、前の直線上で穂先の距離より近くに相手が入ったら止める（遠い相手・横の相手では止めない）。
TEST(SpearCalcTest, LungeStopsWhenTargetWithinTipStart)
{
	const VEC3 shoulder{ 0.0f, 1.0f, 0.0f };
	const VEC3 forward{ 0.0f, 0.0f, 1.0f };
	const TENTACLE_TARGET farTargets[] = { Body(0.0f, 4.0f), Body(2.0f, 1.0f) };
	EXPECT_FALSE(SpearCalcShouldStopLunge(shoulder, forward, THRUST, 0.4f, farTargets, 2));
	const TENTACLE_TARGET nearTargets[] = { Body(0.0f, 4.0f), Body(0.0f, 2.2f) };
	EXPECT_TRUE(SpearCalcShouldStopLunge(shoulder, forward, THRUST, 0.4f, nearTargets, 2));
}
