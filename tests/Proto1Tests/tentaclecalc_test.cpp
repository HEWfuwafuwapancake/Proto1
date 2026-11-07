#include <gtest/gtest.h>
#include "physics/tentaclecalc.h"
#include "physics/knockbackcalc.h"
#include <initializer_list>
namespace Proto1Tests
{
	static const STAGE stage{ 0, 0, 15, 15, 0, 2, -15 };
	static const TENTACLE_PARAMS params{ 8, 24, 30, 3, 0.18f };
	static const VEC3 origin{ 0, 1.35f, 0 };
	static const VEC3 aim{ 0, 0, 1 };
	static const VEC3 hold{ 1, 1.35f, 2 };
	static TENTACLE_TARGET Target(float z)
	{
		return { { { 0, 0.5f, z }, { 0, 1.3f, z }, 0.5f }, true };
	}
	static TENTACLE_EVENT Step(TentacleCalc& arm, bool press, float dt, TENTACLE_TARGET* targets = nullptr, int count = 0)
	{
		return arm.Step(origin, aim, hold, press, false, dt, params, targets, count, stage);
	}
	TEST(TentacleCalcTest, Wiggle_ExactEndpointsAndZeroAmplitude)
	{
		const VEC3 a{ 1, 2, 3 }, b{ 5, 8, 11 };
		const VEC3 first = TentacleCalcWigglePoint(a, b, 0, 2, 0.25f, 1.5f);
		const VEC3 last = TentacleCalcWigglePoint(a, b, 1, 2, 0.25f, 1.5f);
		EXPECT_EQ(0.0f, Vec3Length(Vec3Sub(first, a)));
		EXPECT_EQ(0.0f, Vec3Length(Vec3Sub(last, b)));
		const VEC3 middle = TentacleCalcWigglePoint(a, b, 0.25f, 2, 0, 1.5f);
		EXPECT_NEAR(0.0f, Vec3Length(Vec3Sub(middle, { 2, 3.5f, 5 })), 0.0001f);
	}

	TEST(TentacleCalcTest, Wiggle_FormulaAndPerpendicularOffset)
	{
		// +Z の腕の横は -X。s=0.5、波=0.5、位相=0 で横揺れが最大。
		const VEC3 p = TentacleCalcWigglePoint({}, { 0, 0, 4 }, 0.5f, 0, 0.25f, 0.5f);
		EXPECT_NEAR(-0.25f, p.X, 0.0001f);
		EXPECT_NEAR(0.0f, p.Y, 0.0001f);
		EXPECT_NEAR(2.0f, p.Z, 0.0001f);
		const VEC3 vertical = TentacleCalcWigglePoint({}, { 0, 0, 4 }, 0.5f, 0, 0.25f, 0);
		EXPECT_NEAR(0.125f, vertical.Y, 0.0001f);
		const VEC3 end{ 3, 4, 5 };
		const VEC3 curved = TentacleCalcWigglePoint({}, end, 0.3f, 1, 0.25f, 1.5f);
		EXPECT_NEAR(0.0f, Vec3Dot(Vec3Sub(curved, Vec3Scale(end, 0.3f)), end), 0.0001f);
	}

	// 待機の形は肩から外へ出てから垂れ、先は「外へ bend 分＋垂れる向きへ残り」の位置になる。
	// 垂れる向きを後ろへ倒すと（走ってなびく）、先は肩より後ろへ流れる。
	TEST(TentacleCalcTest, Droop_HangsDownAndTrailsBack)
	{
		const VEC3 origin{ 0, 2, 0 };
		const VEC3 outward{ 1, 0, 0 };
		const VEC3 tip = TentacleCalcDroopPoint(origin, outward, { 0, -1, 0 }, 1.0f, 0.3f, 1.0f, { 0, 0, 1 }, 0.0f, 0.0f, 1.0f);
		EXPECT_NEAR(0.3f, tip.X, 0.0001f);
		EXPECT_NEAR(1.3f, tip.Y, 0.0001f);
		const VEC3 middle = TentacleCalcDroopPoint(origin, outward, { 0, -1, 0 }, 1.0f, 0.3f, 0.5f, { 0, 0, 1 }, 0.0f, 0.0f, 1.0f);
		EXPECT_NEAR(0.225f, middle.X, 0.0001f); // 0.5*0.3 + 0.25*0.3
		EXPECT_NEAR(1.825f, middle.Y, 0.0001f); // 0.25*2 + 0.5*2 + 0.25*1.3
		const VEC3 trailed = TentacleCalcDroopPoint(origin, outward, { 0, -0.6f, -0.8f }, 1.0f, 0.3f, 1.0f, { 0, 0, 1 }, 0.0f, 0.0f, 1.0f);
		EXPECT_NEAR(-0.56f, trailed.Z, 0.0001f);
		// 横揺れは先で amplitude × sin(phase - 2π × waves)。
		const VEC3 swayed = TentacleCalcDroopPoint(origin, outward, { 0, -1, 0 }, 1.0f, 0.3f, 1.0f, { 0, 0, 1 }, 1.5707963f, 0.1f, 1.0f);
		EXPECT_NEAR(0.1f, swayed.Z, 0.0001f);
	}

