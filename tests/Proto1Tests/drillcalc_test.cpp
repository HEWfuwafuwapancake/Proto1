#include <gtest/gtest.h>
#include "physics/drillcalc.h"
#include "physics/armcalc.h"

namespace Proto1Tests
{
	static constexpr float DRILL_EPSILON = 0.0001f;
	static constexpr DRILL_GRIP DRILL_TEST_GRIP{
		{ 0.311337f, -5.719436f, -5.081741f }, { 0.0f, -0.707106781f, 0.707106781f }, 0.08f
	};
	static constexpr DRILL_PARAMS DRILL_TEST_PARAMS{
		DRILL_TEST_GRIP, 12.0f, 18.060484f, 0.35f, 0.4f, 0.5f, 0.1f, 5, 0.3f
	};

	static TENTACLE_TARGET DrillTarget(float z, bool available = true)
	{
		return { { { 0, -0.4f, z }, { 0, 0.4f, z }, 0.5f }, available };
	}

	static DRILL_EVENT StepDrill(DrillCalc& arm, float dt, const TENTACLE_TARGET* targets = nullptr,
		int count = 0, bool trigger = false, bool cancel = false, VEC3 origin = {},
		VEC3 aim = { 0, 0, 1 }, bool mirrored = false)
	{
		return arm.Step(origin, aim, mirrored, trigger, cancel, dt, DRILL_TEST_PARAMS, targets, count);
	}

	static DrillCalc BoringDrill()
	{
		DrillCalc arm;
		arm.Reset();
		const TENTACLE_TARGET target = DrillTarget(2.0f);
		StepDrill(arm, 0.01f, &target, 1, true);
		return arm;
	}

	TEST(DrillCalcTest, Idle_OnlyTriggerStartsStrikeAndFixesHorizontalDirection)
	{
		DrillCalc arm;
		arm.Reset();
		StepDrill(arm, 0.01f);
		EXPECT_EQ((int)DRILL_IDLE, (int)arm.GetState());
		StepDrill(arm, 0.01f, nullptr, 0, true, false, {}, { 3, 20, 4 });
		EXPECT_EQ((int)DRILL_STRIKING, (int)arm.GetState());
		EXPECT_NEAR(0.6f, arm.GetDirection().X, DRILL_EPSILON);
		EXPECT_NEAR(0.0f, arm.GetDirection().Y, DRILL_EPSILON);
		EXPECT_NEAR(0.8f, arm.GetDirection().Z, DRILL_EPSILON);
		StepDrill(arm, 0.01f, nullptr, 0, false, false, {}, { -1, 0, 0 });
		EXPECT_NEAR(0.6f, arm.GetDirection().X, DRILL_EPSILON);
	}

	TEST(DrillCalcTest, Axis_RestPitchMakesBitHorizontalForwardAndBelowShoulder)
	{
		const VEC3 shoulder{ 4, 5, 6 };
		const VEC3 rest = ArmCalcPitchDirection({ 0, 0, 1 }, -0.785398163f);
		const DRILL_AXIS axis = DrillCalcAxis(shoulder, rest, DRILL_TEST_GRIP, false);
		EXPECT_NEAR(0.0f, axis.Direction.X, DRILL_EPSILON);
		EXPECT_NEAR(0.0f, axis.Direction.Y, DRILL_EPSILON);
		EXPECT_NEAR(1.0f, axis.Direction.Z, DRILL_EPSILON);
		EXPECT_NEAR(4.0f - 0.02490696f, axis.Rear.X, DRILL_EPSILON);
		EXPECT_NEAR(5.0f - 0.6110248f, axis.Rear.Y, DRILL_EPSILON);
		EXPECT_NEAR(6.0f + 0.0360735f, axis.Rear.Z, DRILL_EPSILON);
	}

