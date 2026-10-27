#include "CppUnitTest.h"
#include "physics/grapplecalc.h"
#include "physics/armcalc.h"

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

	static GrappleCalc Pulling(VEC3 anchor, int target = -1)
	{
		GrappleCalc arm{ GRAPPLE_PULLING, anchor, { 0, 0, 1 }, 0, target, target < 0, anchor, 0 };
		return arm;
	}

	TEST_CLASS(GrappleCalcTest)
	{
	public:
		TEST_METHOD(Miss_ReturnsAt16ThenBecomesIdle)
		{
			GrappleCalc arm;
			arm.Reset();
			VEC3 center{};
			arm.Step({}, { 0, 0, 1 }, center, true, false, 0.1f,
				GRAPPLE_TEST_PARAMS, nullptr, 0, FAR_STAGE);
			Assert::AreEqual(4.0f, arm.GetDistance(), GRAPPLE_EPSILON);
			for (int i = 0; i < 3; ++i) arm.Step({}, {}, center, false, false, 0.1f,
				GRAPPLE_TEST_PARAMS, nullptr, 0, FAR_STAGE);
			Assert::AreEqual((int)GRAPPLE_RETURNING, (int)arm.GetState());
			AssertPoint({ 0, 0, 16 }, arm.GetHook());
			arm.Step({}, {}, center, false, false, 0.1f, GRAPPLE_TEST_PARAMS, nullptr, 0, FAR_STAGE);
			AssertPoint({ 0, 0, 12 }, arm.GetHook());
			for (int i = 0; i < 3; ++i) arm.Step({}, {}, center, false, false, 0.1f,
				GRAPPLE_TEST_PARAMS, nullptr, 0, FAR_STAGE);
			Assert::AreEqual((int)GRAPPLE_IDLE, (int)arm.GetState());
			Assert::AreEqual(-1, arm.GetTarget());
		}

		TEST_METHOD(Enemy_SweepsBetweenFrameEndpoints)
		{
			GrappleCalc arm;
			arm.Reset();
			// 両端の球では重ならない、フレームの途中の細い敵。
			const TENTACLE_TARGET target{ { { 0, -0.1f, 0.333f }, { 0, 0.1f, 0.333f }, 0.01f }, true };
			VEC3 center{ 0, 0, -5 };
			const GRAPPLE_EVENT e = arm.Step({}, { 0, 0, 1 }, center, true, false, 1.0f / 60,
				GRAPPLE_TEST_PARAMS, &target, 1, FAR_STAGE);
			Assert::IsTrue(e.Attached);
			Assert::AreEqual(0, arm.GetTarget());
			Assert::AreEqual((int)GRAPPLE_PULLING, (int)arm.GetState());
			Assert::IsFalse(arm.GetOnStage());
			AssertPoint({ 0, 0, 0.333f }, arm.GetAnchor());
		}

		TEST_METHOD(Enemy_SkipsUnavailableAndChoosesFirstHit)
		{
			GrappleCalc arm;
			arm.Reset();
			const TENTACLE_TARGET targets[] = {
				{ { { 0, 0, 0.4f }, { 0, 1, 0.4f }, 0.1f }, false },
				{ { { 0, 0, 1.5f }, { 0, 1, 1.5f }, 0.1f }, true },
				{ { { 0, 0, 0.9f }, { 0, 1, 0.9f }, 0.1f }, true }
			};
			VEC3 center{};
			arm.Step({}, { 0, 0, 1 }, center, true, false, 0.05f,
				GRAPPLE_TEST_PARAMS, targets, 3, FAR_STAGE);
			Assert::AreEqual(2, arm.GetTarget());
			arm.Reset();
			arm.Step({}, { 0, 0, 1 }, center, true, false, 0.01f,
				GRAPPLE_TEST_PARAMS, targets, 1, FAR_STAGE);
			Assert::AreEqual((int)GRAPPLE_FLYING, (int)arm.GetState());
		}

		TEST_METHOD(Stage_HitsSideTopAndBottomWithThickness)
		{
			const VEC3 origins[] = { { -6, 1.5f, 0 }, { 0, 3, 0 }, { 0, 0, 0 } };
			const VEC3 aims[] = { { 1, 0, 0 }, { 0, -1, 0 }, { 0, 1, 0 } };
			for (int i = 0; i < 3; ++i) {
				GrappleCalc arm;
				arm.Reset();
				VEC3 center{};
				const GRAPPLE_EVENT e = arm.Step(origins[i], aims[i], center, true, false, 0.05f,
					GRAPPLE_TEST_PARAMS, nullptr, 0, TEST_STAGE);
				Assert::IsTrue(e.Attached);
				Assert::IsTrue(e.OnStage);
				Assert::AreEqual(-1, arm.GetTarget());
			}
			GrappleCalc arm;
			arm.Reset();
			VEC3 center{};
			arm.Step({ -6, 0, 0 }, { 1, 0, 0 }, center, true, false, 0.05f,
				GRAPPLE_TEST_PARAMS, nullptr, 0, TEST_STAGE);
			Assert::AreEqual((int)GRAPPLE_FLYING, (int)arm.GetState());
		}

		TEST_METHOD(Enemy_HasPriorityOverStageInSameSweep)
		{
			GrappleCalc arm;
			arm.Reset();
			const TENTACLE_TARGET target{ { { -4.5f, 1, 0 }, { -4.5f, 2, 0 }, 0.1f }, true };
			VEC3 center{};
			const GRAPPLE_EVENT e = arm.Step({ -6, 1.5f, 0 }, { 1, 0, 0 }, center, true, false, 0.05f,
				GRAPPLE_TEST_PARAMS, &target, 1, TEST_STAGE);
			Assert::IsTrue(e.Attached);
			Assert::AreEqual(0, arm.GetTarget());
			Assert::IsFalse(arm.GetOnStage());
		}

		TEST_METHOD(Anchor_FollowsEnemyEvenWhenHeldAndMissingEnemyReleases)
		{
			GrappleCalc arm = Pulling({ 0, 0, 5 }, 0);
			TENTACLE_TARGET target{ { { 2, 3, 8 }, { 2, 5, 8 }, 0.5f }, true };
			VEC3 center{};
			arm.Step({}, {}, center, false, false, 0.1f, GRAPPLE_TEST_PARAMS, &target, 1, FAR_STAGE);
			AssertPoint({ 2, 4, 8 }, arm.GetAnchor());
			AssertPoint(arm.GetAnchor(), arm.GetHook());
			target.Available = false;
			target.Body.Start.X = target.Body.End.X = 3;
			arm.Step({}, {}, center, false, false, 0.1f, GRAPPLE_TEST_PARAMS, &target, 1, FAR_STAGE);
			AssertPoint({ 3, 4, 8 }, arm.GetAnchor());
			Assert::AreEqual((int)GRAPPLE_PULLING, (int)arm.GetState());
			const VEC3 before = center;
			const GRAPPLE_EVENT e = arm.Step({}, {}, center, false, false, 0.1f,
				GRAPPLE_TEST_PARAMS, nullptr, 0, FAR_STAGE);
			Assert::IsTrue(e.Released);
			Assert::IsFalse(e.Arrived);
			AssertPoint(before, center);
			Assert::AreEqual((int)GRAPPLE_RETURNING, (int)arm.GetState());
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
				GrappleCalc arm = Pulling({ 0, 0, 3 }, kind == 0 ? 0 : -1);
				VEC3 center{};
				const GRAPPLE_EVENT e = arm.Step({}, {}, center, false, false, 0.2f,
					GRAPPLE_TEST_PARAMS, &target, 1, FAR_STAGE);
				Assert::IsTrue(e.Arrived);
				Assert::IsTrue(e.Released);
				Assert::AreEqual(kind != 0, e.OnStage);
				Assert::AreEqual(kind == 0 ? 0 : -1, e.Target);
				AssertPoint({ 0, 0, kind == 0 ? 1.8f : 2.2f }, center);
				AssertPoint({ 0, 0, 1 }, e.Direction);
				const GRAPPLE_EVENT next = arm.Step({}, {}, center, false, false, 0.2f,
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
				GrappleCalc arm = Pulling({ 0, 0, 8 });
				if (kind == 0) arm = { GRAPPLE_FLYING, arm.GetHook(), arm.GetDirection(), arm.GetDistance(),
					arm.GetTarget(), arm.GetOnStage(), arm.GetAnchor(), arm.GetPullTime() };
				VEC3 center{ 1, 2, 3 };
				const GRAPPLE_EVENT e = arm.Step({}, {}, center, true, false, 0.1f,
					GRAPPLE_TEST_PARAMS, nullptr, 0, FAR_STAGE);
				Assert::IsTrue(e.Released);
				Assert::IsFalse(e.Arrived);
				AssertPoint({ 1, 2, 3 }, center);
				Assert::AreEqual((int)GRAPPLE_RETURNING, (int)arm.GetState());
			}
		}

		TEST_METHOD(Timeout_ReleasesAt1Point5Seconds)
		{
			GrappleCalc arm = Pulling({ 0, 0, 100 });
			VEC3 center{};
			GRAPPLE_EVENT e = arm.Step({}, {}, center, false, false, 1.0f,
				GRAPPLE_TEST_PARAMS, nullptr, 0, FAR_STAGE);
			Assert::IsFalse(e.Released);
			Assert::AreEqual(20.0f, center.Z, GRAPPLE_EPSILON);
			e = arm.Step({}, {}, center, false, false, 1.0f,
				GRAPPLE_TEST_PARAMS, nullptr, 0, FAR_STAGE);
			Assert::IsTrue(e.Released);
			Assert::IsFalse(e.Arrived);
			Assert::AreEqual(1.5f, arm.GetPullTime(), GRAPPLE_EPSILON);
			Assert::AreEqual(30.0f, center.Z, GRAPPLE_EPSILON);
		}

		TEST_METHOD(Cancel_ResetsEveryStateEvenWhenTimeIsStopped)
		{
			for (int state = GRAPPLE_IDLE; state <= GRAPPLE_RETURNING; ++state) {
				GrappleCalc arm = Pulling({ 1, 2, 3 }, 0);
				arm = { (GRAPPLE_STATE)state, arm.GetHook(), arm.GetDirection(), arm.GetDistance(),
					arm.GetTarget(), arm.GetOnStage(), arm.GetAnchor(), arm.GetPullTime() };
				VEC3 center{ 4, 5, 6 };
				arm.Step({}, {}, center, true, true, 0, GRAPPLE_TEST_PARAMS, nullptr, 0, FAR_STAGE);
				Assert::AreEqual((int)GRAPPLE_IDLE, (int)arm.GetState());
				Assert::AreEqual(-1, arm.GetTarget());
				Assert::IsFalse(arm.GetOnStage());
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
			GrappleCalc arm;
			arm.Reset();
			VEC3 center{};
			GRAPPLE_EVENT e{};
			for (int i = 0; i < 30 && !e.Attached; ++i) {
				e = arm.Step(origin, GrappleCalcAim(origin, { 0, 3, 0 }, true, { 0, 0, 1 }), center,
					i == 0, false, 1.0f / 60, GRAPPLE_TEST_PARAMS, nullptr, 0, TEST_STAGE);
			}
			Assert::IsTrue(e.Attached);
			Assert::IsTrue(e.OnStage);
		}

		TEST_METHOD(ModelPoint_RollMovesBothShoulderBoxesDown)
		{
			AssertPoint({ 0, 1, 0 }, GrappleCalcModelPoint({ 1, 0, 0 }, {}, { 0, 0, 1 }, 1, false, HALF_PI));
			AssertPoint({ 0, -1, 0 }, GrappleCalcModelPoint({ 1, 0, 0 }, {}, { 0, 0, 1 }, 1, false, -HALF_PI));
			// 箱は元モデルの -X 側。左右の鏡写しと逆向きのひねりで、両方とも下。
			AssertPoint({ 0, -1, 0 }, GrappleCalcModelPoint({ -1, 0, 0 }, {}, { 0, 0, 1 }, 1, false, HALF_PI));
			AssertPoint({ 0, -1, 0 }, GrappleCalcModelPoint({ -1, 0, 0 }, {}, { 0, 0, 1 }, 1, true, -HALF_PI));
		}

		TEST_METHOD(ModelPoint_BendingForearmRaisesHookForBothHands)
		{
			const VEC3 shoulder{ 2, 3, -4 };
			const VEC3 forward{ 0, 0, 1 };
			const VEC3 upper = ArmCalcPitchDirection(forward, -HALF_PI * 0.5f);
			const VEC3 lower = ArmCalcPitchDirection(forward, 0.0f);
			for (int hand = 0; hand < 2; ++hand) {
				const bool mirror = hand == 0;
				const float roll = mirror ? -HALF_PI : HALF_PI;
				const VEC3 elbow = GrappleCalcModelPoint({ -1.614f, -10.900f, 0.241f },
					shoulder, Vec3Add(shoulder, upper), 0.08f, mirror, roll);
				const VEC3 straightHook = GrappleCalcModelPoint({ 0, -14.222f, 0 },
					elbow, Vec3Add(elbow, upper), 0.08f, mirror, roll);
				const VEC3 bentHook = GrappleCalcModelPoint({ 0, -14.222f, 0 },
					elbow, Vec3Add(elbow, lower), 0.08f, mirror, roll);
				Assert::IsTrue(bentHook.Y > straightHook.Y);
				Assert::AreEqual(elbow.Y, bentHook.Y, GRAPPLE_EPSILON);
				Assert::AreEqual(14.222f * 0.08f, Vec3Length(Vec3Sub(bentHook, elbow)), GRAPPLE_EPSILON);
				Assert::AreEqual(14.222f * 0.08f * sqrtf(0.5f), bentHook.Y - straightHook.Y, GRAPPLE_EPSILON);
			}
		}
	};
}
