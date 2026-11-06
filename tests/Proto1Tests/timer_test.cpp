// core/timer.h（「○秒数える」タイマー）の単体テスト
#include <gtest/gtest.h>
#include "core/timer.h"

namespace Proto1Tests
{
	static constexpr float TIMER_EPSILON = 1.0e-5f;	// float の比較で許す誤差
	static constexpr float DT = 1.0f / 60.0f;		// 1フレームの秒数（ゲームと同じ）

	// 数え始めた直後は、残りが渡した秒数で、まだ終わっていない
	TEST(TimerTest, Start_SetsRemaining)
	{
		TIMER timer{};
		TimerStart(timer, 90.0f);
		EXPECT_NEAR(90.0f, TimerGetRemaining(timer), TIMER_EPSILON);
		EXPECT_FALSE(TimerIsFinished(timer));
	}

	// 進めた分だけ残りが減る
	TEST(TimerTest, Update_DecreasesRemaining)
	{
		TIMER timer{};
		TimerStart(timer, 3.0f);
		TimerUpdate(timer, 1.0f);
		EXPECT_NEAR(2.0f, TimerGetRemaining(timer), TIMER_EPSILON);
		EXPECT_FALSE(TimerIsFinished(timer));
	}

	// 1 秒のタイマーは、59 フレームではまだ終わらず、60 フレームあまりで数え終わる
	TEST(TimerTest, OneSecond_FinishesAroundFrame60)
	{
		TIMER timer{};
		TimerStart(timer, 1.0f);
		for (int i = 0; i < 59; i++) {
			TimerUpdate(timer, DT);
		}
		EXPECT_FALSE(TimerIsFinished(timer));

		// float の足し算の誤差で 60 フレームちょうどでは 0 にならないことがあるので、1 フレーム余分に進めて確かめる
		TimerUpdate(timer, DT);
		TimerUpdate(timer, DT);
		EXPECT_TRUE(TimerIsFinished(timer));
	}

	// 0 を過ぎても負にならない
	TEST(TimerTest, Update_NeverGoesNegative)
	{
		TIMER timer{};
		TimerStart(timer, 0.5f);
		TimerUpdate(timer, 2.0f);
		EXPECT_NEAR(0.0f, TimerGetRemaining(timer), TIMER_EPSILON);
		EXPECT_TRUE(TimerIsFinished(timer));
	}

	// 数えている途中で Start すると、最初からやり直す
	TEST(TimerTest, Start_RestartsWhileRunning)
	{
		TIMER timer{};
		TimerStart(timer, 3.0f);
		TimerUpdate(timer, 2.5f);
		TimerStart(timer, 3.0f);
		EXPECT_NEAR(3.0f, TimerGetRemaining(timer), TIMER_EPSILON);
		EXPECT_NEAR(3.0f, timer.Duration, TIMER_EPSILON);
	}
}
