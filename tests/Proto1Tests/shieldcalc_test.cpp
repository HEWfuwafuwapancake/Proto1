#include "CppUnitTest.h"
#include "physics/shieldcalc.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace Proto1Tests
{
	static constexpr float SHIELD_EPSILON = 0.0001f;
	static constexpr SHIELD_PARAMS SHIELD_TEST_PARAMS{ 2.0f, 0.15f, 0.3f, 2.0f, 0.9f };

	TEST_CLASS(ShieldCalcTest)
	{
	public:
		TEST_METHOD(Trigger_RaisesShieldAndKeepsGuardingAfterRelease)
		{
			ShieldCalc arm;
			Assert::AreEqual((int)SHIELD_IDLE, (int)arm.GetState());
			Assert::IsFalse(arm.Step(true, false, 0.01f, SHIELD_TEST_PARAMS));
			Assert::AreEqual((int)SHIELD_GUARDING, (int)arm.GetState());
			Assert::AreEqual(0.0f, arm.GetTime(), SHIELD_EPSILON);
			Assert::IsFalse(arm.Step(false, false, 1.0f, SHIELD_TEST_PARAMS));
			Assert::AreEqual((int)SHIELD_GUARDING, (int)arm.GetState());
			Assert::AreEqual(1.0f, arm.GetTime(), SHIELD_EPSILON);
		}

		TEST_METHOD(Retrigger_BashesOnceThenRestsAndReturnsToIdle)
		{
			ShieldCalc arm;
			arm.Step(true, false, 0.01f, SHIELD_TEST_PARAMS);
			Assert::IsTrue(arm.Step(true, false, 0.01f, SHIELD_TEST_PARAMS));
			Assert::AreEqual((int)SHIELD_BASHING, (int)arm.GetState());
			Assert::AreEqual(0.0f, arm.GetTime(), SHIELD_EPSILON);
			Assert::IsFalse(arm.Step(true, false, 0.1f, SHIELD_TEST_PARAMS));
			Assert::AreEqual((int)SHIELD_BASHING, (int)arm.GetState());
			Assert::IsFalse(arm.Step(false, false, 0.05f, SHIELD_TEST_PARAMS));
			Assert::AreEqual((int)SHIELD_COOLDOWN, (int)arm.GetState());
			Assert::AreEqual(0.0f, arm.GetTime(), SHIELD_EPSILON);
			// 休みの途中と、休みが終わるフレームの入力は受け付けない。
			Assert::IsFalse(arm.Step(true, false, 0.2f, SHIELD_TEST_PARAMS));
			Assert::AreEqual((int)SHIELD_COOLDOWN, (int)arm.GetState());
			Assert::IsFalse(arm.Step(true, false, 0.1f, SHIELD_TEST_PARAMS));
			Assert::AreEqual((int)SHIELD_IDLE, (int)arm.GetState());
			Assert::AreEqual(0.0f, arm.GetTime(), SHIELD_EPSILON);
			arm.Step(true, false, 0.01f, SHIELD_TEST_PARAMS);
			Assert::AreEqual((int)SHIELD_GUARDING, (int)arm.GetState());
		}

		TEST_METHOD(GuardTimeout_LowersWithoutBashingAfterTwoSeconds)
		{
			ShieldCalc arm;
			arm.Step(true, false, 0.01f, SHIELD_TEST_PARAMS);
			// 実際の60fpsと同じ刻みで2秒を数え、累積誤差でも1フレーム遅れないことを確かめる。
			for (int frame = 0; frame < 119; ++frame) {
				Assert::IsFalse(arm.Step(false, false, 1.0f / 60.0f, SHIELD_TEST_PARAMS));
			}
			Assert::AreEqual((int)SHIELD_GUARDING, (int)arm.GetState());
			Assert::IsFalse(arm.Step(false, false, 1.0f / 60.0f, SHIELD_TEST_PARAMS));
			Assert::AreEqual((int)SHIELD_COOLDOWN, (int)arm.GetState());
			Assert::AreEqual(0.0f, arm.GetTime(), SHIELD_EPSILON);
		}

		TEST_METHOD(Respawn_CancelsGuardOrBashWithoutBashing)
		{
			for (SHIELD_STATE state : { SHIELD_GUARDING, SHIELD_BASHING }) {
				ShieldCalc arm;
				arm.Step(true, false, 0.01f, SHIELD_TEST_PARAMS);
				if (state == SHIELD_BASHING) arm.Step(true, false, 0.01f, SHIELD_TEST_PARAMS);
				Assert::IsFalse(arm.Step(true, true, 0.01f, SHIELD_TEST_PARAMS));
				Assert::AreEqual((int)SHIELD_COOLDOWN, (int)arm.GetState());
				Assert::AreEqual(0.0f, arm.GetTime(), SHIELD_EPSILON);
			}
		}

		TEST_METHOD(Reset_ReturnsToIdleAndClearsTime)
		{
			ShieldCalc arm;
			arm.Step(true, false, 0.01f, SHIELD_TEST_PARAMS);
			arm.Step(false, false, 0.5f, SHIELD_TEST_PARAMS);
			arm.Reset();
			Assert::AreEqual((int)SHIELD_IDLE, (int)arm.GetState());
			Assert::AreEqual(0.0f, arm.GetTime(), SHIELD_EPSILON);
		}

		TEST_METHOD(BashCapsule_ExtendsTwoUnitsHorizontallyFromShoulder)
		{
			const VEC3 shoulder{ 4, 5, 6 };
			const CAPSULE capsule = ShieldCalcBashCapsule(shoulder, { 3, 20, 4 }, SHIELD_TEST_PARAMS);
			Assert::AreEqual(0.0f, Vec3Length(Vec3Sub(capsule.Start, shoulder)), SHIELD_EPSILON);
			Assert::AreEqual(5.2f, capsule.End.X, SHIELD_EPSILON);
			Assert::AreEqual(5.0f, capsule.End.Y, SHIELD_EPSILON);
			Assert::AreEqual(7.6f, capsule.End.Z, SHIELD_EPSILON);
			Assert::AreEqual(0.9f, capsule.Radius, SHIELD_EPSILON);
		}

		TEST_METHOD(PushVelocity_AddsUpToHorizontalForwardAndHasFixedSpeed)
		{
			const VEC3 velocity = ShieldCalcPushVelocity({ 3, 20, 4 }, 12.0f, 0.15f);
			const float expectedScale = 12.0f / sqrtf(1.0f + 0.15f * 0.15f);
			Assert::AreEqual(0.6f * expectedScale, velocity.X, SHIELD_EPSILON);
			Assert::AreEqual(0.15f * expectedScale, velocity.Y, SHIELD_EPSILON);
			Assert::AreEqual(0.8f * expectedScale, velocity.Z, SHIELD_EPSILON);
			Assert::AreEqual(12.0f, Vec3Length(velocity), SHIELD_EPSILON);
		}
	};
}
