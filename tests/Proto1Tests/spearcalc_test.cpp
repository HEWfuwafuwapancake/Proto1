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
