// core/framestats.h（フレームの間隔と処理時間）の単体テスト
#include "CppUnitTest.h"
#include "core/framestats.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace Proto1Tests
{
	TEST_CLASS(FrameStatsTest)
	{
	public:
		// ふだんの59フレームと重い1フレームを集計し、取り出したら次の1秒のために空にする
		TEST_METHOD(Take_SummarizesFramesAndClears)
		{
			FRAME_STATS stats{};
			for (int i = 0; i < 59; ++i) FrameStatsAdd(stats, 16.7f, 3.0f);
			FrameStatsAdd(stats, 50.0f, 15.0f);
			Assert::AreEqual(60, stats.FrameCount);

			const FRAME_STATS_RESULT result = FrameStatsTake(stats);
			Assert::AreEqual(50.0f, result.MaxIntervalMs, 0.0001f);
			Assert::AreEqual(3.2f, result.AverageWorkMs, 0.0001f);
			Assert::AreEqual(1, result.HitchCount);
			Assert::AreEqual(0, stats.FrameCount);
			Assert::AreEqual(0.0f, stats.TotalWorkMs);
			const FRAME_STATS_RESULT empty = FrameStatsTake(stats);
			Assert::AreEqual(0.0f, empty.MaxIntervalMs);
			Assert::AreEqual(0.0f, empty.AverageWorkMs);
			Assert::AreEqual(0, empty.HitchCount);
		}
	};
}
