#include "CppUnitTest.h"
#include "physics/grapplecalc.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace Proto1Tests
{
	static constexpr float GRAPPLE_EPSILON = 0.0001f;
	static constexpr float HALF_PI = 1.57079632679f;
	static constexpr GRAPPLE_PARAMS GRAPPLE_TEST_PARAMS{ 40, 16, 0.3f, 20, 1.2f, 0.8f, 1.5f };
	static constexpr STAGE FAR_STAGE{ 100, 100, 5, 5, 0, 1, -15 };
	static constexpr STAGE TEST_STAGE{ 0, 0, 5, 4, 2, 1, -15 };

	static void AssertPoint(VEC3 expected, VEC3 actual)
	{
		Assert::AreEqual(expected.X, actual.X, GRAPPLE_EPSILON);
		Assert::AreEqual(expected.Y, actual.Y, GRAPPLE_EPSILON);
		Assert::AreEqual(expected.Z, actual.Z, GRAPPLE_EPSILON);
	}

	static GRAPPLE Pulling(VEC3 anchor, int target = -1)
	{
		GRAPPLE arm;
		GrappleCalcReset(arm);
		arm.State = GRAPPLE_PULLING;
		arm.Anchor = arm.Hook = anchor;
		arm.Direction = { 0, 0, 1 };
		arm.Target = target;
		arm.OnStage = target < 0;
		return arm;
	}

	TEST_CLASS(GrappleCalcTest)
	{
	public:
		TEST_METHOD(Miss_ReturnsAt16ThenBecomesIdle)
		{
			GRAPPLE arm;
			GrappleCalcReset(arm);
			VEC3 center{};
			GrappleCalcStep(arm, {}, { 0, 0, 1 }, center, true, false, 0.1f,
				GRAPPLE_TEST_PARAMS, nullptr, 0, FAR_STAGE);
			Assert::AreEqual(4.0f, arm.Distance, GRAPPLE_EPSILON);
			for (int i = 0; i < 3; ++i) GrappleCalcStep(arm, {}, {}, center, false, false, 0.1f,
				GRAPPLE_TEST_PARAMS, nullptr, 0, FAR_STAGE);
			Assert::AreEqual((int)GRAPPLE_RETURNING, (int)arm.State);
			AssertPoint({ 0, 0, 16 }, arm.Hook);
			GrappleCalcStep(arm, {}, {}, center, false, false, 0.1f, GRAPPLE_TEST_PARAMS, nullptr, 0, FAR_STAGE);
			AssertPoint({ 0, 0, 12 }, arm.Hook);
			for (int i = 0; i < 3; ++i) GrappleCalcStep(arm, {}, {}, center, false, false, 0.1f,
				GRAPPLE_TEST_PARAMS, nullptr, 0, FAR_STAGE);
			Assert::AreEqual((int)GRAPPLE_IDLE, (int)arm.State);
			Assert::AreEqual(-1, arm.Target);
		}

		TEST_METHOD(Enemy_SweepsBetweenFrameEndpoints)
		{
			GRAPPLE arm;
			GrappleCalcReset(arm);
			// 両端の球では重ならない、フレームの途中の細い敵。
			const TENTACLE_TARGET target{ { { 0, -0.1f, 0.333f }, { 0, 0.1f, 0.333f }, 0.01f }, true };
			VEC3 center{ 0, 0, -5 };
			const GRAPPLE_EVENT e = GrappleCalcStep(arm, {}, { 0, 0, 1 }, center, true, false, 1.0f / 60,
				GRAPPLE_TEST_PARAMS, &target, 1, FAR_STAGE);
			Assert::IsTrue(e.Attached);
			Assert::AreEqual(0, arm.Target);
			Assert::AreEqual((int)GRAPPLE_PULLING, (int)arm.State);
			Assert::IsFalse(arm.OnStage);
			AssertPoint({ 0, 0, 0.333f }, arm.Anchor);
		}

		TEST_METHOD(Enemy_SkipsUnavailableAndChoosesFirstHit)
		{
			GRAPPLE arm;
			GrappleCalcReset(arm);
			const TENTACLE_TARGET targets[] = {
				{ { { 0, 0, 0.4f }, { 0, 1, 0.4f }, 0.1f }, false },
				{ { { 0, 0, 1.5f }, { 0, 1, 1.5f }, 0.1f }, true },
				{ { { 0, 0, 0.9f }, { 0, 1, 0.9f }, 0.1f }, true }
			};
			VEC3 center{};
			GrappleCalcStep(arm, {}, { 0, 0, 1 }, center, true, false, 0.05f,
				GRAPPLE_TEST_PARAMS, targets, 3, FAR_STAGE);
			Assert::AreEqual(2, arm.Target);
			GrappleCalcReset(arm);
			GrappleCalcStep(arm, {}, { 0, 0, 1 }, center, true, false, 0.01f,
				GRAPPLE_TEST_PARAMS, targets, 1, FAR_STAGE);
			Assert::AreEqual((int)GRAPPLE_FLYING, (int)arm.State);
		}

		TEST_METHOD(Stage_HitsSideTopAndBottomWithThickness)
		{
			const VEC3 origins[] = { { -6, 1.5f, 0 }, { 0, 3, 0 }, { 0, 0, 0 } };
			const VEC3 aims[] = { { 1, 0, 0 }, { 0, -1, 0 }, { 0, 1, 0 } };
			for (int i = 0; i < 3; ++i) {
				GRAPPLE arm;
				GrappleCalcReset(arm);
				VEC3 center{};
				const GRAPPLE_EVENT e = GrappleCalcStep(arm, origins[i], aims[i], center, true, false, 0.05f,
					GRAPPLE_TEST_PARAMS, nullptr, 0, TEST_STAGE);
				Assert::IsTrue(e.Attached);
				Assert::IsTrue(e.OnStage);
				Assert::AreEqual(-1, arm.Target);
			}
			GRAPPLE arm;
			GrappleCalcReset(arm);
			VEC3 center{};
			GrappleCalcStep(arm, { -6, 0, 0 }, { 1, 0, 0 }, center, true, false, 0.05f,
				GRAPPLE_TEST_PARAMS, nullptr, 0, TEST_STAGE);
			Assert::AreEqual((int)GRAPPLE_FLYING, (int)arm.State);
		}

		TEST_METHOD(Enemy_HasPriorityOverStageInSameSweep)
		{
			GRAPPLE arm;
			GrappleCalcReset(arm);
			const TENTACLE_TARGET target{ { { -4.5f, 1, 0 }, { -4.5f, 2, 0 }, 0.1f }, true };
			VEC3 center{};
			const GRAPPLE_EVENT e = GrappleCalcStep(arm, { -6, 1.5f, 0 }, { 1, 0, 0 }, center, true, false, 0.05f,
				GRAPPLE_TEST_PARAMS, &target, 1, TEST_STAGE);
			Assert::IsTrue(e.Attached);
			Assert::AreEqual(0, arm.Target);
			Assert::IsFalse(arm.OnStage);
		}

		TEST_METHOD(Anchor_FollowsEnemyEvenWhenHeldAndMissingEnemyReleases)
		{
			GRAPPLE arm = Pulling({ 0, 0, 5 }, 0);
			TENTACLE_TARGET target{ { { 2, 3, 8 }, { 2, 5, 8 }, 0.5f }, true };
			VEC3 center{};
			GrappleCalcStep(arm, {}, {}, center, false, false, 0.1f, GRAPPLE_TEST_PARAMS, &target, 1, FAR_STAGE);
			AssertPoint({ 2, 4, 8 }, arm.Anchor);
			AssertPoint(arm.Anchor, arm.Hook);
			target.Available = false;
			target.Body.Start.X = target.Body.End.X = 3;
			GrappleCalcStep(arm, {}, {}, center, false, false, 0.1f, GRAPPLE_TEST_PARAMS, &target, 1, FAR_STAGE);
			AssertPoint({ 3, 4, 8 }, arm.Anchor);
			Assert::AreEqual((int)GRAPPLE_PULLING, (int)arm.State);
			const VEC3 before = center;
			const GRAPPLE_EVENT e = GrappleCalcStep(arm, {}, {}, center, false, false, 0.1f,
				GRAPPLE_TEST_PARAMS, nullptr, 0, FAR_STAGE);
			Assert::IsTrue(e.Released);
			Assert::IsFalse(e.Arrived);
			AssertPoint(before, center);
			Assert::AreEqual((int)GRAPPLE_RETURNING, (int)arm.State);
		}

		TEST_METHOD(Pull_MovesAt20AndStopsAtArrivalDistance)
		{
			VEC3 center{};
			Assert::IsFalse(GrappleCalcPull(center, { 0, 0, 10 }, 20, 1.2f, 0.1f));
			AssertPoint({ 0, 0, 2 }, center);
			Assert::IsTrue(GrappleCalcPull(center, { 0, 0, 10 }, 20, 1.2f, 1.0f));
			AssertPoint({ 0, 0, 8.8f }, center);
			Assert::IsTrue(GrappleCalcPull(center, { 0, 0, 10 }, 20, 1.2f, 1.0f));
			AssertPoint({ 0, 0, 8.8f }, center);
			center = {};
			Assert::IsFalse(GrappleCalcPull(center, { 0, 0, 10 }, 20, 1.2f, 0));
			AssertPoint({}, center);
		}

		TEST_METHOD(Arrival_ReportsEnemyAndStageOnce)
		{
			const TENTACLE_TARGET target{ { { 0, -0.5f, 3 }, { 0, 0.5f, 3 }, 0.5f }, true };
			for (int kind = 0; kind < 2; ++kind) {
				GRAPPLE arm = Pulling({ 0, 0, 3 }, kind == 0 ? 0 : -1);
				VEC3 center{};
				const GRAPPLE_EVENT e = GrappleCalcStep(arm, {}, {}, center, false, false, 0.2f,
					GRAPPLE_TEST_PARAMS, &target, 1, FAR_STAGE);
				Assert::IsTrue(e.Arrived);
				Assert::IsTrue(e.Released);
				Assert::AreEqual(kind != 0, e.OnStage);
				Assert::AreEqual(kind == 0 ? 0 : -1, e.Target);
				AssertPoint({ 0, 0, kind == 0 ? 1.8f : 2.2f }, center);
				AssertPoint({ 0, 0, 1 }, e.Direction);
				const GRAPPLE_EVENT next = GrappleCalcStep(arm, {}, {}, center, false, false, 0.2f,
					GRAPPLE_TEST_PARAMS, &target, 1, FAR_STAGE);
				Assert::IsFalse(next.Arrived);
			}
		}

		TEST_METHOD(Landing_ClampsFourSidesAndCornersOnTranslatedStage)
		{
			const STAGE stage{ 10, 20, 5, 4, 2, 1, -15 };
			const VEC3 anchors[] = { { 5, 0, 20 }, { 15, 0, 20 }, { 10, 0, 16 }, { 10, 0, 24 },
				{ 5, 0, 16 }, { 15, 0, 16 }, { 5, 0, 24 }, { 15, 0, 24 } };
			const VEC3 expected[] = { { 5.8f, 2, 20 }, { 14.2f, 2, 20 }, { 10, 2, 16.8f }, { 10, 2, 23.2f },
				{ 5.8f, 2, 16.8f }, { 14.2f, 2, 16.8f }, { 5.8f, 2, 23.2f }, { 14.2f, 2, 23.2f } };
			for (int i = 0; i < 8; ++i) AssertPoint(expected[i], GrappleCalcLanding(anchors[i], stage, 0.8f));
		}

		TEST_METHOD(Trigger_ReleasesDuringFlightAndPullWithoutMovingBody)
		{
			for (int kind = 0; kind < 2; ++kind) {
				GRAPPLE arm = Pulling({ 0, 0, 8 });
				if (kind == 0) arm.State = GRAPPLE_FLYING;
				VEC3 center{ 1, 2, 3 };
				const GRAPPLE_EVENT e = GrappleCalcStep(arm, {}, {}, center, true, false, 0.1f,
					GRAPPLE_TEST_PARAMS, nullptr, 0, FAR_STAGE);
				Assert::IsTrue(e.Released);
				Assert::IsFalse(e.Arrived);
				AssertPoint({ 1, 2, 3 }, center);
				Assert::AreEqual((int)GRAPPLE_RETURNING, (int)arm.State);
			}
		}

		TEST_METHOD(Timeout_ReleasesAt1Point5Seconds)
		{
			GRAPPLE arm = Pulling({ 0, 0, 100 });
			VEC3 center{};
			GRAPPLE_EVENT e = GrappleCalcStep(arm, {}, {}, center, false, false, 1.0f,
				GRAPPLE_TEST_PARAMS, nullptr, 0, FAR_STAGE);
			Assert::IsFalse(e.Released);
			Assert::AreEqual(20.0f, center.Z, GRAPPLE_EPSILON);
			e = GrappleCalcStep(arm, {}, {}, center, false, false, 1.0f,
				GRAPPLE_TEST_PARAMS, nullptr, 0, FAR_STAGE);
			Assert::IsTrue(e.Released);
			Assert::IsFalse(e.Arrived);
			Assert::AreEqual(1.5f, arm.PullTime, GRAPPLE_EPSILON);
			Assert::AreEqual(30.0f, center.Z, GRAPPLE_EPSILON);
		}

		TEST_METHOD(Cancel_ResetsEveryStateEvenWhenTimeIsStopped)
		{
			for (int state = GRAPPLE_IDLE; state <= GRAPPLE_RETURNING; ++state) {
				GRAPPLE arm = Pulling({ 1, 2, 3 }, 0);
				arm.State = (GRAPPLE_STATE)state;
				VEC3 center{ 4, 5, 6 };
				GrappleCalcStep(arm, {}, {}, center, true, true, 0, GRAPPLE_TEST_PARAMS, nullptr, 0, FAR_STAGE);
				Assert::AreEqual((int)GRAPPLE_IDLE, (int)arm.State);
				Assert::AreEqual(-1, arm.Target);
				Assert::IsFalse(arm.OnStage);
				AssertPoint({ 4, 5, 6 }, center);
			}
		}

		TEST_METHOD(Aim_TargetIncludesHeightAndFallbackIsHorizontal)
		{
			AssertPoint({ 0, 0.6f, 0.8f }, GrappleCalcAim({ 0, -6, -4 }, { 0, 0, 4 }, true, { 1, 0, 0 }));
			AssertPoint({ 1, 0, 0 }, GrappleCalcAim({ 0, -6, -4 }, { 0, 0, 4 }, false, { 2, 5, 0 }));
			AssertPoint({ 1, 0, 0 }, GrappleCalcAim({ 1, 2, 3 }, { 1, 2, 3 }, true, { 1, 0, 0 }));
		}

		TEST_METHOD(Aim_FromBelowHitsStageBottom)
		{
			// 浮島（上の面 2・厚さ 1）より下から、上のターゲットへ撃つと浮島に刺さる。
			const VEC3 origin{ 0, -3, -6 };
			GRAPPLE arm;
			GrappleCalcReset(arm);
			VEC3 center{};
			GRAPPLE_EVENT e{};
			for (int i = 0; i < 30 && !e.Attached; ++i) {
				e = GrappleCalcStep(arm, origin, GrappleCalcAim(origin, { 0, 3, 0 }, true, { 0, 0, 1 }), center,
					i == 0, false, 1.0f / 60, GRAPPLE_TEST_PARAMS, nullptr, 0, TEST_STAGE);
			}
			Assert::IsTrue(e.Attached);
			Assert::IsTrue(e.OnStage);
		}

		TEST_METHOD(ModelPoint_ZeroRollUsesHorizontalXForwardMinusYAndDownMinusZ)
		{
			const VEC3 start{ 1, 2, 3 }, end{ 1, 2, 4 };
			AssertPoint({ 3, 2, 3 }, GrappleCalcModelPoint({ 1, 0, 0 }, start, end, 2, false, 0));
			AssertPoint({ 1, 2, 5 }, GrappleCalcModelPoint({ 0, -1, 0 }, start, end, 2, false, 0));
			AssertPoint({ 1, 0, 3 }, GrappleCalcModelPoint({ 0, 0, -1 }, start, end, 2, false, 0));
			AssertPoint({ -1, 2, 3 }, GrappleCalcModelPoint({ 1, 0, 0 }, start, end, 2, true, 0));
		}

		TEST_METHOD(ModelPoint_RollMovesBothShoulderBoxesDown)
		{
			AssertPoint({ 0, 1, 0 }, GrappleCalcModelPoint({ 1, 0, 0 }, {}, { 0, 0, 1 }, 1, false, HALF_PI));
			AssertPoint({ 0, -1, 0 }, GrappleCalcModelPoint({ 1, 0, 0 }, {}, { 0, 0, 1 }, 1, false, -HALF_PI));
			// 箱は元モデルの -X 側。左右の鏡写しと逆向きのひねりで、両方とも下。
			AssertPoint({ 0, -1, 0 }, GrappleCalcModelPoint({ -1, 0, 0 }, {}, { 0, 0, 1 }, 1, false, HALF_PI));
			AssertPoint({ 0, -1, 0 }, GrappleCalcModelPoint({ -1, 0, 0 }, {}, { 0, 0, 1 }, 1, true, -HALF_PI));
		}
	};
}
