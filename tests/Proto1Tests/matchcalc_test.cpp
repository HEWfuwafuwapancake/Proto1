// physics/matchcalc.h（試合の流れの計算）の単体テスト
#include "CppUnitTest.h"
#include "physics/matchcalc.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace Proto1Tests
{
	static constexpr float DT = 1.0f / 60.0f;	// 1フレームの秒数（ゲームと同じ）

	TEST_CLASS(MatchCalcTest)
	{
	public:
		// 始めは試合中・0 点・残り 90 秒
		TEST_METHOD(Start_InitialState)
		{
			MatchCalc match{};
			match.Start(90.0f, 5);
			Assert::IsTrue(match.GetState() == MATCH_STATE_PLAYING);
			Assert::AreEqual(0, match.GetScore());
			Assert::AreEqual(90, match.GetDisplaySeconds());
		}

		// 場外の回数が 1 増えたら +1（始めの回数が 0 でなくても、増えた分だけ）
		TEST_METHOD(Step_OneOut_AddsOne)
		{
			MatchCalc match{};
			match.Start(90.0f, 5);
			Assert::IsTrue(match.Step(DT, 5) == MATCH_EVENT_NONE);
			Assert::IsTrue(match.Step(DT, 6) == MATCH_EVENT_SCORED);
			Assert::AreEqual(1, match.GetScore());
			// 回数が増えなければ、それ以上は増えない
			Assert::IsTrue(match.Step(DT, 6) == MATCH_EVENT_NONE);
			Assert::AreEqual(1, match.GetScore());
		}

		// 同じフレームに 2 体落ちたら +2
		TEST_METHOD(Step_TwoOutsInOneFrame_AddsTwo)
		{
			MatchCalc match{};
			match.Start(90.0f, 0);
			match.Step(DT, 2);
			Assert::AreEqual(2, match.GetScore());
		}

		// 時間切れは 1 回だけ返り、状態が終わりになる
		TEST_METHOD(Step_TimeUp_ReturnedOnce)
		{
			MatchCalc match{};
			match.Start(1.0f, 0);
			Assert::IsTrue(match.Step(0.5f, 0) == MATCH_EVENT_NONE);
			Assert::IsTrue(match.Step(0.5f, 0) == MATCH_EVENT_TIME_UP);
			Assert::IsTrue(match.GetState() == MATCH_STATE_FINISHED);
			Assert::AreEqual(0, match.GetDisplaySeconds());
			Assert::IsTrue(match.Step(0.5f, 0) == MATCH_EVENT_NONE);
		}

		// 時間切れと同じフレームの場外は得点に入る
		TEST_METHOD(Step_OutOnTimeUpFrame_Counts)
		{
			MatchCalc match{};
			match.Start(0.5f, 0);
			Assert::IsTrue(match.Step(0.5f, 1) == MATCH_EVENT_TIME_UP);
			Assert::AreEqual(1, match.GetScore());
		}

		// 時間切れのあとは、場外が増えても得点しない
		TEST_METHOD(Step_AfterTimeUp_NoScore)
		{
			MatchCalc match{};
			match.Start(0.5f, 0);
			match.Step(0.5f, 0);
			match.Step(DT, 3);
			Assert::AreEqual(0, match.GetScore());
		}

		// もう一度始めると最初に戻る
		TEST_METHOD(Start_AfterFinished_Resets)
		{
			MatchCalc match{};
			match.Start(0.5f, 0);
			match.Step(0.5f, 2);
			match.Start(90.0f, 0);
			Assert::IsTrue(match.GetState() == MATCH_STATE_PLAYING);
			Assert::AreEqual(0, match.GetScore());
			Assert::AreEqual(90, match.GetDisplaySeconds());
		}

		// 表示する秒数は切り上げ（残り 0.01 秒でも 1）。90 秒を 1/60 秒ずつ進めて、ちょうど 5400 フレームで終わる
		TEST_METHOD(DisplaySeconds_RoundsUp)
		{
			MatchCalc match{};
			match.Start(90.0f, 0);
			match.Step(DT, 0);
			Assert::AreEqual(90, match.GetDisplaySeconds());
			for (int i = 0; i < 60; i++) {
				match.Step(DT, 0);
			}
			Assert::AreEqual(89, match.GetDisplaySeconds());

			match.Start(1.0f, 0);
			match.Step(0.99f, 0);
			Assert::AreEqual(1, match.GetDisplaySeconds());
		}
	};
}
