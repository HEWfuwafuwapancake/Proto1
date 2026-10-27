#include "CppUnitTest.h"
#include "physics/magnetcalc.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace Proto1Tests
{
	static constexpr float MAGNET_EPSILON = 0.0001f;
	static constexpr MAGNET_PARAMS MAGNET_TEST_PARAMS{ 10.0f, 0.8f, 15.0f, 0.1f, 1.5f, 0.3f };

	static TENTACLE_TARGET MagnetTarget(VEC3 center, bool available = true)
	{
		return { { Vec3Add(center, { 0, -0.4f, 0 }), Vec3Add(center, { 0, 0.4f, 0 }), 0.5f }, available };
	}

	static MAGNET_EVENT StepMagnet(MagnetCalc& arm, float dt, const TENTACLE_TARGET* targets,
		int count, bool trigger = false, bool cancel = false, VEC3 hold = { 0, 0, 2 },
		VEC3 shoulder = {}, VEC3 aim = { 0, 0, 1 })
	{
		return arm.Step(shoulder, aim, hold, trigger, cancel, dt, MAGNET_TEST_PARAMS, targets, count);
	}

	static MagnetCalc PullingMagnet()
	{
		MagnetCalc arm;
		arm.Reset();
		const TENTACLE_TARGET target = MagnetTarget({ 0, 0, 8 });
		StepMagnet(arm, 0.01f, &target, 1, true);
		return arm;
	}

	TEST_CLASS(MagnetCalcTest)
	{
	public:
		TEST_METHOD(Trigger_GrabsNearestAvailableEnemyInHorizontalBeamFromShoulder)
		{
			MagnetCalc arm;
			const VEC3 shoulder{ 4, 5, 6 };
			const TENTACLE_TARGET targets[]{
				MagnetTarget({ 6, 5, 6 }, false), MagnetTarget({ 12, 5, 6 }),
				MagnetTarget({ 9, 5, 6 }), MagnetTarget({ 16, 5, 6 })
			};
			StepMagnet(arm, 0.01f, targets, 4, false, false, {}, shoulder, { 1, 20, 0 });
			Assert::AreEqual((int)MAGNET_IDLE, (int)arm.GetState());
			const MAGNET_EVENT event = StepMagnet(arm, 0.01f, targets, 4, true, false, {}, shoulder, { 1, 20, 0 });
			Assert::AreEqual(2, event.Grabbed);
			Assert::AreEqual(-1, event.Released);
			Assert::IsFalse(event.Repel);
			Assert::AreEqual((int)MAGNET_PULLING, (int)arm.GetState());
			Assert::AreEqual(2, arm.GetTarget());
			Assert::AreEqual(9.0f, arm.GetCenter().X, MAGNET_EPSILON);
			Assert::AreEqual(5.0f, arm.GetCenter().Y, MAGNET_EPSILON);
			Assert::AreEqual(6.0f, arm.GetCenter().Z, MAGNET_EPSILON);
		}

		TEST_METHOD(Miss_SkipsFarUnavailableAndOffBeamEnemiesAndRestsForPointThree)
		{
			MagnetCalc arm;
			const TENTACLE_TARGET targets[]{
				MagnetTarget({ 0, 0, 12 }), MagnetTarget({ 0, 0, 3 }, false),
				MagnetTarget({ 3, 0, 4 }), MagnetTarget({ 0, -5, 4 })
			};
			const MAGNET_EVENT miss = StepMagnet(arm, 0.01f, targets, 4, true);
			Assert::AreEqual(-1, miss.Grabbed);
			Assert::AreEqual(-1, miss.Released);
			Assert::AreEqual((int)MAGNET_COOLDOWN, (int)arm.GetState());
			const TENTACLE_TARGET target = MagnetTarget({ 0, 0, 8 });
			Assert::AreEqual(-1, StepMagnet(arm, 0.29f, &target, 1, true).Grabbed);
			Assert::AreEqual((int)MAGNET_COOLDOWN, (int)arm.GetState());
			StepMagnet(arm, 0.01f, &target, 1, true);
			Assert::AreEqual((int)MAGNET_IDLE, (int)arm.GetState());
			Assert::AreEqual(0, StepMagnet(arm, 0.01f, &target, 1, true).Grabbed);
		}

		TEST_METHOD(Pulling_MovesAtFifteenTowardMovingHoldPointAndStopsWithoutOvershoot)
		{
			MagnetCalc arm = PullingMagnet();
			// 保持した敵自身は Available が false でも引き寄せを続ける。
			const TENTACLE_TARGET held = MagnetTarget({ 0, 0, 8 }, false);
			StepMagnet(arm, 0.2f, &held, 1);
			Assert::AreEqual(5.0f, arm.GetCenter().Z, MAGNET_EPSILON);
			Assert::AreEqual((int)MAGNET_PULLING, (int)arm.GetState());
			const VEC3 movedHold{ 3, 4, 5 };
			StepMagnet(arm, 0.2f, &held, 1, false, false, movedHold);
			Assert::AreEqual(1.8f, arm.GetCenter().X, MAGNET_EPSILON);
			Assert::AreEqual(2.4f, arm.GetCenter().Y, MAGNET_EPSILON);
			Assert::AreEqual(5.0f, arm.GetCenter().Z, MAGNET_EPSILON);
			StepMagnet(arm, 2.0f, &held, 1, false, false, movedHold);
			Assert::AreEqual((int)MAGNET_HOLDING, (int)arm.GetState());
			Assert::AreEqual(0.0f, Vec3Length(Vec3Sub(arm.GetCenter(), movedHold)), MAGNET_EPSILON);
			Assert::AreEqual(0.0f, arm.GetTime(), MAGNET_EPSILON);
		}

		TEST_METHOD(Holding_FollowsHoldPointAndReleasesWithoutRepelAfterOnePointFive)
		{
			MagnetCalc arm = PullingMagnet();
			const TENTACLE_TARGET held = MagnetTarget({ 0, 0, 8 }, false);
			StepMagnet(arm, 1.0f, &held, 1);
			const VEC3 movedHold{ 2, 1, 3 };
			const MAGNET_EVENT before = StepMagnet(arm, 1.4f, &held, 1, false, false, movedHold);
			Assert::AreEqual(-1, before.Released);
			Assert::AreEqual((int)MAGNET_HOLDING, (int)arm.GetState());
			Assert::AreEqual(0.0f, Vec3Length(Vec3Sub(arm.GetCenter(), movedHold)), MAGNET_EPSILON);
			const MAGNET_EVENT released = StepMagnet(arm, 0.1f, &held, 1, false, false, movedHold);
			Assert::AreEqual(0, released.Released);
			Assert::IsFalse(released.Repel);
			Assert::AreEqual((int)MAGNET_COOLDOWN, (int)arm.GetState());
			Assert::AreEqual(-1, arm.GetTarget());
		}

		TEST_METHOD(Retrigger_RepelsDuringPullingOrHoldingAndEntersCooldown)
		{
			for (MAGNET_STATE state : { MAGNET_PULLING, MAGNET_HOLDING }) {
				MagnetCalc arm = PullingMagnet();
				const TENTACLE_TARGET held = MagnetTarget({ 0, 0, 8 }, false);
				if (state == MAGNET_HOLDING) StepMagnet(arm, 1.0f, &held, 1);
				const MAGNET_EVENT event = StepMagnet(arm, 0.01f, &held, 1, true);
				Assert::AreEqual(0, event.Released);
				Assert::IsTrue(event.Repel);
				Assert::AreEqual((int)MAGNET_COOLDOWN, (int)arm.GetState());
				Assert::AreEqual(-1, arm.GetTarget());
				Assert::AreEqual(-1, StepMagnet(arm, 0.01f, &held, 1, true).Released);
			}
		}

		TEST_METHOD(DisappearanceOrRespawn_ReleasesWithoutRepelDuringPullingOrHolding)
		{
			for (MAGNET_STATE state : { MAGNET_PULLING, MAGNET_HOLDING }) {
				for (bool respawn : { false, true }) {
					MagnetCalc arm = PullingMagnet();
					const TENTACLE_TARGET held = MagnetTarget({ 0, 0, 8 }, false);
					if (state == MAGNET_HOLDING) StepMagnet(arm, 1.0f, &held, 1);
					const VEC3 center = arm.GetCenter();
					const MAGNET_EVENT event = StepMagnet(arm, 0.01f, respawn ? &held : nullptr,
						respawn ? 1 : 0, true, respawn, { 100, 100, 100 });
					Assert::AreEqual(0, event.Released);
					Assert::IsFalse(event.Repel);
					Assert::AreEqual((int)MAGNET_COOLDOWN, (int)arm.GetState());
					Assert::AreEqual(-1, arm.GetTarget());
					Assert::AreEqual(0.0f, Vec3Length(Vec3Sub(arm.GetCenter(), center)), MAGNET_EPSILON);
				}
			}
		}
	};
}
