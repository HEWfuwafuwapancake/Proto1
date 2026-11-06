#include <gtest/gtest.h>
#include "physics/rocketpunchcalc.h"

namespace Proto1Tests
{
	static constexpr STAGE ROCKET_STAGE{ 0.0f, 0.0f, 20.0f, 20.0f, 0.0f, 1.0f, -10.0f };
	static constexpr ROCKET_PARAMS ROCKET_TEST_PARAMS{ 22.0f, 10.0f, 0.35f, 26.0f };
	static constexpr float ROCKET_DT = 1.0f / 60.0f;

	// 飛んで相手に当たると戻り始め、肩に着くと待機に戻ってまた撃てる。
	TEST(RocketPunchCalcTest, Step_HitReturnsThenIdle)
	{
		const TENTACLE_TARGET target{ { { 0.0f, 0.5f, 5.0f }, { 0.0f, 1.3f, 5.0f }, 0.5f }, true };
		const VEC3 shoulder{ 0.0f, 1.2f, 0.0f };
		RocketPunchCalc fist;
		EXPECT_TRUE(fist.Launch(shoulder, { 0.0f, 0.0f, 1.0f }, ROCKET_TEST_PARAMS));
		EXPECT_FALSE(fist.Launch(shoulder, { 0.0f, 0.0f, 1.0f }, ROCKET_TEST_PARAMS));
		int hit = -1;
		for (int f = 0; f < 30 && hit < 0; ++f) hit = fist.Step(false, ROCKET_DT, shoulder, &target, 1, ROCKET_STAGE, ROCKET_TEST_PARAMS);
		EXPECT_EQ(0, hit);
		EXPECT_EQ((int)ROCKET_RETURNING, (int)fist.GetState());
		for (int f = 0; f < 60; ++f) fist.Step(false, ROCKET_DT, shoulder, &target, 1, ROCKET_STAGE, ROCKET_TEST_PARAMS);
		EXPECT_EQ((int)ROCKET_IDLE, (int)fist.GetState());
	}

	// 飛んでいる間に呼び戻すと、当たらずに戻る。
	TEST(RocketPunchCalcTest, Step_RecallReturnsWithoutHit)
	{
		const VEC3 shoulder{ 0.0f, 1.2f, 0.0f };
		RocketPunchCalc fist;
		fist.Launch(shoulder, { 1.0f, 0.0f, 0.0f }, ROCKET_TEST_PARAMS);
		for (int f = 0; f < 10; ++f) fist.Step(false, ROCKET_DT, shoulder, nullptr, 0, ROCKET_STAGE, ROCKET_TEST_PARAMS);
		fist.Step(true, ROCKET_DT, shoulder, nullptr, 0, ROCKET_STAGE, ROCKET_TEST_PARAMS);
		EXPECT_EQ((int)ROCKET_RETURNING, (int)fist.GetState());
	}
}
