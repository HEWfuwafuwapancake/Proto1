#include <gtest/gtest.h>
#include "physics/magnetcalc.h"

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

	TEST(MagnetCalcTest, Trigger_GrabsNearestAvailableEnemyInHorizontalBeamFromShoulder)
	{
		MagnetCalc arm;
		const VEC3 shoulder{ 4, 5, 6 };
		const TENTACLE_TARGET targets[]{
			MagnetTarget({ 6, 5, 6 }, false), MagnetTarget({ 12, 5, 6 }),
			MagnetTarget({ 9, 5, 6 }), MagnetTarget({ 16, 5, 6 })
		};
		StepMagnet(arm, 0.01f, targets, 4, false, false, {}, shoulder, { 1, 20, 0 });
		EXPECT_EQ((int)MAGNET_IDLE, (int)arm.GetState());
		const MAGNET_EVENT event = StepMagnet(arm, 0.01f, targets, 4, true, false, {}, shoulder, { 1, 20, 0 });
		EXPECT_EQ(2, event.Grabbed);
		EXPECT_EQ(-1, event.Released);
		EXPECT_FALSE(event.Repel);
		EXPECT_EQ((int)MAGNET_PULLING, (int)arm.GetState());
		EXPECT_EQ(2, arm.GetTarget());
		EXPECT_NEAR(9.0f, arm.GetCenter().X, MAGNET_EPSILON);
		EXPECT_NEAR(5.0f, arm.GetCenter().Y, MAGNET_EPSILON);
		EXPECT_NEAR(6.0f, arm.GetCenter().Z, MAGNET_EPSILON);
	}

	TEST(MagnetCalcTest, Miss_SkipsFarUnavailableAndOffBeamEnemiesAndRestsForPointThree)
	{
		MagnetCalc arm;
		const TENTACLE_TARGET targets[]{
			MagnetTarget({ 0, 0, 12 }), MagnetTarget({ 0, 0, 3 }, false),
			MagnetTarget({ 3, 0, 4 }), MagnetTarget({ 0, -5, 4 })
		};
		const MAGNET_EVENT miss = StepMagnet(arm, 0.01f, targets, 4, true);
		EXPECT_EQ(-1, miss.Grabbed);
		EXPECT_EQ(-1, miss.Released);
		EXPECT_EQ((int)MAGNET_COOLDOWN, (int)arm.GetState());
		const TENTACLE_TARGET target = MagnetTarget({ 0, 0, 8 });
		EXPECT_EQ(-1, StepMagnet(arm, 0.29f, &target, 1, true).Grabbed);
		EXPECT_EQ((int)MAGNET_COOLDOWN, (int)arm.GetState());
		StepMagnet(arm, 0.01f, &target, 1, true);
		EXPECT_EQ((int)MAGNET_IDLE, (int)arm.GetState());
		EXPECT_EQ(0, StepMagnet(arm, 0.01f, &target, 1, true).Grabbed);
	}

	TEST(MagnetCalcTest, Pulling_MovesAtFifteenTowardMovingHoldPointAndStopsWithoutOvershoot)
	{
		MagnetCalc arm = PullingMagnet();
		// 保持した敵自身は Available が false でも引き寄せを続ける。
		const TENTACLE_TARGET held = MagnetTarget({ 0, 0, 8 }, false);
		StepMagnet(arm, 0.2f, &held, 1);
		EXPECT_NEAR(5.0f, arm.GetCenter().Z, MAGNET_EPSILON);
		EXPECT_EQ((int)MAGNET_PULLING, (int)arm.GetState());
		const VEC3 movedHold{ 3, 4, 5 };
		StepMagnet(arm, 0.2f, &held, 1, false, false, movedHold);
		EXPECT_NEAR(1.8f, arm.GetCenter().X, MAGNET_EPSILON);
		EXPECT_NEAR(2.4f, arm.GetCenter().Y, MAGNET_EPSILON);
		EXPECT_NEAR(5.0f, arm.GetCenter().Z, MAGNET_EPSILON);
		StepMagnet(arm, 2.0f, &held, 1, false, false, movedHold);
		EXPECT_EQ((int)MAGNET_HOLDING, (int)arm.GetState());
		EXPECT_NEAR(0.0f, Vec3Length(Vec3Sub(arm.GetCenter(), movedHold)), MAGNET_EPSILON);
		EXPECT_NEAR(0.0f, arm.GetTime(), MAGNET_EPSILON);
	}

	TEST(MagnetCalcTest, Holding_FollowsHoldPointAndReleasesWithoutRepelAfterOnePointFive)
	{
		MagnetCalc arm = PullingMagnet();
		const TENTACLE_TARGET held = MagnetTarget({ 0, 0, 8 }, false);
		StepMagnet(arm, 1.0f, &held, 1);
		const VEC3 movedHold{ 2, 1, 3 };
		const MAGNET_EVENT before = StepMagnet(arm, 1.4f, &held, 1, false, false, movedHold);
		EXPECT_EQ(-1, before.Released);
		EXPECT_EQ((int)MAGNET_HOLDING, (int)arm.GetState());
		EXPECT_NEAR(0.0f, Vec3Length(Vec3Sub(arm.GetCenter(), movedHold)), MAGNET_EPSILON);
		const MAGNET_EVENT released = StepMagnet(arm, 0.1f, &held, 1, false, false, movedHold);
		EXPECT_EQ(0, released.Released);
		EXPECT_FALSE(released.Repel);
		EXPECT_EQ((int)MAGNET_COOLDOWN, (int)arm.GetState());
		EXPECT_EQ(-1, arm.GetTarget());
	}

	TEST(MagnetCalcTest, Retrigger_RepelsDuringPullingOrHoldingAndEntersCooldown)
	{
		for (MAGNET_STATE state : { MAGNET_PULLING, MAGNET_HOLDING }) {
			MagnetCalc arm = PullingMagnet();
			const TENTACLE_TARGET held = MagnetTarget({ 0, 0, 8 }, false);
			if (state == MAGNET_HOLDING) StepMagnet(arm, 1.0f, &held, 1);
			const MAGNET_EVENT event = StepMagnet(arm, 0.01f, &held, 1, true);
			EXPECT_EQ(0, event.Released);
			EXPECT_TRUE(event.Repel);
			EXPECT_EQ((int)MAGNET_COOLDOWN, (int)arm.GetState());
			EXPECT_EQ(-1, arm.GetTarget());
			EXPECT_EQ(-1, StepMagnet(arm, 0.01f, &held, 1, true).Released);
		}
	}

	TEST(MagnetCalcTest, DisappearanceOrRespawn_ReleasesWithoutRepelDuringPullingOrHolding)
	{
		for (MAGNET_STATE state : { MAGNET_PULLING, MAGNET_HOLDING }) {
			for (bool respawn : { false, true }) {
				MagnetCalc arm = PullingMagnet();
				const TENTACLE_TARGET held = MagnetTarget({ 0, 0, 8 }, false);
				if (state == MAGNET_HOLDING) StepMagnet(arm, 1.0f, &held, 1);
				const VEC3 center = arm.GetCenter();
				const MAGNET_EVENT event = StepMagnet(arm, 0.01f, respawn ? &held : nullptr,
					respawn ? 1 : 0, true, respawn, { 100, 100, 100 });
				EXPECT_EQ(0, event.Released);
				EXPECT_FALSE(event.Repel);
				EXPECT_EQ((int)MAGNET_COOLDOWN, (int)arm.GetState());
				EXPECT_EQ(-1, arm.GetTarget());
				EXPECT_NEAR(0.0f, Vec3Length(Vec3Sub(arm.GetCenter(), center)), MAGNET_EPSILON);
			}
		}
	}
}
