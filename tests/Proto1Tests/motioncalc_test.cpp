#include <gtest/gtest.h>
#include "render/motioncalc.h"
#include <cmath>

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

// つかまれ中は、被弾直後・しびれ・吹っ飛びより優先してもがく。
TEST(MotionCalcTest, HeldReactionTakesPriority)
{
	const MOTION_HIT_REACTION reaction = MotionCalcHitReaction(true, true, true, 0.1f, 0.33f);
	EXPECT_TRUE(reaction.Active);
	EXPECT_EQ(SKINNED_CLIP::HitHeld, reaction.Clip);
	EXPECT_LT(reaction.ClipTime, 0.0f);
}

// 被弾直後はしびれより優先して経過秒の位置でのけぞり、時間が過ぎると吹っ飛びのループになる。
TEST(MotionCalcTest, FlinchUsesElapsedTimeThenLaunchedLoops)
{
	const MOTION_HIT_REACTION flinch = MotionCalcHitReaction(false, true, true, 0.1f, 0.33f);
	EXPECT_TRUE(flinch.Active);
	EXPECT_EQ(SKINNED_CLIP::HitFlinch, flinch.Clip);
	EXPECT_NEAR(flinch.ClipTime, 0.1f, 0.0001f);
	const MOTION_HIT_REACTION launched = MotionCalcHitReaction(false, false, true, 0.5f, 0.33f);
	EXPECT_TRUE(launched.Active);
	EXPECT_EQ(SKINNED_CLIP::HitLaunched, launched.Clip);
	EXPECT_LT(launched.ClipTime, 0.0f);
}

// のけぞりの時間を過ぎてもしびれていれば、吹っ飛びより優先してしびれをループする。
TEST(MotionCalcTest, StunReactionTakesPriorityOverLaunched)
{
	const MOTION_HIT_REACTION reaction = MotionCalcHitReaction(false, true, true, 0.5f, 0.33f);
	EXPECT_TRUE(reaction.Active);
	EXPECT_EQ(SKINNED_CLIP::HitStun, reaction.Clip);
	EXPECT_LT(reaction.ClipTime, 0.0f);
}

// やられの状態がなければ、通常の走り・ジャンプなどをそのまま使う。
TEST(MotionCalcTest, NoReactionKeepsLocomotion)
{
	EXPECT_FALSE(MotionCalcHitReaction(false, false, false, 1.0f, 0.33f).Active);
}

// 攻撃の最後の組は攻撃扱いのまま、やられは攻撃扱いにしない。
TEST(MotionCalcTest, HitReactionsAreNotAttackClips)
{
	EXPECT_TRUE(AnimCalcIsAttackClip(SKINNED_CLIP::KnuckleAirRushLeft));
	EXPECT_TRUE(AnimCalcIsAttackClip(SKINNED_CLIP::KnuckleAirRushRight));
	EXPECT_FALSE(AnimCalcIsAttackClip(SKINNED_CLIP::HitFlinch));
	EXPECT_FALSE(AnimCalcIsAttackClip(SKINNED_CLIP::HitLaunched));
	EXPECT_FALSE(AnimCalcIsAttackClip(SKINNED_CLIP::HitStun));
	EXPECT_FALSE(AnimCalcIsAttackClip(SKINNED_CLIP::HitHeld));
}

// 地面で斜め前へダッシュを始めると、1フレーム目から脚がダッシュの向きにそろい、経過秒の位置でダッシュのクリップを見せる。
TEST(MotionCalcTest, GroundDashFacesDashDirection)
{
	MotionCalc motion;
	motion.Reset(0.0f);
	MOTION_INPUT input = Moving(10.0f, 15.0f, 0.0f);
	input.DashElapsed = 0.1f;
	input.DashYaw = PI * 0.2f;
	StepFrames(motion, input, 1);
	EXPECT_EQ(SKINNED_CLIP::Dash, motion.GetClip());
	EXPECT_NEAR(motion.GetLegYaw(), PI * 0.2f, 0.001f);
	EXPECT_NEAR(motion.GetClipTime(), 0.1f, 0.0001f);
}

// 空中で始めたダッシュは空中のクリップになり、真後ろへのダッシュは脚がそちらへ向き直る（上半身のねじれは上限まで）。
TEST(MotionCalcTest, AirDashTurnsBodyBackward)
{
	MotionCalc motion;
	motion.Reset(0.0f);
	const MOTION_PARAMS params;
	MOTION_INPUT input = Moving(0.0f, -18.0f, 0.0f);
	input.OnGround = false;
	input.DashElapsed = 0.0f;
	input.DashAir = true;
	input.DashYaw = PI;
	StepFrames(motion, input, 1);
	EXPECT_EQ(SKINNED_CLIP::AirDash, motion.GetClip());
	EXPECT_NEAR(fabsf(motion.GetLegYaw()), PI, 0.001f);
	EXPECT_NEAR(fabsf(motion.GetTwist()), params.MaxTwist, 0.001f);
}

// ダッシュのクリップを見せる秒数を過ぎると、今までの走りに戻り、再生位置の指定もやめる。
TEST(MotionCalcTest, DashEndsAfterClipSeconds)
{
	const MOTION_PARAMS params;
	EXPECT_TRUE(MotionCalcDash(params.DashClipSeconds - 0.01f, false, 0.0f, params).Active);
	MotionCalc motion;
	motion.Reset(0.0f);
	MOTION_INPUT input = Moving(0.0f, 6.0f, 0.0f);
	input.DashElapsed = params.DashClipSeconds;
	StepFrames(motion, input, 1);
	EXPECT_EQ(SKINNED_CLIP::Run, motion.GetClip());
	EXPECT_LT(motion.GetClipTime(), 0.0f);
}

// 狙いから見て右へダッシュすると、体は狙いの向きのまま（ねじれなし）右の横ダッシュを見せる。左へは左の横ダッシュ。
TEST(MotionCalcTest, SideDashKeepsBodyOnAim)
{
	MotionCalc motion;
	motion.Reset(0.0f);
	MOTION_INPUT input = Moving(18.0f, 0.0f, 0.0f);
	input.DashElapsed = 0.1f;
	input.DashYaw = PI * 0.5f;
	StepFrames(motion, input, 1);
	EXPECT_EQ(SKINNED_CLIP::SideDashRight, motion.GetClip());
	EXPECT_NEAR(motion.GetLegYaw(), 0.0f, 0.001f);
	EXPECT_NEAR(motion.GetTwist(), 0.0f, 0.001f);
	const MOTION_PARAMS params;
	EXPECT_EQ(SKINNED_CLIP::SideDashLeft, MotionCalcDash(0.1f, true, -PI * 0.5f, params).Clip);
}
