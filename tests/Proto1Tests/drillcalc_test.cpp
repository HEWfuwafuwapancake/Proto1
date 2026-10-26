#include "CppUnitTest.h"
#include "physics/drillcalc.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace Proto1Tests
{
	static constexpr float DRILL_EPSILON = 0.0001f;
	static constexpr DRILL_PARAMS DRILL_TEST_PARAMS{ 1.2f, 2.65f, 0.35f, 0.4f, 0.5f, 0.1f, 5, 0.3f };

	static TENTACLE_TARGET DrillTarget(float z, bool available = true)
	{
		return { { { 0, -0.4f, z }, { 0, 0.4f, z }, 0.5f }, available };
	}

	static DRILL_EVENT StepDrill(DRILL& arm, float dt, const TENTACLE_TARGET* targets = nullptr,
		int count = 0, bool trigger = false, bool cancel = false, VEC3 origin = {}, VEC3 aim = { 0, 0, 1 })
	{
		return DrillCalcStep(arm, origin, aim, trigger, cancel, dt, DRILL_TEST_PARAMS, targets, count);
	}

	static DRILL BoringDrill()
	{
		DRILL arm;
		DrillCalcReset(arm);
		const TENTACLE_TARGET target = DrillTarget(2.0f);
		StepDrill(arm, 0.01f, &target, 1, true);
		return arm;
	}

	TEST_CLASS(DrillCalcTest)
	{
	public:
		TEST_METHOD(Reset_ClearsTargetTimeAndTicks)
		{
			DRILL arm{ DRILL_BORING, { 1, 2, 3 }, 0.4f, 2, 4 };
			DrillCalcReset(arm);
			Assert::AreEqual((int)DRILL_IDLE, (int)arm.State);
			Assert::AreEqual(-1, arm.Target);
			Assert::AreEqual(0, arm.Ticks);
			Assert::AreEqual(0.0f, arm.Time, DRILL_EPSILON);
			Assert::AreEqual(1.0f, arm.Direction.Z, DRILL_EPSILON);
		}

		TEST_METHOD(Idle_OnlyTriggerStartsStrikeAndFixesHorizontalDirection)
		{
			DRILL arm;
			DrillCalcReset(arm);
			StepDrill(arm, 0.01f);
			Assert::AreEqual((int)DRILL_IDLE, (int)arm.State);
			StepDrill(arm, 0.01f, nullptr, 0, true, false, {}, { 3, 20, 4 });
			Assert::AreEqual((int)DRILL_STRIKING, (int)arm.State);
			Assert::AreEqual(0.6f, arm.Direction.X, DRILL_EPSILON);
			Assert::AreEqual(0.0f, arm.Direction.Y, DRILL_EPSILON);
			Assert::AreEqual(0.8f, arm.Direction.Z, DRILL_EPSILON);
			StepDrill(arm, 0.01f, nullptr, 0, false, false, {}, { -1, 0, 0 });
			Assert::AreEqual(0.6f, arm.Direction.X, DRILL_EPSILON);
		}

		TEST_METHOD(Bit_UsesOnlyFixedBitSegmentAndFollowsShoulder)
		{
			const CAPSULE bit = DrillCalcBit({ 4, 5, 6 }, { 0, 8, 2 }, DRILL_TEST_PARAMS);
			Assert::AreEqual(4.0f, bit.Start.X, DRILL_EPSILON);
			Assert::AreEqual(5.0f, bit.Start.Y, DRILL_EPSILON);
			Assert::AreEqual(7.2f, bit.Start.Z, DRILL_EPSILON);
			Assert::AreEqual(8.65f, bit.End.Z, DRILL_EPSILON);
			Assert::AreEqual(0.35f, bit.Radius, DRILL_EPSILON);
			const CAPSULE fallback = DrillCalcBit({}, {}, DRILL_TEST_PARAMS);
			Assert::AreEqual(2.65f, fallback.End.Z, DRILL_EPSILON);
		}

		TEST_METHOD(Strike_HitsNearEnemyButNotFarEnemyOrShoulder)
		{
			// 距離はカプセルの半径も含む。中心 3.6 なら、ビット半径 0.35＋体半径 0.5 の外。
			for (float distance : { 2.65f, 3.6f, 0.0f }) {
				DRILL arm;
				DrillCalcReset(arm);
				const TENTACLE_TARGET target = DrillTarget(distance);
				const DRILL_EVENT e = StepDrill(arm, 0.01f, &target, 1, true);
				Assert::AreEqual((int)(distance == 2.65f ? DRILL_BORING : DRILL_STRIKING), (int)arm.State);
				Assert::AreEqual(0, e.DamageTicks);
				Assert::IsFalse(e.Push);
			}
		}

		TEST_METHOD(Strike_CapsuleRadiiAlsoCountAtTip)
		{
			DRILL arm;
			DrillCalcReset(arm);
			const TENTACLE_TARGET target = DrillTarget(3.0f);
			StepDrill(arm, 0.01f, &target, 1, true);
			Assert::AreEqual((int)DRILL_BORING, (int)arm.State);
		}

		TEST_METHOD(Strike_SkipsUnavailableAndChoosesNearestOverlappingEnemy)
		{
			DRILL arm;
			DrillCalcReset(arm);
			const TENTACLE_TARGET targets[]{ DrillTarget(1.3f, false), DrillTarget(2.6f), DrillTarget(1.6f) };
			StepDrill(arm, 0.01f, targets, 3, true);
			Assert::AreEqual(2, arm.Target);
			Assert::AreEqual((int)DRILL_BORING, (int)arm.State);
		}

		TEST_METHOD(Strike_UnavailableEnemyDoesNotHit)
		{
			DRILL arm;
			DrillCalcReset(arm);
			const TENTACLE_TARGET target = DrillTarget(2.0f, false);
			StepDrill(arm, 0.01f, &target, 1, true);
			Assert::AreEqual((int)DRILL_STRIKING, (int)arm.State);
			Assert::AreEqual(-1, arm.Target);
		}

		TEST_METHOD(Miss_StrikesForPointFourThenRestsForPointThree)
		{
			DRILL arm;
			DrillCalcReset(arm);
			StepDrill(arm, 0.39f, nullptr, 0, true);
			Assert::AreEqual((int)DRILL_STRIKING, (int)arm.State);
			const DRILL_EVENT e = StepDrill(arm, 0.01f);
			Assert::AreEqual((int)DRILL_COOLDOWN, (int)arm.State);
			Assert::IsFalse(e.Push);
			StepDrill(arm, 0.29f);
			Assert::AreEqual((int)DRILL_COOLDOWN, (int)arm.State);
			StepDrill(arm, 0.01f);
			Assert::AreEqual((int)DRILL_IDLE, (int)arm.State);
			StepDrill(arm, 0.01f, nullptr, 0, true);
			Assert::AreEqual((int)DRILL_STRIKING, (int)arm.State);
		}

		TEST_METHOD(Boring_OneTickEveryPointOneAndFifthTickPushesOnce)
		{
			DRILL arm = BoringDrill();
			const TENTACLE_TARGET target = DrillTarget(2.0f);
			Assert::AreEqual(0.0f, arm.Time, DRILL_EPSILON);
			Assert::AreEqual(0, StepDrill(arm, 0.09f, &target, 1).DamageTicks);
			for (int tick = 1; tick <= 5; ++tick) {
				const DRILL_EVENT e = StepDrill(arm, tick == 1 ? 0.01f : 0.1f, &target, 1);
				Assert::AreEqual(1, e.DamageTicks);
				Assert::AreEqual(0, e.Target);
				Assert::AreEqual(tick == 5, e.Push);
			}
			Assert::AreEqual(5, arm.Ticks);
			Assert::AreEqual((int)DRILL_COOLDOWN, (int)arm.State);
			const DRILL_EVENT after = StepDrill(arm, 0.01f, &target, 1);
			Assert::AreEqual(0, after.DamageTicks);
			Assert::IsFalse(after.Push);
		}

		TEST_METHOD(Boring_LongFramesCollectTicksButNeverExceedFive)
		{
			DRILL arm = BoringDrill();
			const TENTACLE_TARGET target = DrillTarget(2.0f);
			const DRILL_EVENT first = StepDrill(arm, 0.35f, &target, 1);
			Assert::AreEqual(3, first.DamageTicks);
			Assert::IsFalse(first.Push);
			const DRILL_EVENT last = StepDrill(arm, 8.0f, &target, 1);
			Assert::AreEqual(2, last.DamageTicks);
			Assert::IsTrue(last.Push);
			DRILL all = BoringDrill();
			Assert::AreEqual(5, StepDrill(all, 8.0f, &target, 1).DamageTicks);
		}

		TEST_METHOD(Boring_LeavingDisappearingOrBeingGrabbedCancelsBeforeTick)
		{
			for (int reason = 0; reason < 3; ++reason) {
				DRILL arm = BoringDrill();
				const TENTACLE_TARGET target = DrillTarget(reason == 0 ? 10.0f : 2.0f, reason != 2);
				const DRILL_EVENT e = StepDrill(arm, 0.5f, reason == 1 ? nullptr : &target, reason == 1 ? 0 : 1);
				Assert::AreEqual((int)DRILL_COOLDOWN, (int)arm.State);
				Assert::AreEqual(-1, arm.Target);
				Assert::AreEqual(0, e.DamageTicks);
				Assert::IsFalse(e.Push);
			}
		}

		TEST_METHOD(Boring_MovingShoulderAwayCancelsEvenWithAnotherNearbyEnemy)
		{
			DRILL arm = BoringDrill();
			const TENTACLE_TARGET targets[]{ DrillTarget(2.0f), DrillTarget(12.0f) };
			const DRILL_EVENT e = StepDrill(arm, 0.5f, targets, 2, false, false, { 0, 0, 10 });
			Assert::AreEqual((int)DRILL_COOLDOWN, (int)arm.State);
			Assert::AreEqual(0, e.DamageTicks);
			Assert::IsFalse(e.Push);
		}

		TEST_METHOD(Respawn_CancelsStrikeAndBoringWithoutPushEvenWhenTimeStopped)
		{
			const TENTACLE_TARGET target = DrillTarget(2.0f);
			for (DRILL_STATE state : { DRILL_STRIKING, DRILL_BORING }) {
				DRILL arm = BoringDrill();
				arm.State = state;
				const DRILL_EVENT e = StepDrill(arm, 0.0f, &target, 1, true, true);
				Assert::AreEqual((int)DRILL_COOLDOWN, (int)arm.State);
				Assert::AreEqual(0, e.DamageTicks);
				Assert::IsFalse(e.Push);
			}
		}

		TEST_METHOD(RepeatedTriggersDoNotRestartActiveStatesOrChangeDirection)
		{
			const TENTACLE_TARGET target = DrillTarget(2.0f);
			for (DRILL_STATE state : { DRILL_STRIKING, DRILL_BORING, DRILL_COOLDOWN }) {
				DRILL control = BoringDrill();
				control.State = state;
				control.Time = 0.15f;
				DRILL pressed = control;
				const DRILL_EVENT expected = StepDrill(control, 0.01f, &target, 1);
				const DRILL_EVENT actual = StepDrill(pressed, 0.01f, &target, 1, true, false, {}, { -1, 4, 0 });
				Assert::AreEqual((int)control.State, (int)pressed.State);
				Assert::AreEqual(control.Time, pressed.Time, DRILL_EPSILON);
				Assert::AreEqual(control.Ticks, pressed.Ticks);
				Assert::AreEqual(control.Target, pressed.Target);
				Assert::AreEqual(1.0f, pressed.Direction.Z, DRILL_EPSILON);
				Assert::AreEqual(expected.DamageTicks, actual.DamageTicks);
				Assert::AreEqual(expected.Push, actual.Push);
			}
		}

		TEST_METHOD(ZeroOrNegativeTime_DoesNotStartOrAdvance)
		{
			for (float dt : { 0.0f, -1.0f }) {
				DRILL arm;
				DrillCalcReset(arm);
				StepDrill(arm, dt, nullptr, 0, true);
				Assert::AreEqual((int)DRILL_IDLE, (int)arm.State);
				arm = BoringDrill();
				const TENTACLE_TARGET target = DrillTarget(2.0f);
				Assert::AreEqual(0, StepDrill(arm, dt, &target, 1).DamageTicks);
				Assert::AreEqual(0.0f, arm.Time, DRILL_EPSILON);
			}
		}

		TEST_METHOD(PushDirection_IsHorizontalPlusSmallUpAndUnitLength)
		{
			const VEC3 push = DrillCalcPushDirection({ 3, -100, 4 }, 0.15f);
			const float length = sqrtf(1.0f + 0.15f * 0.15f);
			Assert::AreEqual(0.6f / length, push.X, DRILL_EPSILON);
			Assert::AreEqual(0.15f / length, push.Y, DRILL_EPSILON);
			Assert::AreEqual(0.8f / length, push.Z, DRILL_EPSILON);
			Assert::AreEqual(1.0f, Vec3Length(push), DRILL_EPSILON);
			const VEC3 fallback = DrillCalcPushDirection({ 0, 5, 0 }, 0.15f);
			Assert::AreEqual(0.0f, fallback.X, DRILL_EPSILON);
			Assert::AreEqual(1.0f / length, fallback.Z, DRILL_EPSILON);
		}
	};
}
