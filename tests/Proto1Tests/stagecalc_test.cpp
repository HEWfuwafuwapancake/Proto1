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

	// 高台が2つある中心 (0, 0) の浮島。(0, 10) が近い高台、(0, 14) が遠い高台。
	static STAGE MakeGripStage(void)
	{
		STAGE stage = MakeStage();
		stage.CenterX = 0.0f;
		stage.CenterZ = 0.0f;
		stage.Platforms[0] = { 0.0f, 10.0f, 1.0f, 1.0f, 4.0f, 0.6f };
		stage.Platforms[1] = { 0.0f, 14.0f, 1.0f, 1.0f, 4.0f, 0.6f };
		stage.PlatformCount = 2;
		return stage;
	}
	static constexpr VEC3 GRIP_FEET{ 0.0f, 0.0f, 0.0f };
	static constexpr VEC3 GRIP_ORIGIN{ 0.0f, 1.35f, 0.0f };
	static constexpr float GRIP_RANGE = 16.0f;
	static constexpr float GRIP_HALF_ANGLE = 0.7f;

	// 正面の高台のうち、いちばん近いふち（手前の辺）を選ぶ
	TEST(StageCalcTest, FindGrip_ChoosesNearestRimInFront)
	{
		const STAGE stage = MakeGripStage();
		STAGE_GRIP grip{};
		ASSERT_TRUE(StageCalcFindGrip(stage, GRIP_ORIGIN, GRIP_FEET, { 0.0f, 0.0f, 1.0f }, GRIP_RANGE, GRIP_HALF_ANGLE, grip));
		EXPECT_NEAR(9.0f, grip.Point.Z, 0.001f);
		EXPECT_NEAR(4.0f, grip.Point.Y, 0.001f);
		EXPECT_NEAR(10.0f, grip.Surface.CenterZ, 0.001f);
	}

	// 見上げているときは、近い高台ではなく見ている先の高台を選ぶ
	TEST(StageCalcTest, FindGrip_LookingUpChoosesRimInView)
	{
		const STAGE stage = MakeGripStage();
		STAGE_GRIP grip{};
		const VEC3 toFar = Vec3Sub({ 0.0f, 4.0f, 14.0f }, GRIP_ORIGIN);
		ASSERT_TRUE(StageCalcFindGrip(stage, GRIP_ORIGIN, GRIP_FEET, toFar, GRIP_RANGE, GRIP_HALF_ANGLE, grip));
		EXPECT_NEAR(14.0f, grip.Surface.CenterZ, 0.001f);
	}

	// 後ろの高台と、遠すぎる高台は選ばない
	TEST(StageCalcTest, FindGrip_IgnoresBehindAndTooFar)
	{
		const STAGE stage = MakeGripStage();
		STAGE_GRIP grip{};
		EXPECT_FALSE(StageCalcFindGrip(stage, GRIP_ORIGIN, GRIP_FEET, { 0.0f, 0.0f, -1.0f }, GRIP_RANGE, GRIP_HALF_ANGLE, grip));
		EXPECT_FALSE(StageCalcFindGrip(stage, GRIP_ORIGIN, GRIP_FEET, { 0.0f, 0.0f, 1.0f }, 5.0f, GRIP_HALF_ANGLE, grip));
	}

	// 立っている高台は選ばず、その先の高台を選ぶ
	TEST(StageCalcTest, FindGrip_SkipsPlatformBeingStoodOn)
	{
		const STAGE stage = MakeGripStage();
		const VEC3 feet{ 0.0f, 4.0f, 10.0f };
		STAGE_GRIP grip{};
		ASSERT_TRUE(StageCalcFindGrip(stage, { 0.0f, 5.35f, 10.0f }, feet, { 0.0f, 0.0f, 1.0f }, GRIP_RANGE, GRIP_HALF_ANGLE, grip));
		EXPECT_NEAR(14.0f, grip.Surface.CenterZ, 0.001f);
	}

	// 高台の下で起きた爆発は、高台の上の相手に届かない。高台の外へずれた所や、同じ面の上どうしは届く
	TEST(StageCalcTest, IsBlocked_PlatformBetween)
	{
		STAGE stage = MakeStage();
		stage.Platforms[0] = { 0.0f, 0.0f, 2.0f, 2.0f, 4.0f, 0.5f };
		stage.PlatformCount = 1;
		EXPECT_TRUE(StageCalcIsBlocked(stage, { 0.0f, 0.3f, 0.0f }, { 0.0f, 5.0f, 0.0f }));
		EXPECT_FALSE(StageCalcIsBlocked(stage, { 6.0f, 0.3f, 0.0f }, { 6.0f, 5.0f, 0.0f }));
		EXPECT_FALSE(StageCalcIsBlocked(stage, { 0.0f, 0.3f, 0.0f }, { 3.0f, 1.0f, 0.0f }));
	}

	// 浮島のふちは、場外にいるときだけ選ぶ
	TEST(StageCalcTest, FindGrip_MainIslandOnlyWhenOffIt)
	{
		STAGE stage = MakeGripStage();
		stage.PlatformCount = 0;
		STAGE_GRIP grip{};
		EXPECT_FALSE(StageCalcFindGrip(stage, GRIP_ORIGIN, GRIP_FEET, { 0.0f, 0.0f, 1.0f }, GRIP_RANGE, GRIP_HALF_ANGLE, grip));
		ASSERT_TRUE(StageCalcFindGrip(stage, { 0.0f, -1.65f, -18.0f }, { 0.0f, -3.0f, -18.0f }, { 0.0f, 0.0f, 1.0f }, GRIP_RANGE, GRIP_HALF_ANGLE, grip));
		EXPECT_NEAR(-15.0f, grip.Point.Z, 0.001f);
		EXPECT_NEAR(stage.TopY, grip.Surface.TopY, 0.001f);
	}
}
