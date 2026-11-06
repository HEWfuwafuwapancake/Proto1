// physics/stagecalc.h（浮島の上にいるか・場外に落ちたか）の単体テスト
#include <gtest/gtest.h>
#include "physics/stagecalc.h"

namespace Proto1Tests
{
	// ゲームと同じ形の浮島（30 × 30、上の面 y = 0、下の線 y = -15）。中心だけずらして、中心を使っているかも確かめる
	static STAGE MakeStage(void)
	{
		STAGE stage{};
		stage.CenterX = 2.0f;
		stage.CenterZ = -1.0f;
		stage.HalfWidth = 15.0f;
		stage.HalfDepth = 15.0f;
		stage.TopY = 0.0f;
		stage.Thickness = 2.0f;
		stage.OutY = -15.0f;
		return stage;
	}

	// 浮島の真ん中は「浮島がある」
	TEST(StageCalcTest, OnPlatform_Center)
	{
		STAGE stage = MakeStage();
		EXPECT_TRUE(StageCalcIsOnPlatform(stage, 2.0f, -1.0f));
	}

	// 端ちょうどは「ある」、少しでも外は「ない」（4つの辺すべて）
	TEST(StageCalcTest, OnPlatform_EdgesAndOutside)
	{
		STAGE stage = MakeStage();
		EXPECT_TRUE(StageCalcIsOnPlatform(stage, 17.0f, -1.0f));		// 右の端
		EXPECT_TRUE(StageCalcIsOnPlatform(stage, -13.0f, -1.0f));	// 左の端
		EXPECT_TRUE(StageCalcIsOnPlatform(stage, 2.0f, 14.0f));		// 奥の端
		EXPECT_TRUE(StageCalcIsOnPlatform(stage, 2.0f, -16.0f));		// 手前の端

		EXPECT_FALSE(StageCalcIsOnPlatform(stage, 17.1f, -1.0f));
		EXPECT_FALSE(StageCalcIsOnPlatform(stage, -13.1f, -1.0f));
		EXPECT_FALSE(StageCalcIsOnPlatform(stage, 2.0f, 14.1f));
		EXPECT_FALSE(StageCalcIsOnPlatform(stage, 2.0f, -16.1f));
	}

	// 下の線より上は場外でない。ちょうどもまだ場外でない。下なら場外
	TEST(StageCalcTest, IsOut_BottomLine)
	{
		STAGE stage = MakeStage();
		EXPECT_FALSE(StageCalcIsOut(stage, 0.0f));
		EXPECT_FALSE(StageCalcIsOut(stage, -14.9f));
		EXPECT_FALSE(StageCalcIsOut(stage, -15.0f));
		EXPECT_TRUE(StageCalcIsOut(stage, -15.1f));
	}

	// 横や上に飛ばされても、高さが下の線より上なら、まだ場外でない（場外の線は下だけ）
	TEST(StageCalcTest, IsOut_FarSideOrHighIsNotOut)
	{
		STAGE stage = MakeStage();
		EXPECT_FALSE(StageCalcIsOnPlatform(stage, 100.0f, 0.0f));
		EXPECT_FALSE(StageCalcIsOut(stage, 50.0f));
	}
}
