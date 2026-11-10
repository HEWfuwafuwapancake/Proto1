#include <gtest/gtest.h>
#include "physics/armpresscalc.h"

static constexpr float ARM_PRESS_DT = 1.0f / 60.0f;

// 0.25 秒になる前に離すと、そのフレームだけ単押しになる。
TEST(ArmPressCalcTest, ReleasesAsTapBeforeThreshold)
{
	ArmPressCalc press;
	for (int frame = 0; frame < 14; ++frame) {
		const ARM_INPUT input = press.Step(true, ARM_PRESS_DT);
		EXPECT_TRUE(input.Pressed);
		EXPECT_FALSE(input.Tap);
		EXPECT_FALSE(input.HoldStarted);
	}
	const ARM_INPUT release = press.Step(false, ARM_PRESS_DT);
	EXPECT_TRUE(release.Tap);
	EXPECT_FALSE(release.Pressed);
	EXPECT_FALSE(release.HoldStarted);
	EXPECT_FALSE(press.Step(false, ARM_PRESS_DT).Tap);
}

// 60 fps では15フレーム目に長押しが始まり、その後は押下中だけが続く。
TEST(ArmPressCalcTest, StartsHoldOnceAtThreshold)
{
	ArmPressCalc press;
	for (int frame = 0; frame < 14; ++frame) press.Step(true, ARM_PRESS_DT);
	const ARM_INPUT start = press.Step(true, ARM_PRESS_DT);
	EXPECT_TRUE(start.HoldStarted);
	EXPECT_TRUE(start.Pressed);
	EXPECT_FALSE(start.Tap);

	const ARM_INPUT held = press.Step(true, ARM_PRESS_DT);
	EXPECT_FALSE(held.HoldStarted);
	EXPECT_TRUE(held.Pressed);
	EXPECT_FALSE(held.Tap);
	EXPECT_FALSE(press.Step(false, ARM_PRESS_DT).Tap);
}

// 中断前に押した入力は離して捨て、押し直すと再び単押しになる。
TEST(ArmPressCalcTest, CancelledPressMustBeReleasedBeforeRearming)
{
	ArmPressCalc press;
	press.Step(true, ARM_PRESS_DT);
	press.Reset(true);
	for (int frame = 0; frame < 15; ++frame) {
		EXPECT_FALSE(press.Step(true, ARM_PRESS_DT).HoldStarted);
	}
	EXPECT_FALSE(press.Step(false, ARM_PRESS_DT).Tap);
	press.Step(true, ARM_PRESS_DT);
	EXPECT_TRUE(press.Step(false, ARM_PRESS_DT).Tap);
}

// 腕の動作中に押した単押しは覚えておき、受け付けられるようになったフレームで渡す。
TEST(ArmPressCalcTest, BuffersTapWhileArmIsBusy)
{
	ArmPressCalc press;
	press.Step(true, ARM_PRESS_DT, false, false);
	EXPECT_FALSE(press.Step(false, ARM_PRESS_DT, false, false).Tap);
	EXPECT_FALSE(press.Step(false, ARM_PRESS_DT, false, false).Tap);
	EXPECT_TRUE(press.Step(false, ARM_PRESS_DT, true, true).Tap);
	EXPECT_FALSE(press.Step(false, ARM_PRESS_DT, true, true).Tap);
}

// 覚えておく時間（0.2 秒）を過ぎた単押しは捨てる。
TEST(ArmPressCalcTest, DropsBufferedTapAfterWindow)
{
	ArmPressCalc press;
	press.Step(true, ARM_PRESS_DT, false, false);
	press.Step(false, ARM_PRESS_DT, false, false);
	for (int frame = 0; frame < 13; ++frame) press.Step(false, ARM_PRESS_DT, false, false);
	EXPECT_FALSE(press.Step(false, ARM_PRESS_DT, true, true).Tap);
}

// 腕の動作中に長押しに達しても消さず、押し続けていれば受け付けられたフレームで始める。
TEST(ArmPressCalcTest, HoldWaitsUntilArmIsReady)
{
	ArmPressCalc press;
	for (int frame = 0; frame < 20; ++frame) {
		EXPECT_FALSE(press.Step(true, ARM_PRESS_DT, false, false).HoldStarted);
	}
	EXPECT_TRUE(press.Step(true, ARM_PRESS_DT, true, true).HoldStarted);
	EXPECT_FALSE(press.Step(true, ARM_PRESS_DT, true, true).HoldStarted);
}
