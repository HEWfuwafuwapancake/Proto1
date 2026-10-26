#include "CppUnitTest.h"
#include "physics/tentaclecalc.h"
#include "physics/knockbackcalc.h"
#include <initializer_list>
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
		TEST_METHOD(Wiggle_ExactEndpointsAndZeroAmplitude)
		{
			const VEC3 a{ 1, 2, 3 }, b{ 5, 8, 11 };
			const VEC3 first = TentacleCalcWigglePoint(a, b, 0, 2, 0.25f, 1.5f);
			const VEC3 last = TentacleCalcWigglePoint(a, b, 1, 2, 0.25f, 1.5f);
			Assert::AreEqual(0.0f, Vec3Length(Vec3Sub(first, a)));
			Assert::AreEqual(0.0f, Vec3Length(Vec3Sub(last, b)));
			const VEC3 middle = TentacleCalcWigglePoint(a, b, 0.25f, 2, 0, 1.5f);
			Assert::AreEqual(0.0f, Vec3Length(Vec3Sub(middle, { 2, 3.5f, 5 })), 0.0001f);
		}
		TEST_METHOD(Wiggle_FormulaAndPerpendicularOffset)
		{
			// +Z の腕の横は -X。s=0.5、波=0.5、位相=0 で横揺れが最大。
			const VEC3 p = TentacleCalcWigglePoint({}, { 0, 0, 4 }, 0.5f, 0, 0.25f, 0.5f);
			Assert::AreEqual(-0.25f, p.X, 0.0001f);
			Assert::AreEqual(0.0f, p.Y, 0.0001f);
			Assert::AreEqual(2.0f, p.Z, 0.0001f);
			const VEC3 vertical = TentacleCalcWigglePoint({}, { 0, 0, 4 }, 0.5f, 0, 0.25f, 0);
			Assert::AreEqual(0.125f, vertical.Y, 0.0001f);
			const VEC3 end{ 3, 4, 5 };
			const VEC3 curved = TentacleCalcWigglePoint({}, end, 0.3f, 1, 0.25f, 1.5f);
			Assert::AreEqual(0.0f, Vec3Dot(Vec3Sub(curved, Vec3Scale(end, 0.3f)), end), 0.0001f);
		}
		TEST_METHOD(Wiggle_VerticalAndZeroLength)
		{
			for (float height : { -4.0f, 4.0f }) {
				const VEC3 p = TentacleCalcWigglePoint({}, { 0, height, 0 }, 0.5f, 0, 0.25f, 0.5f);
				Assert::AreEqual(height * 0.5f, p.Y, 0.0001f);
				Assert::AreEqual(0.25f, fabsf(p.X), 0.0001f);
			}
			const VEC3 p = TentacleCalcWigglePoint(origin, origin, 0.5f, 2, 0.25f, 1.5f);
			Assert::AreEqual(0.0f, Vec3Length(Vec3Sub(p, origin)));
		}
		TEST_METHOD(PieceAxes_OrthonormalAndForwardIncludingDegenerateCases)
		{
			for (VEC3 end : { VEC3{ 2, 3, 4 }, VEC3{ 0, 4, 0 }, VEC3{ 0, -4, 0 }, VEC3{} }) {
				for (VEC3 down : { VEC3{ 0, -1, 0 }, end, VEC3{} }) {
					const auto axes = TentacleCalcPieceAxes({}, end, down);
					Assert::AreEqual(1.0f, Vec3Length(axes.X), 0.0001f);
					Assert::AreEqual(1.0f, Vec3Length(axes.Y), 0.0001f);
					Assert::AreEqual(1.0f, Vec3Length(axes.Z), 0.0001f);
					Assert::AreEqual(0.0f, Vec3Dot(axes.X, axes.Y), 0.0001f);
					Assert::AreEqual(0.0f, Vec3Dot(axes.Y, axes.Z), 0.0001f);
					Assert::AreEqual(0.0f, Vec3Dot(axes.Z, axes.X), 0.0001f);
					const VEC3 direction = Vec3Length(end) > 0 ? Vec3Scale(end, 1.0f / Vec3Length(end)) : VEC3{ 0, 0, 1 };
					Assert::AreEqual(-1.0f, Vec3Dot(axes.Y, direction), 0.0001f);
					const VEC3 cross{ axes.X.Y * axes.Y.Z - axes.X.Z * axes.Y.Y,
						axes.X.Z * axes.Y.X - axes.X.X * axes.Y.Z, axes.X.X * axes.Y.Y - axes.X.Y * axes.Y.X };
					Assert::AreEqual(1.0f, Vec3Dot(cross, axes.Z), 0.0001f);
				}
			}
		}
		TEST_METHOD(PieceAxes_SuckersFaceProjectedDown)
		{
			const VEC3 end{ 0, 3, 4 };
			const auto axes = TentacleCalcPieceAxes({}, end, { 0, -1, 0 });
			// -Z が、下を腕に直角な面へ映した向き (0, -0.8, 0.6) になる
			Assert::AreEqual(0.0f, axes.Z.X, 0.0001f);
			Assert::AreEqual(0.8f, axes.Z.Y, 0.0001f);
			Assert::AreEqual(-0.6f, axes.Z.Z, 0.0001f);
		}
		TEST_METHOD(Aim_TargetGivesHorizontalUnitDirection)
		{
			const VEC3 direction = TentacleCalcAim({ 2, 1, -3 }, { 5, 1, 1 }, true, aim);
			Assert::AreEqual(0.6f, direction.X, 0.0001f);
			Assert::AreEqual(0.0f, direction.Y, 0.0001f);
			Assert::AreEqual(0.8f, direction.Z, 0.0001f);
			Assert::AreEqual(1.0f, Vec3Length(direction), 0.0001f);
		}
		TEST_METHOD(Aim_IgnoresTargetHeightAndJumpHeight)
		{
			for (float height : { -20.0f, 20.0f }) {
				const VEC3 direction = TentacleCalcAim({ 2, -height, -3 }, { 5, height, 1 }, true, aim);
				Assert::AreEqual(0.6f, direction.X, 0.0001f);
				Assert::AreEqual(0.0f, direction.Y, 0.0001f);
				Assert::AreEqual(0.8f, direction.Z, 0.0001f);
			}
		}
		TEST_METHOD(Aim_NoTargetUsesFallback)
		{
			const VEC3 direction = TentacleCalcAim(origin, { 3, 1, 4 }, false, { -1, 0, 0 });
			Assert::AreEqual(-1.0f, direction.X, 0.0001f);
			Assert::AreEqual(0.0f, direction.Y, 0.0001f);
			Assert::AreEqual(0.0f, direction.Z, 0.0001f);
		}
		TEST_METHOD(Aim_TargetDirectlyAboveBelowOrAtShoulderUsesFallback)
		{
			for (float height : { -20.0f, 1.35f, 20.0f }) {
				const VEC3 direction = TentacleCalcAim(origin, { 0, height, 0 }, true, { -1, 0, 0 });
				Assert::AreEqual(-1.0f, direction.X, 0.0001f);
				Assert::AreEqual(0.0f, direction.Y, 0.0001f);
				Assert::AreEqual(0.0f, direction.Z, 0.0001f);
			}
		}
		TEST_METHOD(Aim_NearlyZeroHorizontalDistanceUsesFallback)
		{
			const VEC3 direction = TentacleCalcAim(origin, { 0.000001f, 10, -0.000001f }, true, aim);
			Assert::AreEqual(0.0f, direction.X, 0.0001f);
			Assert::AreEqual(0.0f, direction.Y, 0.0001f);
			Assert::AreEqual(1.0f, direction.Z, 0.0001f);
		}
		TEST_METHOD(Aim_FallbackIsHorizontalAndNormalized)
		{
			for (bool hasTarget : { false, true }) {
				const VEC3 direction = TentacleCalcAim(origin, origin, hasTarget, { -3, 12, 4 });
				Assert::AreEqual(-0.6f, direction.X, 0.0001f);
				Assert::AreEqual(0.0f, direction.Y, 0.0001f);
				Assert::AreEqual(0.8f, direction.Z, 0.0001f);
				Assert::AreEqual(1.0f, Vec3Length(direction), 0.0001f);
			}
		}
		TEST_METHOD(Aim_DegenerateFallbackUsesPositiveZ)
		{
			for (const VEC3& fallback : { VEC3{}, VEC3{ 0, 5, 0 }, VEC3{ 0.000001f, 5, 0 } }) {
				for (bool hasTarget : { false, true }) {
					const VEC3 direction = TentacleCalcAim(origin, origin, hasTarget, fallback);
					Assert::AreEqual(0.0f, direction.X, 0.0001f);
					Assert::AreEqual(0.0f, direction.Y, 0.0001f);
					Assert::AreEqual(1.0f, direction.Z, 0.0001f);
				}
			}
		}
		TEST_METHOD(Aim_EachShoulderPointsToSameTarget)
		{
			const VEC3 target{ 0, 9, 4 };
			const VEC3 left = TentacleCalcAim({ -3, 1, 0 }, target, true, aim);
			const VEC3 right = TentacleCalcAim({ 3, 1, 0 }, target, true, aim);
			Assert::AreEqual(0.6f, left.X, 0.0001f);
			Assert::AreEqual(-0.6f, right.X, 0.0001f);
			Assert::AreEqual(0.0f, left.Y, 0.0001f);
			Assert::AreEqual(0.0f, right.Y, 0.0001f);
			Assert::AreEqual(0.8f, left.Z, 0.0001f);
			Assert::AreEqual(0.8f, right.Z, 0.0001f);
		}
		TEST_METHOD(Aim_DistantTargetKeepsInitialDirectionAndMaximumLength)
		{
			TENTACLE arm; TentacleCalcReset(arm);
			const VEC3 initial = TentacleCalcAim(origin, { 30, 10, 40 }, true, aim);
			TentacleCalcStep(arm, origin, initial, hold, true, false, 0.05f, params, nullptr, 0, stage);
			const VEC3 moved = TentacleCalcAim(origin, { -40, -10, 30 }, true, aim);
			TentacleCalcStep(arm, origin, moved, hold, false, false, 0.05f, params, nullptr, 0, stage);
			Assert::AreEqual(1.44f, arm.Tip.X, 0.0001f);
			Assert::AreEqual(origin.Y, arm.Tip.Y, 0.0001f);
			Assert::AreEqual(1.92f, arm.Tip.Z, 0.0001f);
			TentacleCalcStep(arm, origin, moved, hold, false, false, 1, params, nullptr, 0, stage);
			Assert::AreEqual(4.8f, arm.Tip.X, 0.0001f);
			Assert::AreEqual(6.4f, arm.Tip.Z, 0.0001f);
			Assert::AreEqual(8.0f, arm.Length, 0.0001f);
			Assert::AreEqual((int)TENTACLE_RETURNING, (int)arm.State);
		}
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
		TEST_METHOD(ExtendRatio_GrowsWhileExtending)
		{
			TENTACLE arm{};
			TentacleCalcReset(arm);
			Assert::AreEqual(0.0f, TentacleCalcExtendRatio(arm, params));
			Step(arm, true, 0.1f);
			Assert::IsTrue(arm.State == TENTACLE_EXTENDING);
			const float first = TentacleCalcExtendRatio(arm, params);
			Assert::AreEqual(arm.Length / params.MaxLength, first, 0.0001f);
			Assert::IsTrue(first > 0.0f);
			Step(arm, false, 0.1f);
			Assert::IsTrue(TentacleCalcExtendRatio(arm, params) > first);
		}
		TEST_METHOD(ExtendRatio_ZeroWhenNotExtending)
		{
			TENTACLE arm{};
			TentacleCalcReset(arm);
			arm.Length = 4.0f;
			arm.State = TENTACLE_RETURNING;
			Assert::AreEqual(0.0f, TentacleCalcExtendRatio(arm, params));
			arm.State = TENTACLE_HOLDING;
			Assert::AreEqual(0.0f, TentacleCalcExtendRatio(arm, params));
			arm.State = TENTACLE_EXTENDING;
			Assert::AreEqual(0.5f, TentacleCalcExtendRatio(arm, params), 0.0001f);
			const TENTACLE_PARAMS zero{ 0, 24, 30, 3, 0.18f };
			Assert::AreEqual(0.0f, TentacleCalcExtendRatio(arm, zero));
		}
		TEST_METHOD(ExtendRatio_ClampsToOne)
		{
			TENTACLE arm{};
			TentacleCalcReset(arm);
			arm.State = TENTACLE_EXTENDING;
			arm.Length = 20.0f;
			Assert::AreEqual(1.0f, TentacleCalcExtendRatio(arm, params));
			arm.Length = 8.0f;
			Assert::AreEqual(1.0f, TentacleCalcExtendRatio(arm, params), 0.0001f);
			arm.Length = -1.0f;
			Assert::AreEqual(0.0f, TentacleCalcExtendRatio(arm, params));
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
