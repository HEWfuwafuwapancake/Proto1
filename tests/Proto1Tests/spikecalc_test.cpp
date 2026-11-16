#include <gtest/gtest.h>
#include "physics/spikecalc.h"

namespace
{
	// 10本を 0.45 秒かけて手前から生やし、0.1 秒で伸び、0.5 秒残り、0.1 秒で引っこむ。
	constexpr SPIKE_LINE_PARAMS LINE{ 1.0f, 1.0f, 10, 0.45f, 0.1f, 0.5f, 0.1f };

	// 中心 (0, 0)、30 × 30 の浮島（上の面 y = 0）。
	STAGE MakeStage()
	{
		STAGE stage{};
		stage.HalfWidth = 15.0f;
		stage.HalfDepth = 15.0f;
		stage.TopY = 0.0f;
		stage.Thickness = 2.0f;
		stage.OutY = -15.0f;
		return stage;
	}
}

// トゲは手前から順に生え、全部引っこんだら列が終わる。
TEST(SpikeCalcTest, LineGrowsFromNearToFar)
{
	EXPECT_GT(SpikeCalcLineGrowth(LINE, 0, 0.05f), 0.0f);
	EXPECT_EQ(0.0f, SpikeCalcLineGrowth(LINE, 9, 0.05f));
	EXPECT_EQ(1.0f, SpikeCalcLineGrowth(LINE, 9, 0.6f));
	EXPECT_EQ(0.0f, SpikeCalcLineGrowth(LINE, 0, 0.8f));
	EXPECT_FALSE(SpikeCalcLineFinished(LINE, 10, 1.0f));
	EXPECT_TRUE(SpikeCalcLineFinished(LINE, 10, 1.2f));
}

// 浮島のふちから先にはトゲを生やさない。
TEST(SpikeCalcTest, LineStopsAtPlatformEdge)
{
	const STAGE stage = MakeStage();
	EXPECT_EQ(10, SpikeCalcLineCount(stage, { 0.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f }, LINE));
	// x = 10 から +X へ：11, 12, 13, 14, 15 の5本（16 は浮島の外）。
	EXPECT_EQ(5, SpikeCalcLineCount(stage, { 10.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f }, LINE));
}

// 茨の中で動いた距離だけを数え、止まっている・外にいる・入った瞬間は数えない。
TEST(SpikeCalcTest, FieldCountsDistanceMovedInside)
{
	ThornFieldCalc fields;
	fields.Place({ 0.0f, 0.0f, 0.0f }, 4.0f);
	const VEC3 a{ 1.0f, 0.0f, 0.0f };
	const VEC3 b{ 2.0f, 0.0f, 0.0f };
	const VEC3 outside{ 10.0f, 0.0f, 0.0f };
	EXPECT_TRUE(fields.IsInside(a, 4.0f, 1.0f));
	EXPECT_FALSE(fields.IsInside(outside, 4.0f, 1.0f));
	EXPECT_EQ(0.0f, fields.TrackMove(0, a, true));
	EXPECT_EQ(0.0f, fields.TrackMove(0, a, true));
	EXPECT_NEAR(1.0f, fields.TrackMove(0, b, true), 0.0001f);
	EXPECT_EQ(0.0f, fields.TrackMove(0, outside, false));
	EXPECT_EQ(0.0f, fields.TrackMove(0, a, true));
}

// 茨は残る秒数が過ぎると消える。
TEST(SpikeCalcTest, FieldExpires)
{
	ThornFieldCalc fields;
	fields.Place({ 0.0f, 0.0f, 0.0f }, 4.0f);
	fields.Step(3.9f);
	EXPECT_EQ(1, fields.GetCount());
	fields.Step(0.2f);
	EXPECT_EQ(0, fields.GetCount());
	EXPECT_FALSE(fields.IsInside({ 0.0f, 0.0f, 0.0f }, 4.0f, 1.0f));
}