	TEST(TentacleCalcTest, PieceAxes_OrthonormalAndForwardIncludingDegenerateCases)
	{
		for (VEC3 end : { VEC3{ 2, 3, 4 }, VEC3{ 0, 4, 0 }, VEC3{ 0, -4, 0 }, VEC3{} }) {
			for (VEC3 down : { VEC3{ 0, -1, 0 }, end, VEC3{} }) {
				const auto axes = TentacleCalcPieceAxes({}, end, down);
				EXPECT_NEAR(1.0f, Vec3Length(axes.X), 0.0001f);
				EXPECT_NEAR(1.0f, Vec3Length(axes.Y), 0.0001f);
				EXPECT_NEAR(1.0f, Vec3Length(axes.Z), 0.0001f);
				EXPECT_NEAR(0.0f, Vec3Dot(axes.X, axes.Y), 0.0001f);
				EXPECT_NEAR(0.0f, Vec3Dot(axes.Y, axes.Z), 0.0001f);
				EXPECT_NEAR(0.0f, Vec3Dot(axes.Z, axes.X), 0.0001f);
				const VEC3 direction = Vec3Length(end) > 0 ? Vec3Scale(end, 1.0f / Vec3Length(end)) : VEC3{ 0, 0, 1 };
				EXPECT_NEAR(-1.0f, Vec3Dot(axes.Y, direction), 0.0001f);
				const VEC3 cross{ axes.X.Y * axes.Y.Z - axes.X.Z * axes.Y.Y,
					axes.X.Z * axes.Y.X - axes.X.X * axes.Y.Z, axes.X.X * axes.Y.Y - axes.X.Y * axes.Y.X };
				EXPECT_NEAR(1.0f, Vec3Dot(cross, axes.Z), 0.0001f);
			}
		}
	}

	TEST(TentacleCalcTest, PieceAxes_SuckersFaceProjectedDown)
	{
		const VEC3 end{ 0, 3, 4 };
		const auto axes = TentacleCalcPieceAxes({}, end, { 0, -1, 0 });
		// -Z が、下を腕に直角な面へ映した向き (0, -0.8, 0.6) になる
		EXPECT_NEAR(0.0f, axes.Z.X, 0.0001f);
		EXPECT_NEAR(0.8f, axes.Z.Y, 0.0001f);
		EXPECT_NEAR(-0.6f, axes.Z.Z, 0.0001f);
	}

	TEST(TentacleCalcTest, Aim_TargetGivesHorizontalUnitDirection)
	{
		const VEC3 direction = TentacleCalcAim({ 2, 1, -3 }, { 5, 1, 1 }, true, aim);
		EXPECT_NEAR(0.6f, direction.X, 0.0001f);
		EXPECT_NEAR(0.0f, direction.Y, 0.0001f);
		EXPECT_NEAR(0.8f, direction.Z, 0.0001f);
		EXPECT_NEAR(1.0f, Vec3Length(direction), 0.0001f);
	}

	TEST(TentacleCalcTest, Aim_IgnoresTargetHeightAndJumpHeight)
	{
		for (float height : { -20.0f, 20.0f }) {
			const VEC3 direction = TentacleCalcAim({ 2, -height, -3 }, { 5, height, 1 }, true, aim);
			EXPECT_NEAR(0.6f, direction.X, 0.0001f);
			EXPECT_NEAR(0.0f, direction.Y, 0.0001f);
			EXPECT_NEAR(0.8f, direction.Z, 0.0001f);
		}
	}

