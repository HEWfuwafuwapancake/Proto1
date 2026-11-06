#include <gtest/gtest.h>
#include "physics/shieldcalc.h"

namespace Proto1Tests
{
	static constexpr float SHIELD_EPSILON = 0.0001f;
	static constexpr SHIELD_PARAMS SHIELD_TEST_PARAMS{ 2.0f, 0.15f, 0.3f, 2.0f, 0.9f };

	TEST(ShieldCalcTest, Trigger_RaisesShieldAndKeepsGuardingAfterRelease)
	{
		ShieldCalc arm;
		EXPECT_EQ((int)SHIELD_IDLE, (int)arm.GetState());
		EXPECT_FALSE(arm.Step(true, false, 0.01f, SHIELD_TEST_PARAMS));
		EXPECT_EQ((int)SHIELD_GUARDING, (int)arm.GetState());
		EXPECT_NEAR(0.0f, arm.GetTime(), SHIELD_EPSILON);
		EXPECT_FALSE(arm.Step(false, false, 1.0f, SHIELD_TEST_PARAMS));
		EXPECT_EQ((int)SHIELD_GUARDING, (int)arm.GetState());
		EXPECT_NEAR(1.0f, arm.GetTime(), SHIELD_EPSILON);
	}

	TEST(ShieldCalcTest, Retrigger_BashesOnceThenRestsAndReturnsToIdle)
	{
		ShieldCalc arm;
		arm.Step(true, false, 0.01f, SHIELD_TEST_PARAMS);
		EXPECT_TRUE(arm.Step(true, false, 0.01f, SHIELD_TEST_PARAMS));
		EXPECT_EQ((int)SHIELD_BASHING, (int)arm.GetState());
		EXPECT_NEAR(0.0f, arm.GetTime(), SHIELD_EPSILON);
		EXPECT_FALSE(arm.Step(true, false, 0.1f, SHIELD_TEST_PARAMS));
		EXPECT_EQ((int)SHIELD_BASHING, (int)arm.GetState());
		EXPECT_FALSE(arm.Step(false, false, 0.05f, SHIELD_TEST_PARAMS));
		EXPECT_EQ((int)SHIELD_COOLDOWN, (int)arm.GetState());
		EXPECT_NEAR(0.0f, arm.GetTime(), SHIELD_EPSILON);
		// 休みの途中と、休みが終わるフレームの入力は受け付けない。
		EXPECT_FALSE(arm.Step(true, false, 0.2f, SHIELD_TEST_PARAMS));
		EXPECT_EQ((int)SHIELD_COOLDOWN, (int)arm.GetState());
		EXPECT_FALSE(arm.Step(true, false, 0.1f, SHIELD_TEST_PARAMS));
		EXPECT_EQ((int)SHIELD_IDLE, (int)arm.GetState());
		EXPECT_NEAR(0.0f, arm.GetTime(), SHIELD_EPSILON);
		arm.Step(true, false, 0.01f, SHIELD_TEST_PARAMS);
		EXPECT_EQ((int)SHIELD_GUARDING, (int)arm.GetState());
	}

	TEST(ShieldCalcTest, GuardTimeout_LowersWithoutBashingAfterTwoSeconds)
	{
		ShieldCalc arm;
		arm.Step(true, false, 0.01f, SHIELD_TEST_PARAMS);
		// 実際の60fpsと同じ刻みで2秒を数え、累積誤差でも1フレーム遅れないことを確かめる。
		for (int frame = 0; frame < 119; ++frame) {
			EXPECT_FALSE(arm.Step(false, false, 1.0f / 60.0f, SHIELD_TEST_PARAMS));
		}
		EXPECT_EQ((int)SHIELD_GUARDING, (int)arm.GetState());
		EXPECT_FALSE(arm.Step(false, false, 1.0f / 60.0f, SHIELD_TEST_PARAMS));
		EXPECT_EQ((int)SHIELD_COOLDOWN, (int)arm.GetState());
		EXPECT_NEAR(0.0f, arm.GetTime(), SHIELD_EPSILON);
	}

	TEST(ShieldCalcTest, Respawn_CancelsGuardOrBashWithoutBashing)
	{
		for (SHIELD_STATE state : { SHIELD_GUARDING, SHIELD_BASHING }) {
			ShieldCalc arm;
			arm.Step(true, false, 0.01f, SHIELD_TEST_PARAMS);
			if (state == SHIELD_BASHING) arm.Step(true, false, 0.01f, SHIELD_TEST_PARAMS);
			EXPECT_FALSE(arm.Step(true, true, 0.01f, SHIELD_TEST_PARAMS));
			EXPECT_EQ((int)SHIELD_COOLDOWN, (int)arm.GetState());
			EXPECT_NEAR(0.0f, arm.GetTime(), SHIELD_EPSILON);
		}
	}

	TEST(ShieldCalcTest, Reset_ReturnsToIdleAndClearsTime)
	{
		ShieldCalc arm;
		arm.Step(true, false, 0.01f, SHIELD_TEST_PARAMS);
		arm.Step(false, false, 0.5f, SHIELD_TEST_PARAMS);
		arm.Reset();
		EXPECT_EQ((int)SHIELD_IDLE, (int)arm.GetState());
		EXPECT_NEAR(0.0f, arm.GetTime(), SHIELD_EPSILON);
	}

	TEST(ShieldCalcTest, BashCapsule_ExtendsTwoUnitsHorizontallyFromShoulder)
	{
		const VEC3 shoulder{ 4, 5, 6 };
		const CAPSULE capsule = ShieldCalcBashCapsule(shoulder, { 3, 20, 4 }, SHIELD_TEST_PARAMS);
		EXPECT_NEAR(0.0f, Vec3Length(Vec3Sub(capsule.Start, shoulder)), SHIELD_EPSILON);
		EXPECT_NEAR(5.2f, capsule.End.X, SHIELD_EPSILON);
		EXPECT_NEAR(5.0f, capsule.End.Y, SHIELD_EPSILON);
		EXPECT_NEAR(7.6f, capsule.End.Z, SHIELD_EPSILON);
		EXPECT_NEAR(0.9f, capsule.Radius, SHIELD_EPSILON);
	}

	TEST(ShieldCalcTest, PushVelocity_AddsUpToHorizontalForwardAndHasFixedSpeed)
	{
		const VEC3 velocity = ShieldCalcPushVelocity({ 3, 20, 4 }, 12.0f, 0.15f);
		const float expectedScale = 12.0f / sqrtf(1.0f + 0.15f * 0.15f);
		EXPECT_NEAR(0.6f * expectedScale, velocity.X, SHIELD_EPSILON);
		EXPECT_NEAR(0.15f * expectedScale, velocity.Y, SHIELD_EPSILON);
		EXPECT_NEAR(0.8f * expectedScale, velocity.Z, SHIELD_EPSILON);
		EXPECT_NEAR(12.0f, Vec3Length(velocity), SHIELD_EPSILON);
	}
}
