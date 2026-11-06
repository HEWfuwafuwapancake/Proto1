// core/framestats.h（フレームの間隔と処理時間）の単体テスト
#include <gtest/gtest.h>
#include "core/framestats.h"

namespace Proto1Tests
{
	// ふだんの59フレームと重い1フレームを集計し、取り出したら次の1秒のために空にする
	TEST(FrameStatsTest, Take_SummarizesFramesAndClears)
	{
		FRAME_STATS stats{};
		for (int i = 0; i < 59; ++i) FrameStatsAdd(stats, 16.7f, 3.0f);
		FrameStatsAdd(stats, 50.0f, 15.0f);
		EXPECT_EQ(60, stats.FrameCount);

		const FRAME_STATS_RESULT result = FrameStatsTake(stats);
		EXPECT_NEAR(50.0f, result.MaxIntervalMs, 0.0001f);
		EXPECT_NEAR(3.2f, result.AverageWorkMs, 0.0001f);
		EXPECT_EQ(1, result.HitchCount);
		EXPECT_EQ(0, stats.FrameCount);
		EXPECT_EQ(0.0f, stats.TotalWorkMs);
		const FRAME_STATS_RESULT empty = FrameStatsTake(stats);
		EXPECT_EQ(0.0f, empty.MaxIntervalMs);
		EXPECT_EQ(0.0f, empty.AverageWorkMs);
		EXPECT_EQ(0, empty.HitchCount);
	}
}