	TEST(DrillCalcTest, ThrustArmDirection_MakesAxisMatchHorizontalAimForBothHands)
	{
		for (const VEC3 aim : { VEC3{ 3, 20, 4 }, VEC3{ -1, 0, 0 }, VEC3{} }) {
			const VEC3 expected = TentacleCalcAim({}, {}, false, aim);
			const VEC3 armDirection = DrillCalcThrustArmDirection(aim, DRILL_TEST_GRIP);
			EXPECT_NEAR(1.0f, Vec3Length(armDirection), DRILL_EPSILON);
			for (bool mirrored : { false, true }) {
				const DRILL_AXIS axis = DrillCalcAxis({ 4, 5, 6 }, armDirection, DRILL_TEST_GRIP, mirrored);
				EXPECT_NEAR(expected.X, axis.Direction.X, DRILL_EPSILON);
				EXPECT_NEAR(expected.Y, axis.Direction.Y, DRILL_EPSILON);
				EXPECT_NEAR(expected.Z, axis.Direction.Z, DRILL_EPSILON);
				EXPECT_NEAR(1.0f, Vec3Length(axis.Direction), DRILL_EPSILON);
			}
		}
		DRILL_GRIP grip = DRILL_TEST_GRIP;
		grip.AxisDirection = { 0, -0.866025404f, 0.5f }; // 握りが 30° なら腕は -30°。
		const VEC3 direction = DrillCalcThrustArmDirection({ 0, 0, 1 }, grip);
		EXPECT_NEAR(-0.5f, direction.Y, DRILL_EPSILON);
		EXPECT_NEAR(0.866025404f, direction.Z, DRILL_EPSILON);
	}

	TEST(DrillCalcTest, Axis_MirroringReversesOnlyRearLateralOffset)
	{
		const VEC3 shoulder{ 4, 5, 6 };
		const VEC3 direction = DrillCalcThrustArmDirection({ 0, 0, 1 }, DRILL_TEST_GRIP);
		const DRILL_AXIS left = DrillCalcAxis(shoulder, direction, DRILL_TEST_GRIP, false);
		const DRILL_AXIS right = DrillCalcAxis(shoulder, direction, DRILL_TEST_GRIP, true);
		EXPECT_NEAR(-0.02490696f, left.Rear.X - shoulder.X, DRILL_EPSILON);
		EXPECT_NEAR(0.02490696f, right.Rear.X - shoulder.X, DRILL_EPSILON);
		EXPECT_NEAR(left.Rear.Y, right.Rear.Y, DRILL_EPSILON);
		EXPECT_NEAR(left.Rear.Z, right.Rear.Z, DRILL_EPSILON);
		EXPECT_NEAR(left.Direction.X, right.Direction.X, DRILL_EPSILON);
		EXPECT_NEAR(left.Direction.Y, right.Direction.Y, DRILL_EPSILON);
		EXPECT_NEAR(left.Direction.Z, right.Direction.Z, DRILL_EPSILON);
	}

	TEST(DrillCalcTest, Bit_UsesHitStartAndEndOnAxisAndFollowsShoulder)
	{
		const VEC3 shoulder{ 4, 5, 6 };
		const VEC3 direction = DrillCalcThrustArmDirection({ 0, 0, 1 }, DRILL_TEST_GRIP);
		const CAPSULE bit = DrillCalcBit(shoulder, direction, false, DRILL_TEST_PARAMS);
		EXPECT_NEAR(4.0f - 0.02490696f, bit.Start.X, DRILL_EPSILON);
		EXPECT_NEAR(5.0f - 0.6110248f, bit.Start.Y, DRILL_EPSILON);
		EXPECT_NEAR(6.0f + 0.0360735f + 12.0f * 0.08f, bit.Start.Z, DRILL_EPSILON);
		EXPECT_NEAR(bit.Start.X, bit.End.X, DRILL_EPSILON);
		EXPECT_NEAR(bit.Start.Y, bit.End.Y, DRILL_EPSILON);
		EXPECT_NEAR(6.0f + 0.0360735f + 18.060484f * 0.08f, bit.End.Z, DRILL_EPSILON);
		EXPECT_NEAR(0.35f, bit.Radius, DRILL_EPSILON);
		// 腕が水平な途中の姿と鏡写しでも、軸から外れない。
		for (bool mirrored : { false, true }) {
			const DRILL_AXIS axis = DrillCalcAxis(shoulder, { 0.6f, 0, 0.8f }, DRILL_TEST_GRIP, mirrored);
			const CAPSULE tilted = DrillCalcBit(shoulder, { 0.6f, 0, 0.8f }, mirrored, DRILL_TEST_PARAMS);
			const VEC3 startOffset = Vec3Sub(tilted.Start, axis.Rear);
			const VEC3 endOffset = Vec3Sub(tilted.End, axis.Rear);
			EXPECT_NEAR(12.0f * 0.08f, Vec3Length(startOffset), DRILL_EPSILON);
			EXPECT_NEAR(18.060484f * 0.08f, Vec3Length(endOffset), DRILL_EPSILON);
			EXPECT_NEAR(Vec3Length(startOffset), Vec3Dot(startOffset, axis.Direction), DRILL_EPSILON);
			EXPECT_NEAR(Vec3Length(endOffset), Vec3Dot(endOffset, axis.Direction), DRILL_EPSILON);
		}
	}

