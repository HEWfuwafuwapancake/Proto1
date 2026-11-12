// physics/dashcalc.h（ダッシュの残り時間・休み・空中の1回）の単体テスト
#include <gtest/gtest.h>
#include "physics/dashcalc.h"

namespace Proto1Tests
{
	static constexpr float DASH_DT = 1.0f / 60.0f;	// 1フレームの秒数（ゲームと同じ）
	static constexpr DASH_PARAMS DASH_TEST_PARAMS{ 18.0f, 0.2f, 0.6f };

	// calc を frames フレーム進め、進んだ距離を返す
	static float RunDash(DashCalc& calc, int frames, bool onGround)
	{
		float distance = 0.0f;
		for (int i = 0; i < frames; i++) distance += calc.Step(DASH_DT, onGround, DASH_TEST_PARAMS).X * DASH_DT;
		return distance;
	}

	// 決めた秒数だけ決めた速さで進んで止まる。休みの間は次を出せず、休みが終わると出せる
	TEST(DashCalcTest, Dash_RunsForSecondsThenWaitsCooldown)
	{
		DashCalc calc;
		EXPECT_TRUE(calc.TryStart({ 2.0f, 0.0f, 0.0f }, true, DASH_TEST_PARAMS, true));
		EXPECT_NEAR(18.0f * 0.2f, RunDash(calc, 30, true), 0.35f); // 0.5 秒進めても、進むのは 0.2 秒分だけ
		EXPECT_FALSE(calc.IsDashing());
		EXPECT_FALSE(calc.TryStart({ 1.0f, 0.0f, 0.0f }, true, DASH_TEST_PARAMS, true)); // 0.5 秒：まだ休み（0.8 秒まで）
		RunDash(calc, 24, true);
		EXPECT_TRUE(calc.TryStart({ 1.0f, 0.0f, 0.0f }, true, DASH_TEST_PARAMS, true)); // 0.9 秒：出せる
	}

	// 空中では着地まで1回だけ。着地すると、また出せる
	TEST(DashCalcTest, Dash_OnceInAirUntilLanding)
	{
		DashCalc calc;
		EXPECT_TRUE(calc.TryStart({ 0.0f, 0.0f, 1.0f }, false, DASH_TEST_PARAMS, true));
		RunDash(calc, 60, false); // 休みも終わる
		EXPECT_FALSE(calc.TryStart({ 0.0f, 0.0f, 1.0f }, false, DASH_TEST_PARAMS, true));
		RunDash(calc, 1, true); // 着地
		EXPECT_TRUE(calc.TryStart({ 0.0f, 0.0f, 1.0f }, false, DASH_TEST_PARAMS, true));
	}

	// 進み続けない始め方（吹っ飛び中）は、速度を返さずに休みだけ始まる
	TEST(DashCalcTest, Dash_ImpulseOnlyStartsCooldown)
	{
		DashCalc calc;
		EXPECT_TRUE(calc.TryStart({ 1.0f, 0.0f, 0.0f }, true, DASH_TEST_PARAMS, false));
		EXPECT_NEAR(0.0f, RunDash(calc, 6, true), 1.0e-5f);
		EXPECT_FALSE(calc.TryStart({ 1.0f, 0.0f, 0.0f }, true, DASH_TEST_PARAMS, false));
	}
}
