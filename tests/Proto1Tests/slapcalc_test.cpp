#include "CppUnitTest.h"
#include "physics/slapcalc.h"
using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace Proto1Tests
{
	static const SLAP_PARAMS slapParams{ 3.0f, 0.3f, 1.04719755f, 0.2f, 0.3f };
	TEST_CLASS(SlapCalcTest)
	{
	public:
		TEST_METHOD(Start_ClearsHitsAndRejectsWhileActive)
		{
			SLAP slap{};
			HitRecordTryAdd(slap.Hits, 2);
			Assert::IsTrue(SlapCalcStart(slap));
			Assert::IsTrue(slap.Active);
			Assert::AreEqual(0, slap.Hits.Count);
			SlapCalcStep(slap, 0.1f, slapParams);
			Assert::IsFalse(SlapCalcStart(slap));
			Assert::AreEqual(0.1f, slap.Time);
		}
		TEST_METHOD(Duration_EndsAtPointTwoAndWaitsPointThree)
		{
			SLAP slap{}; SlapCalcStart(slap);
			SlapCalcStep(slap, 0.199f, slapParams);
			Assert::IsTrue(slap.Active);
			SlapCalcStep(slap, 0.001f, slapParams);
			Assert::IsFalse(slap.Active);
			Assert::AreEqual(0.3f, slap.Wait, 0.00001f);
			Assert::IsFalse(SlapCalcStart(slap));
			SlapCalcStep(slap, 0.299f, slapParams);
			Assert::IsFalse(SlapCalcStart(slap));
			SlapCalcStep(slap, 0.002f, slapParams);
			Assert::IsTrue(SlapCalcStart(slap));
		}
		TEST_METHOD(FixedFrames_EndAfterTwelveThenWaitEighteen)
		{
			SLAP slap{}; SlapCalcStart(slap);
			for (int i = 0; i < 12; ++i) SlapCalcStep(slap, 1.0f / 60.0f, slapParams);
			Assert::IsFalse(slap.Active);
			for (int i = 0; i < 18; ++i) SlapCalcStep(slap, 1.0f / 60.0f, slapParams);
			Assert::IsTrue(SlapCalcCanStart(slap));
		}
		TEST_METHOD(LongStep_ConsumesOnlyWaitAfterSwing)
		{
			SLAP slap{}; SlapCalcStart(slap);
			SlapCalcStep(slap, 0.4f, slapParams);
			Assert::IsFalse(slap.Active);
			Assert::AreEqual(0.1f, slap.Wait, 0.00001f);
			SlapCalcStep(slap, 1.0f, slapParams);
			Assert::IsTrue(SlapCalcCanStart(slap));
		}
		TEST_METHOD(Direction_RightAndLeftSweepOppositeWays)
		{
			SLAP slap{}; SlapCalcStart(slap);
			const VEC3 right = SlapCalcDirection(slap, { 0, 3, 2 }, 1, slapParams);
			const VEC3 left = SlapCalcDirection(slap, { 0, 0, 1 }, -1, slapParams);
			Assert::AreEqual(0.8660254f, right.X, 0.00001f);
			Assert::AreEqual(0.5f, right.Z, 0.00001f);
			Assert::AreEqual(-right.X, left.X, 0.00001f);
			Assert::AreEqual(0.0f, right.Y);
			SlapCalcStep(slap, 0.1f, slapParams);
			Assert::AreEqual(1.0f, SlapCalcDirection(slap, { 1, 0, 0 }, 1, slapParams).X, 0.00001f);
			SlapCalcStep(slap, 0.1f, slapParams);
			Assert::AreEqual(-right.X, SlapCalcDirection(slap, { 0, 0, 1 }, 1, slapParams).X, 0.00001f);
			Assert::AreEqual(right.X, SlapCalcDirection(slap, { 0, 0, 1 }, -1, slapParams).X, 0.00001f);
		}
		TEST_METHOD(Hit_OncePerEnemyAndAgainOnNextSwing)
		{
			SLAP slap{}; SlapCalcStart(slap);
			const CAPSULE reach{ { 0, 1, 0 }, { 0, 1, 3 }, 0.3f };
			const TENTACLE_TARGET targets[]{
				{ { { 0, 0.5f, 1 }, { 0, 1.3f, 1 }, 0.5f }, true },
				{ { { 0, 0.5f, 2 }, { 0, 1.3f, 2 }, 0.5f }, true } };
			Assert::AreEqual(0, SlapCalcHit(slap, reach, targets, 2));
			Assert::AreEqual(1, SlapCalcHit(slap, reach, targets, 2));
			Assert::AreEqual(-1, SlapCalcHit(slap, reach, targets, 2));
			SlapCalcStep(slap, 1, slapParams); SlapCalcStart(slap);
			Assert::AreEqual(0, SlapCalcHit(slap, reach, targets, 2));
		}
		TEST_METHOD(Hit_IgnoresHeldDeadDistantAndInactive)
		{
			SLAP slap{};
			const CAPSULE reach{ { 0, 1, 0 }, { 0, 1, 3 }, 0.3f };
			TENTACLE_TARGET targets[]{
				{ { { 0, 0.5f, 2 }, { 0, 1.3f, 2 }, 0.5f }, false },
				{ { { 5, 0.5f, 2 }, { 5, 1.3f, 2 }, 0.5f }, true }, {} };
			SlapCalcStart(slap);
			Assert::AreEqual(-1, SlapCalcHit(slap, reach, targets, 3));
			targets[0].Available = true;
			Assert::AreEqual(0, SlapCalcHit(slap, reach, targets, 3));
			SlapCalcStep(slap, 0.2f, slapParams);
			Assert::AreEqual(-1, SlapCalcHit(slap, reach, targets, 3));
		}
		TEST_METHOD(ZeroDelta_FreezesSwingAndWait)
		{
			SLAP slap{}; SlapCalcStart(slap);
			SlapCalcStep(slap, 0, slapParams);
			Assert::IsTrue(slap.Active); Assert::AreEqual(0.0f, slap.Time);
			SlapCalcStep(slap, 0.2f, slapParams);
			SlapCalcStep(slap, 0, slapParams);
			Assert::AreEqual(0.3f, slap.Wait, 0.00001f);
		}
		TEST_METHOD(Reset_ClearsSwingWaitAndHits)
		{
			SLAP slap{}; SlapCalcStart(slap); HitRecordTryAdd(slap.Hits, 1);
			SlapCalcReset(slap);
			Assert::IsTrue(SlapCalcCanStart(slap)); Assert::AreEqual(0, slap.Hits.Count);
			SlapCalcStart(slap); SlapCalcStep(slap, 0.2f, slapParams); SlapCalcReset(slap);
			Assert::IsTrue(SlapCalcCanStart(slap)); Assert::AreEqual(0.0f, slap.Time);
		}
	};
}