	TEST(TentacleCalcTest, Aim_NoTargetUsesFallback)
	{
		const VEC3 direction = TentacleCalcAim(origin, { 3, 1, 4 }, false, { -1, 0, 0 });
		EXPECT_NEAR(-1.0f, direction.X, 0.0001f);
		EXPECT_NEAR(0.0f, direction.Y, 0.0001f);
		EXPECT_NEAR(0.0f, direction.Z, 0.0001f);
	}

	TEST(TentacleCalcTest, Aim_DistantTargetKeepsInitialDirectionAndMaximumLength)
	{
		TentacleCalc arm; arm.Reset();
		const VEC3 initial = TentacleCalcAim(origin, { 30, 10, 40 }, true, aim);
		arm.Step(origin, initial, hold, true, false, 0.05f, params, nullptr, 0, stage);
		const VEC3 moved = TentacleCalcAim(origin, { -40, -10, 30 }, true, aim);
		arm.Step(origin, moved, hold, false, false, 0.05f, params, nullptr, 0, stage);
		EXPECT_NEAR(1.44f, arm.GetTip().X, 0.0001f);
		EXPECT_NEAR(origin.Y, arm.GetTip().Y, 0.0001f);
		EXPECT_NEAR(1.92f, arm.GetTip().Z, 0.0001f);
		arm.Step(origin, moved, hold, false, false, 1, params, nullptr, 0, stage);
		EXPECT_NEAR(4.8f, arm.GetTip().X, 0.0001f);
		EXPECT_NEAR(6.4f, arm.GetTip().Z, 0.0001f);
		EXPECT_NEAR(8.0f, arm.GetLength(), 0.0001f);
		EXPECT_EQ((int)TENTACLE_RETURNING, (int)arm.GetState());
	}

	TEST(TentacleCalcTest, Miss_ReturnsAndCanExtendAgain)
	{
		TentacleCalc arm; arm.Reset();
		Step(arm, true, 1);
		EXPECT_EQ((int)TENTACLE_RETURNING, (int)arm.GetState());
		EXPECT_NEAR(8.0f, arm.GetTip().Z, 0.0001f);
		Step(arm, true, 1); // 戻る途中の連打を予約しない
		EXPECT_EQ((int)TENTACLE_IDLE, (int)arm.GetState());
		Step(arm, false, 0.1f);
		EXPECT_EQ((int)TENTACLE_IDLE, (int)arm.GetState());
		Step(arm, true, 0.1f);
		EXPECT_EQ((int)TENTACLE_EXTENDING, (int)arm.GetState());
	}

	TEST(TentacleCalcTest, LongStep_ChoosesNearTargetRegardlessOfArrayOrder)
	{
		TentacleCalc arm; arm.Reset();
		TENTACLE_TARGET targets[]{ Target(6), Target(3) };
		const auto event = Step(arm, true, 1, targets, 2);
		EXPECT_EQ(1, event.Grabbed);
		EXPECT_TRUE(arm.GetHits().Contains(1));
		EXPECT_FALSE(event.Throw);
	}

	TEST(TentacleCalcTest, Grab_PullsTowardHandAndThrowsOnlyOnce)
	{
		TentacleCalc arm; arm.Reset();
		TENTACLE_TARGET target = Target(6);
		Step(arm, true, 0.3f, &target, 1);
		target.Available = false; // 自分が保持中でも引き寄せる
		Step(arm, false, 0.1f, &target, 1);
		EXPECT_TRUE(arm.GetTip().Z < 6 && arm.GetTip().Z > 2);
		const auto event = Step(arm, true, 0.01f, &target, 1);
		EXPECT_EQ(0, event.Released);
		EXPECT_TRUE(event.Throw);
		EXPECT_EQ(-1, arm.GetTarget());
		EXPECT_EQ(-1, Step(arm, true, 0.01f, &target, 1).Released);
	}

	TEST(TentacleCalcTest, Timeout_ReleasesWithoutDamage)
	{
		TentacleCalc arm; arm.Reset();
		TENTACLE_TARGET target = Target(3);
		Step(arm, true, 0.2f, &target, 1);
		const auto event = Step(arm, false, 3, &target, 1);
		EXPECT_EQ(0, event.Released);
		EXPECT_FALSE(event.Throw);
	}

	TEST(TentacleCalcTest, TwoArms_CannotGrabSameEnemy)
	{
		TentacleCalc left, right; left.Reset(); right.Reset();
		TENTACLE_TARGET targets[]{ Target(3), Target(6) };
		const auto first = Step(left, true, 1, targets, 2);
		targets[first.Grabbed].Available = false;
		EXPECT_EQ(1, Step(right, true, 1, targets, 2).Grabbed);
		EXPECT_EQ(0, left.GetTarget());
	}

