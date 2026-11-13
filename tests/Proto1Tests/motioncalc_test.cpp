#include <gtest/gtest.h>
#include "render/motioncalc.h"

namespace
{
	constexpr float PI = 3.14159265f;
	constexpr float DT = 1.0f / 60.0f;

	// 同じ状態で何フレームか進める。
	void StepFrames(MotionCalc& motion, const MOTION_INPUT& input, int frames)
	{
		const MOTION_PARAMS params;
		for (int i = 0; i < frames; ++i) motion.Step(input, params, DT);
	}

	MOTION_INPUT Moving(float vx, float vz, float aim)
	{
		MOTION_INPUT input;
		input.VelocityX = vx;
		input.VelocityZ = vz;
		input.AimYaw = aim;
		return input;
	}
}

// 狙いと同じ向きへ走ると、脚は進む向きにそろい、ねじれず、走りを速さに合わせて再生する。
TEST(MotionCalcTest, RunsForwardWithLegsTowardMovement)
{
	MotionCalc motion;
	motion.Reset(0.0f);
	StepFrames(motion, Moving(6.0f, 0.0f, PI * 0.5f), 30);
	EXPECT_NEAR(motion.GetLegYaw(), PI * 0.5f, 0.001f);
	EXPECT_NEAR(motion.GetTwist(), 0.0f, 0.001f);
	EXPECT_EQ(SKINNED_CLIP::Run, motion.GetClip());
	EXPECT_NEAR(motion.GetPlaySpeed(), 1.0f, 0.001f);
	EXPECT_GT(motion.GetLeanPitch(), 0.0f);
}

// 狙いを前に保ったまま横へ走ると、脚は横を向こうとするが、上半身のねじれは上限で止まる。
TEST(MotionCalcTest, StrafeLimitsTwist)
{
	MotionCalc motion;
	motion.Reset(0.0f);
	const MOTION_PARAMS params;
	StepFrames(motion, Moving(6.0f, 0.0f, 0.0f), 60);
	EXPECT_NEAR(motion.GetTwist(), -params.MaxTwist, 0.001f);
	EXPECT_NEAR(motion.GetLegYaw(), params.MaxTwist, 0.001f);
	EXPECT_EQ(SKINNED_CLIP::Run, motion.GetClip());
}

// 狙いと反対へ進むと後ろ走りになり、脚は狙いの向きのまま後ろへ傾く。
TEST(MotionCalcTest, MovingAwayFromAimRunsBackward)
{
	MotionCalc motion;
	motion.Reset(0.0f);
	StepFrames(motion, Moving(0.0f, -6.0f, 0.0f), 30);
	EXPECT_TRUE(motion.IsBackward());
	EXPECT_EQ(SKINNED_CLIP::RunBack, motion.GetClip());
	EXPECT_NEAR(motion.GetLegYaw(), 0.0f, 0.001f);
	EXPECT_LT(motion.GetLeanPitch(), 0.0f);
}

// 空中は上昇中がジャンプ・下降中が落下、着地のあと少しだけ着地を見せて待機に戻る。
TEST(MotionCalcTest, JumpFallLandThenIdle)
{
	MotionCalc motion;
	motion.Reset(0.0f);
	MOTION_INPUT input;
	input.OnGround = false;
	input.VelocityY = 5.0f;
	StepFrames(motion, input, 1);
	EXPECT_EQ(SKINNED_CLIP::Jump, motion.GetClip());
	input.VelocityY = -5.0f;
	StepFrames(motion, input, 1);
	EXPECT_EQ(SKINNED_CLIP::Fall, motion.GetClip());
	input.OnGround = true;
	input.VelocityY = 0.0f;
	StepFrames(motion, input, 1);
	EXPECT_EQ(SKINNED_CLIP::Land, motion.GetClip());
	StepFrames(motion, input, 30);
	EXPECT_EQ(SKINNED_CLIP::Idle, motion.GetClip());
}

// 走らない状態（吹っ飛び中）は、速さがあっても脚は狙いの向きで、空中なら落下だけにする。
TEST(MotionCalcTest, NoLocomotionKeepsLegsOnAim)
{
	MotionCalc motion;
	motion.Reset(0.0f);
	MOTION_INPUT input = Moving(6.0f, 0.0f, 1.0f);
	input.Locomotion = false;
	input.OnGround = false;
	input.VelocityY = 5.0f;
	StepFrames(motion, input, 1);
	EXPECT_NEAR(motion.GetLegYaw(), 1.0f, 0.001f);
	EXPECT_EQ(SKINNED_CLIP::Fall, motion.GetClip());
}