	TEST(DrillCalcTest, Strike_HitsNearEnemyButNotFarEnemyOrShoulder)
	{
		// 軸後端は肩より約 0.61 下、先端は前へ約 1.48。半径も含めて比べる。
		for (float distance : { 1.48f, 2.5f, 0.0f }) {
			DrillCalc arm;
			arm.Reset();
			const TENTACLE_TARGET target = DrillTarget(distance);
			const DRILL_EVENT e = StepDrill(arm, 0.01f, &target, 1, true);
			EXPECT_EQ((int)(distance == 1.48f ? DRILL_BORING : DRILL_STRIKING), (int)arm.GetState());
			EXPECT_EQ(0, e.DamageTicks);
			EXPECT_FALSE(e.Push);
		}
	}

	TEST(DrillCalcTest, Strike_UsesShoulderAndMirroredGripForCollision)
	{
		// 左の判定の横端にだけ重なる小さな敵。肩の移動と右手の X 反転を確認する。
		const VEC3 center{ 4.0f - 0.37f, 5.0f - 0.6110248f, 6.0f + 1.2f };
		const TENTACLE_TARGET target{ { center, center, 0.01f }, true };
		for (bool mirrored : { false, true }) {
			DrillCalc arm;
			arm.Reset();
			StepDrill(arm, 0.01f, &target, 1, true, false, { 4, 5, 6 }, { 0, 0, 1 }, mirrored);
			EXPECT_EQ((int)(mirrored ? DRILL_STRIKING : DRILL_BORING), (int)arm.GetState());
		}
	}

	TEST(DrillCalcTest, Strike_SkipsUnavailableAndChoosesNearestOverlappingEnemy)
	{
		DrillCalc arm;
		arm.Reset();
		const TENTACLE_TARGET targets[]{ DrillTarget(1.1f, false), DrillTarget(1.6f), DrillTarget(1.3f) };
		StepDrill(arm, 0.01f, targets, 3, true);
		EXPECT_EQ(2, arm.GetTarget());
		EXPECT_EQ((int)DRILL_BORING, (int)arm.GetState());
	}

	TEST(DrillCalcTest, Miss_StrikesForPointFourThenRestsForPointThree)
	{
		DrillCalc arm;
		arm.Reset();
		StepDrill(arm, 0.39f, nullptr, 0, true);
		EXPECT_EQ((int)DRILL_STRIKING, (int)arm.GetState());
		const DRILL_EVENT e = StepDrill(arm, 0.01f);
		EXPECT_EQ((int)DRILL_COOLDOWN, (int)arm.GetState());
		EXPECT_FALSE(e.Push);
		StepDrill(arm, 0.29f);
		EXPECT_EQ((int)DRILL_COOLDOWN, (int)arm.GetState());
		StepDrill(arm, 0.01f);
		EXPECT_EQ((int)DRILL_IDLE, (int)arm.GetState());
		StepDrill(arm, 0.01f, nullptr, 0, true);
		EXPECT_EQ((int)DRILL_STRIKING, (int)arm.GetState());
	}

	TEST(DrillCalcTest, Boring_OneTickEveryPointOneAndFifthTickPushesOnce)
	{
		DrillCalc arm = BoringDrill();
		const TENTACLE_TARGET target = DrillTarget(2.0f);
		EXPECT_NEAR(0.0f, arm.GetTime(), DRILL_EPSILON);
		EXPECT_EQ(0, StepDrill(arm, 0.09f, &target, 1).DamageTicks);
		for (int tick = 1; tick <= 5; ++tick) {
			const DRILL_EVENT e = StepDrill(arm, tick == 1 ? 0.01f : 0.1f, &target, 1);
			EXPECT_EQ(1, e.DamageTicks);
			EXPECT_EQ(0, e.Target);
			EXPECT_EQ(tick == 5, e.Push);
		}
		EXPECT_EQ(5, arm.GetTicks());
		EXPECT_EQ((int)DRILL_COOLDOWN, (int)arm.GetState());
		const DRILL_EVENT after = StepDrill(arm, 0.01f, &target, 1);
		EXPECT_EQ(0, after.DamageTicks);
		EXPECT_FALSE(after.Push);
	}

