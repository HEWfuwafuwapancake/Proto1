#include <gtest/gtest.h>
#include "render/animcalc.h"

// 末尾の重複キーの時刻で次の周の0番に戻る。
TEST(AnimCalcTest, LoopsAtDuplicateLastFrame)
{
	EXPECT_FLOAT_EQ(AnimCalcLoopTime(59.0f / 30.0f, 2.0f), 59.0f / 30.0f);
	EXPECT_FLOAT_EQ(AnimCalcLoopTime(2.0f, 2.0f), 0.0f);
	EXPECT_NEAR(AnimCalcLoopTime(61.0f / 30.0f, 2.0f), 1.0f / 30.0f, 0.0001f);
}

// 移動と拡大は線形、回転は角度が時間に比例するように補間する。
TEST(AnimCalcTest, InterpolatesBetweenKeys)
{
	ANIM_TRACK track;
	track.Positions = { { 0, { 0, 0, 0 } }, { 2, { 8, 0, 0 } } };
	track.Scales = { { 0, { 1, 1, 1 } }, { 2, { 5, 1, 1 } } };
	track.Rotations = { { 0, { 0, 0, 0, 1 } }, { 2, { 0, 1, 0, 0 } } };
	const ANIM_TRANSFORM pose = AnimCalcSample(track, 0.5f, {});
	EXPECT_FLOAT_EQ(pose.Position.X, 2.0f);
	EXPECT_FLOAT_EQ(pose.Scale.X, 2.0f);
	EXPECT_NEAR(pose.Rotation.Y, 0.3826834f, 0.0001f);
	EXPECT_NEAR(pose.Rotation.W, 0.9238795f, 0.0001f);
}

// 切り替え時間の半分で半々になり、終わったら新しい姿勢だけを使う。
TEST(AnimCalcTest, BlendsOverTransitionDuration)
{
	EXPECT_FLOAT_EQ(AnimCalcBlendWeight(0.0f, 0.15f), 0.0f);
	EXPECT_FLOAT_EQ(AnimCalcBlendWeight(0.075f, 0.15f), 0.5f);
	EXPECT_FLOAT_EQ(AnimCalcBlendWeight(0.2f, 0.15f), 1.0f);
}
