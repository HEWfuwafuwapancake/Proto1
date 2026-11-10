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

	static const STAGE_GRIP gripAhead{ { 0, 4, 10 }, { 0, 11, 3, 3, 4, 0.6f } };
	static const TENTACLE_PULL_PARAMS pullParams{ 20, 0.8f, 1.5f };
	static const float PULL_DT = 1.0f / 60.0f;

	static void ReachGrip(TentacleCalc& arm)
	{
		const VEC3 toGrip = Vec3Sub(gripAhead.Point, origin);
		arm.Step(origin, toGrip, hold, true, false, PULL_DT, params, nullptr, 0, stage, &gripAhead);
		for (int i = 0; i < 120 && arm.GetState() == TENTACLE_EXTENDING; ++i) {
			arm.Step(origin, toGrip, hold, false, false, PULL_DT, params, nullptr, 0, stage);
		}
	}

	// ふちへ向かう腕は、普通の長さ（8）より遠くても届いて付く
	TEST(TentacleCalcTest, Grip_ReachesBeyondNormalLengthAndAnchors)
	{
		TentacleCalc arm; arm.Reset();
		ReachGrip(arm);
		EXPECT_EQ((int)TENTACLE_PULLING, (int)arm.GetState());
		EXPECT_NEAR(0.0f, Vec3Length(Vec3Sub(arm.GetTip(), gripAhead.Point)), 0.0001f);
		EXPECT_NEAR(Vec3Length(Vec3Sub(gripAhead.Point, origin)), arm.GetLength(), 0.001f);
		EXPECT_NEAR(11.0f, arm.GetSurface().CenterZ, 0.0001f);
	}

	// ふちへ向かう途中に敵がいたら、敵をつかむ
	TEST(TentacleCalcTest, Grip_EnemyInThePathIsGrabbed)
	{
		TentacleCalc arm; arm.Reset();
		TENTACLE_TARGET target{ { { 0, 1.5f, 5 }, { 0, 2.5f, 5 }, 0.5f }, true };
		const VEC3 toGrip = Vec3Sub(gripAhead.Point, origin);
		const auto event = arm.Step(origin, toGrip, hold, true, false, 1, params, &target, 1, stage, &gripAhead);
		EXPECT_EQ(0, event.Grabbed);
		EXPECT_EQ((int)TENTACLE_HOLDING, (int)arm.GetState());
	}

	// 浮島の横のふちに当たると付く（上の面に当たると戻るのは Stage_BlocksDownwardReach）
	TEST(TentacleCalcTest, SideOfMainIsland_Anchors)
	{
		TentacleCalc arm; arm.Reset();
		const VEC3 outside{ 0, -1, -18 };
		const auto event = arm.Step(outside, { 0, 0, 1 }, hold, true, false, 1, params, nullptr, 0, stage);
		EXPECT_TRUE(event.Anchored);
		EXPECT_EQ((int)TENTACLE_PULLING, (int)arm.GetState());
		EXPECT_NEAR(stage.TopY, arm.GetSurface().TopY, 0.0001f);
	}

	// ロックオン中（足場に付けない）は、浮島の横のふちに当たっても付かずに戻る
	TEST(TentacleCalcTest, SideOfMainIsland_ReturnsWhenCannotAnchor)
	{
		TentacleCalc arm; arm.Reset();
		const VEC3 outside{ 0, -1, -18 };
		const auto event = arm.Step(outside, { 0, 0, 1 }, hold, true, false, 1, params, nullptr, 0, stage, nullptr, false);
		EXPECT_FALSE(event.Anchored);
		EXPECT_EQ((int)TENTACLE_RETURNING, (int)arm.GetState());
	}

	// 足場に付けるのはフリーカメラの間と、ロックオンでも浮島の外で宙にいる復帰の間だけ
	TEST(TentacleCalcTest, CanAnchor_FreeLookOrRecoveringOffStage)
	{
		EXPECT_TRUE(TentacleCalcCanAnchor(true, true, stage, 0, 0));
		EXPECT_TRUE(TentacleCalcCanAnchor(false, false, stage, 20, 0));
		EXPECT_FALSE(TentacleCalcCanAnchor(false, false, stage, 0, 0));
		EXPECT_FALSE(TentacleCalcCanAnchor(false, true, stage, 20, 0));
	}

	// 引っぱりは少しずつ近づき、到着したら終わって腕が戻る
	TEST(TentacleCalcTest, Pull_MovesThenArrives)
	{
		TentacleCalc arm; arm.Reset();
		ReachGrip(arm);
		VEC3 center{ 0, 0.9f, 0 };
		const float before = Vec3Length(Vec3Sub(gripAhead.Point, center));
		TENTACLE_PULL pull = arm.StepPull(center, pullParams, PULL_DT);
		EXPECT_FALSE(pull.Ended);
		EXPECT_LT(Vec3Length(Vec3Sub(gripAhead.Point, center)), before);
		for (int i = 0; i < 120 && !pull.Ended; ++i) pull = arm.StepPull(center, pullParams, PULL_DT);
		EXPECT_TRUE(pull.Arrived);
		EXPECT_NEAR(pullParams.ArriveDistance, Vec3Length(Vec3Sub(gripAhead.Point, center)), 0.001f);
		EXPECT_EQ((int)TENTACLE_RETURNING, (int)arm.GetState());
	}

	// 届かないまま時間切れになったら、到着せずに終わる（1/60 秒刻み）
	TEST(TentacleCalcTest, Pull_TimesOutWithoutArriving)
	{
		TentacleCalc arm; arm.Reset();
		ReachGrip(arm);
		VEC3 center{ 0, 0.9f, 0 };
		const TENTACLE_PULL_PARAMS slow{ 1, 0.8f, 1.5f };
		TENTACLE_PULL pull{};
		int frames = 0;
		for (; frames < 200 && !pull.Ended; ++frames) pull = arm.StepPull(center, slow, PULL_DT);
		EXPECT_FALSE(pull.Arrived);
		EXPECT_TRUE(pull.Ended);
		EXPECT_NEAR(90, frames, 1);
	}

	// 引っぱり中にもう一度押すと、やめて腕を戻す
	TEST(TentacleCalcTest, Pull_SecondPressEnds)
	{
		TentacleCalc arm; arm.Reset();
		ReachGrip(arm);
		const auto event = arm.Step(origin, aim, hold, true, false, PULL_DT, params, nullptr, 0, stage);
		EXPECT_TRUE(event.PullEnded);
		EXPECT_EQ((int)TENTACLE_RETURNING, (int)arm.GetState());
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
