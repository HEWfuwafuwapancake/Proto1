// core/timer.h（「○秒数える」タイマー）の単体テスト
#include "CppUnitTest.h"
#include "core/timer.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace Proto1Tests
{
	static constexpr float TIMER_EPSILON = 1.0e-5f;	// float の比較で許す誤差
	static constexpr float DT = 1.0f / 60.0f;		// 1フレームの秒数（ゲームと同じ）

	TEST_CLASS(TimerTest)
	{
	public:
		// 数え始めた直後は、残りが渡した秒数で、まだ終わっていない
		TEST_METHOD(Start_SetsRemaining)
		{
			TIMER timer{};
			TimerStart(timer, 90.0f);
			Assert::AreEqual(90.0f, TimerGetRemaining(timer), TIMER_EPSILON);
			Assert::IsFalse(TimerIsFinished(timer));
		}

		// 進めた分だけ残りが減る
		TEST_METHOD(Update_DecreasesRemaining)
		{
			TIMER timer{};
			TimerStart(timer, 3.0f);
			TimerUpdate(timer, 1.0f);
			Assert::AreEqual(2.0f, TimerGetRemaining(timer), TIMER_EPSILON);
			Assert::IsFalse(TimerIsFinished(timer));
		}

		// 1 秒のタイマーは、59 フレームではまだ終わらず、60 フレームあまりで数え終わる
		TEST_METHOD(OneSecond_FinishesAroundFrame60)
		{
			TIMER timer{};
			TimerStart(timer, 1.0f);
			for (int i = 0; i < 59; i++) {
				TimerUpdate(timer, DT);
			}
			Assert::IsFalse(TimerIsFinished(timer));

			// float の足し算の誤差で 60 フレームちょうどでは 0 にならないことがあるので、1 フレーム余分に進めて確かめる
			TimerUpdate(timer, DT);
			TimerUpdate(timer, DT);
			Assert::IsTrue(TimerIsFinished(timer));
		}

		// 0 を過ぎても負にならない
		TEST_METHOD(Update_NeverGoesNegative)
		{
			TIMER timer{};
			TimerStart(timer, 0.5f);
			TimerUpdate(timer, 2.0f);
			Assert::AreEqual(0.0f, TimerGetRemaining(timer), TIMER_EPSILON);
			Assert::IsTrue(TimerIsFinished(timer));
		}

		// dt が 0（時間が止まっている）なら減らない
		TEST_METHOD(Update_ZeroDtDoesNotChange)
		{
			TIMER timer{};
			TimerStart(timer, 5.0f);
			TimerUpdate(timer, 0.0f);
			Assert::AreEqual(5.0f, TimerGetRemaining(timer), TIMER_EPSILON);
		}

		// 数えている途中で Start すると、最初からやり直す
		TEST_METHOD(Start_RestartsWhileRunning)
		{
			TIMER timer{};
			TimerStart(timer, 3.0f);
			TimerUpdate(timer, 2.5f);
			TimerStart(timer, 3.0f);
			Assert::AreEqual(3.0f, TimerGetRemaining(timer), TIMER_EPSILON);
			Assert::AreEqual(3.0f, timer.Duration, TIMER_EPSILON);
		}

		// 負の秒数は 0 にして、すぐに数え終わる
		TEST_METHOD(Start_NegativeSecondsFinishesImmediately)
		{
			TIMER timer{};
			TimerStart(timer, -1.0f);
			Assert::AreEqual(0.0f, TimerGetRemaining(timer), TIMER_EPSILON);
			Assert::IsTrue(TimerIsFinished(timer));
		}

		// { } で初期化しただけのタイマーは、数え終わった状態
		TEST_METHOD(ZeroInitialized_IsFinished)
		{
			TIMER timer{};
			Assert::IsTrue(TimerIsFinished(timer));
		}
	};
}