	TEST(TentacleCalcTest, UnavailableEnemy_IsIgnored)
	{
		TentacleCalc arm; arm.Reset();
		TENTACLE_TARGET target = Target(3); target.Available = false;
		EXPECT_EQ(-1, Step(arm, true, 1, &target, 1).Grabbed);
	}

	TEST(TentacleCalcTest, Cancel_ReleasesAtOldPositionEvenWhenTimeStopped)
	{
		TentacleCalc arm; arm.Reset();
		TENTACLE_TARGET target = Target(3);
		Step(arm, true, 0.2f, &target, 1);
		const auto event = arm.Step({ 100, 1, 100 }, aim, hold, true, true, 0,
			params, &target, 1, stage);
		EXPECT_EQ(0, event.Released);
		EXPECT_FALSE(event.Throw);
		EXPECT_EQ(-1, arm.GetTarget());
		EXPECT_EQ((int)TENTACLE_IDLE, (int)arm.GetState());
	}

	TEST(TentacleCalcTest, Direction_RemainsFixedWhileExtending)
	{
		TentacleCalc arm; arm.Reset();
		Step(arm, true, 0.05f);
		arm.Step(origin, { 1, 0, 0 }, hold, true, false, 0.05f, params, nullptr, 0, stage);
		EXPECT_NEAR(0.0f, arm.GetTip().X, 0.0001f);
		EXPECT_NEAR(2.4f, arm.GetTip().Z, 0.0001f);
	}

	TEST(TentacleCalcTest, Stage_BlocksDownwardReach)
	{
		TentacleCalc arm; arm.Reset();
		TENTACLE_TARGET target{ { { 0, -4, 0 }, { 0, -3, 0 }, 0.5f }, true };
		const auto event = arm.Step(origin, { 0, -1, 0 }, hold, true, false, 1,
			params, &target, 1, stage);
		EXPECT_EQ(-1, event.Grabbed);
		EXPECT_TRUE(arm.GetTip().Y >= params.Radius);
		EXPECT_EQ((int)TENTACLE_RETURNING, (int)arm.GetState());
	}

	TEST(TentacleCalcTest, HeldBody_DoesNotPassThroughPlatform)
	{
		const VEC3 clipped = TentacleCalcClip({ 0, 2, 0 }, { 0, -5, 0 }, stage, 0.5f, 0.9f);
		EXPECT_TRUE(clipped.Y >= 0.9f && clipped.Y < 0.91f);
		const VEC3 outward = TentacleCalcClip({ 0, 0.9f, 0 }, { 0, 2, 0 }, stage, 0.5f, 0.9f);
		EXPECT_NEAR(2.0f, outward.Y, 0.0001f);
		const VEC3 side = TentacleCalcClip({ 17, -1, 0 }, { 0, -1, 0 }, stage, 0.5f, 0.9f);
		EXPECT_TRUE(side.X >= 15.5f);
	}

	TEST(TentacleCalcTest, ExtendRatio_GrowsWhileExtending)
	{
		TentacleCalc arm{};
		arm.Reset();
		EXPECT_EQ(0.0f, arm.GetExtendRatio(params));
		Step(arm, true, 0.1f);
		EXPECT_TRUE(arm.GetState() == TENTACLE_EXTENDING);
		const float first = arm.GetExtendRatio(params);
		EXPECT_NEAR(arm.GetLength() / params.MaxLength, first, 0.0001f);
		EXPECT_TRUE(first > 0.0f);
		Step(arm, false, 0.1f);
		EXPECT_TRUE(arm.GetExtendRatio(params) > first);
	}

	TEST(TentacleCalcTest, DirectedThrow_UsesPitchAndNormalizesSpeed)
	{
		const VEC3 down = KnockbackCalcDirectedVelocity({ 0, -3, 4 }, 10);
		EXPECT_NEAR(-6.0f, down.Y, 0.0001f);
		EXPECT_NEAR(8.0f, down.Z, 0.0001f);
		EXPECT_NEAR(10.0f, Vec3Length(down), 0.0001f);
		EXPECT_NEAR(10.0f, KnockbackCalcDirectedVelocity({ 0, 2, 0 }, 10).Y, 0.0001f);
		EXPECT_NEAR(10.0f, KnockbackCalcDirectedVelocity({}, 10).Y, 0.0001f);
	}
}
