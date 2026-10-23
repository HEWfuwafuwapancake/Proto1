#include "CppUnitTest.h"
#include "physics/tentaclecalc.h"
#include "physics/knockbackcalc.h"
using namespace Microsoft::VisualStudio::CppUnitTestFramework;

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
	static TENTACLE_EVENT Step(TENTACLE& arm, bool press, float dt, TENTACLE_TARGET* targets = nullptr, int count = 0)
	{
		return TentacleCalcStep(arm, origin, aim, hold, press, false, dt, params, targets, count, stage);
	}
	TEST_CLASS(TentacleCalcTest)
	{
	public:
		TEST_METHOD(Miss_ReturnsAndCanExtendAgain)
		{
			TENTACLE arm; TentacleCalcReset(arm);
			Step(arm, true, 1);
			Assert::AreEqual((int)TENTACLE_RETURNING, (int)arm.State);
			Assert::AreEqual(8.0f, arm.Tip.Z, 0.0001f);
			Step(arm, true, 1); // 戻る途中の連打を予約しない
			Assert::AreEqual((int)TENTACLE_IDLE, (int)arm.State);
			Step(arm, false, 0.1f);
			Assert::AreEqual((int)TENTACLE_IDLE, (int)arm.State);
			Step(arm, true, 0.1f);
			Assert::AreEqual((int)TENTACLE_EXTENDING, (int)arm.State);
		}
		TEST_METHOD(LongStep_ChoosesNearTargetRegardlessOfArrayOrder)
		{
			TENTACLE arm; TentacleCalcReset(arm);
			TENTACLE_TARGET targets[]{ Target(6), Target(3) };
			const auto event = Step(arm, true, 1, targets, 2);
			Assert::AreEqual(1, event.Grabbed);
			Assert::IsTrue(HitRecordContains(arm.Hits, 1));
			Assert::IsFalse(event.Throw);
		}
		TEST_METHOD(Grab_PullsTowardHandAndThrowsOnlyOnce)
		{
			TENTACLE arm; TentacleCalcReset(arm);
			TENTACLE_TARGET target = Target(6);
			Step(arm, true, 0.3f, &target, 1);
			target.Available = false; // 自分が保持中でも引き寄せる
			Step(arm, false, 0.1f, &target, 1);
			Assert::IsTrue(arm.Tip.Z < 6 && arm.Tip.Z > 2);
			const auto event = Step(arm, true, 0.01f, &target, 1);
			Assert::AreEqual(0, event.Released);
			Assert::IsTrue(event.Throw);
			Assert::AreEqual(-1, arm.Target);
			Assert::AreEqual(-1, Step(arm, true, 0.01f, &target, 1).Released);
		}
		TEST_METHOD(Timeout_ReleasesWithoutDamage)
		{
			TENTACLE arm; TentacleCalcReset(arm);
			TENTACLE_TARGET target = Target(3);
			Step(arm, true, 0.2f, &target, 1);
			const auto event = Step(arm, false, 3, &target, 1);
			Assert::AreEqual(0, event.Released);
			Assert::IsFalse(event.Throw);
		}
		TEST_METHOD(TwoArms_CannotGrabSameEnemy)
		{
			TENTACLE left, right; TentacleCalcReset(left); TentacleCalcReset(right);
			TENTACLE_TARGET targets[]{ Target(3), Target(6) };
			const auto first = Step(left, true, 1, targets, 2);
			targets[first.Grabbed].Available = false;
			Assert::AreEqual(1, Step(right, true, 1, targets, 2).Grabbed);
			Assert::AreEqual(0, left.Target);
		}
		TEST_METHOD(UnavailableEnemy_IsIgnored)
		{
			TENTACLE arm; TentacleCalcReset(arm);
			TENTACLE_TARGET target = Target(3); target.Available = false;
			Assert::AreEqual(-1, Step(arm, true, 1, &target, 1).Grabbed);
		}
		TEST_METHOD(Cancel_ReleasesAtOldPositionEvenWhenTimeStopped)
		{
			TENTACLE arm; TentacleCalcReset(arm);
			TENTACLE_TARGET target = Target(3);
			Step(arm, true, 0.2f, &target, 1);
			const auto event = TentacleCalcStep(arm, { 100, 1, 100 }, aim, hold, true, true, 0,
				params, &target, 1, stage);
			Assert::AreEqual(0, event.Released);
			Assert::IsFalse(event.Throw);
			Assert::AreEqual(-1, arm.Target);
			Assert::AreEqual((int)TENTACLE_IDLE, (int)arm.State);
		}
		TEST_METHOD(TimeStopped_DoesNotExtendOrThrow)
		{
			TENTACLE arm; TentacleCalcReset(arm);
			Step(arm, true, 0);
			Assert::AreEqual((int)TENTACLE_IDLE, (int)arm.State);
			TENTACLE_TARGET target = Target(3);
			Step(arm, true, 0.2f, &target, 1);
			Assert::AreEqual(-1, Step(arm, true, 0, &target, 1).Released);
			Assert::AreEqual(0.0f, arm.HoldTime);
		}
		TEST_METHOD(Direction_RemainsFixedWhileExtending)
		{
			TENTACLE arm; TentacleCalcReset(arm);
			Step(arm, true, 0.05f);
			TentacleCalcStep(arm, origin, { 1, 0, 0 }, hold, true, false, 0.05f, params, nullptr, 0, stage);
			Assert::AreEqual(0.0f, arm.Tip.X, 0.0001f);
			Assert::AreEqual(2.4f, arm.Tip.Z, 0.0001f);
		}
		TEST_METHOD(Stage_BlocksDownwardReach)
		{
			TENTACLE arm; TentacleCalcReset(arm);
			TENTACLE_TARGET target{ { { 0, -4, 0 }, { 0, -3, 0 }, 0.5f }, true };
			const auto event = TentacleCalcStep(arm, origin, { 0, -1, 0 }, hold, true, false, 1,
				params, &target, 1, stage);
			Assert::AreEqual(-1, event.Grabbed);
			Assert::IsTrue(arm.Tip.Y >= params.Radius);
			Assert::AreEqual((int)TENTACLE_RETURNING, (int)arm.State);
		}
		TEST_METHOD(HeldBody_DoesNotPassThroughPlatform)
		{
			const VEC3 clipped = TentacleCalcClip({ 0, 2, 0 }, { 0, -5, 0 }, stage, 0.5f, 0.9f);
			Assert::IsTrue(clipped.Y >= 0.9f && clipped.Y < 0.91f);
			const VEC3 outward = TentacleCalcClip({ 0, 0.9f, 0 }, { 0, 2, 0 }, stage, 0.5f, 0.9f);
			Assert::AreEqual(2.0f, outward.Y, 0.0001f);
			const VEC3 side = TentacleCalcClip({ 17, -1, 0 }, { 0, -1, 0 }, stage, 0.5f, 0.9f);
			Assert::IsTrue(side.X >= 15.5f);
		}
		TEST_METHOD(DirectedThrow_UsesPitchAndNormalizesSpeed)
		{
			const VEC3 down = KnockbackCalcDirectedVelocity({ 0, -3, 4 }, 10);
			Assert::AreEqual(-6.0f, down.Y, 0.0001f);
			Assert::AreEqual(8.0f, down.Z, 0.0001f);
			Assert::AreEqual(10.0f, Vec3Length(down), 0.0001f);
			Assert::AreEqual(10.0f, KnockbackCalcDirectedVelocity({ 0, 2, 0 }, 10).Y, 0.0001f);
			Assert::AreEqual(10.0f, KnockbackCalcDirectedVelocity({}, 10).Y, 0.0001f);
		}
	};
}