	TEST(DrillCalcTest, Boring_LongFramesCollectTicksButNeverExceedFive)
	{
		DrillCalc arm = BoringDrill();
		const TENTACLE_TARGET target = DrillTarget(2.0f);
		const DRILL_EVENT first = StepDrill(arm, 0.35f, &target, 1);
		EXPECT_EQ(3, first.DamageTicks);
		EXPECT_FALSE(first.Push);
		const DRILL_EVENT last = StepDrill(arm, 8.0f, &target, 1);
		EXPECT_EQ(2, last.DamageTicks);
		EXPECT_TRUE(last.Push);
		DrillCalc all = BoringDrill();
		EXPECT_EQ(5, StepDrill(all, 8.0f, &target, 1).DamageTicks);
	}

	TEST(DrillCalcTest, Boring_LeavingDisappearingOrBeingGrabbedCancelsBeforeTick)
	{
		for (int reason = 0; reason < 3; ++reason) {
			DrillCalc arm = BoringDrill();
			const TENTACLE_TARGET target = DrillTarget(reason == 0 ? 10.0f : 2.0f, reason != 2);
			const DRILL_EVENT e = StepDrill(arm, 0.5f, reason == 1 ? nullptr : &target, reason == 1 ? 0 : 1);
			EXPECT_EQ((int)DRILL_COOLDOWN, (int)arm.GetState());
			EXPECT_EQ(-1, arm.GetTarget());
			EXPECT_EQ(0, e.DamageTicks);
			EXPECT_FALSE(e.Push);
		}
	}

	TEST(DrillCalcTest, Boring_MovingShoulderAwayCancelsEvenWithAnotherNearbyEnemy)
	{
		DrillCalc arm = BoringDrill();
		const TENTACLE_TARGET targets[]{ DrillTarget(2.0f), DrillTarget(12.0f) };
		const DRILL_EVENT e = StepDrill(arm, 0.5f, targets, 2, false, false, { 0, 0, 10 });
		EXPECT_EQ((int)DRILL_COOLDOWN, (int)arm.GetState());
		EXPECT_EQ(0, e.DamageTicks);
		EXPECT_FALSE(e.Push);
	}

	TEST(DrillCalcTest, Respawn_CancelsStrikeAndBoringWithoutPushEvenWhenTimeStopped)
	{
		const TENTACLE_TARGET target = DrillTarget(2.0f);
		for (DRILL_STATE state : { DRILL_STRIKING, DRILL_BORING }) {
			DrillCalc arm = BoringDrill();
			arm = { state, arm.GetDirection(), arm.GetTime(), arm.GetTarget(), arm.GetTicks() };
			const DRILL_EVENT e = StepDrill(arm, 0.0f, &target, 1, true, true);
			EXPECT_EQ((int)DRILL_COOLDOWN, (int)arm.GetState());
			EXPECT_EQ(0, e.DamageTicks);
			EXPECT_FALSE(e.Push);
		}
	}

	TEST(DrillCalcTest, PushDirection_IsHorizontalPlusSmallUpAndUnitLength)
	{
		const VEC3 push = DrillCalcPushDirection({ 3, -100, 4 }, 0.15f);
		const float length = sqrtf(1.0f + 0.15f * 0.15f);
		EXPECT_NEAR(0.6f / length, push.X, DRILL_EPSILON);
		EXPECT_NEAR(0.15f / length, push.Y, DRILL_EPSILON);
		EXPECT_NEAR(0.8f / length, push.Z, DRILL_EPSILON);
		EXPECT_NEAR(1.0f, Vec3Length(push), DRILL_EPSILON);
		const VEC3 fallback = DrillCalcPushDirection({ 0, 5, 0 }, 0.15f);
		EXPECT_NEAR(0.0f, fallback.X, DRILL_EPSILON);
		EXPECT_NEAR(1.0f / length, fallback.Z, DRILL_EPSILON);
	}
}
