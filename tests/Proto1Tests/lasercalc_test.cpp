#include "gtest/gtest.h"
#include "physics/lasercalc.h"

namespace
{
	const LASER_PARAMS PARAMS{ 3.0f, 0.25f, 0.4f, 0.6f };

	ARM_INPUT Pressed() { ARM_INPUT input; input.Pressed = true; return input; }

	// z = z の位置に立つ体（太さ 0.4）
	TENTACLE_TARGET Body(float x, float z, bool available = true)
	{
		return { { { x, 0.5f, z }, { x, 1.5f, z }, 0.4f }, available };
	}
}

// 単押しで単発を撃ち、休みの間の単押しは撃たない。
TEST(LaserCalcTest, TapFiresShotAndWaitsForRecover)
{
	LaserCalc laser;
	ARM_INPUT tap; tap.Tap = true;
	EXPECT_EQ(LASER_FIRE_SHOT, laser.Step(tap, 0.016f, PARAMS));
	EXPECT_EQ(LASER_FIRE_NONE, laser.Step(tap, 0.1f, PARAMS));
	laser.Step({}, 0.4f, PARAMS);
	EXPECT_EQ(LASER_FIRE_SHOT, laser.Step(tap, 0.016f, PARAMS));
}

// 長押しで溜め、3秒（押し始めから）たつ前に離すと単発、溜めきってから離すと砲。
TEST(LaserCalcTest, ReleaseFiresShotBeforeFullChargeAndCannonAfter)
{
	LaserCalc laser;
	ARM_INPUT start = Pressed(); start.HoldStarted = true;
	ARM_INPUT release; release.HoldReleased = true;
	laser.Step(start, 0.0f, PARAMS);
	EXPECT_TRUE(laser.IsCharging());
	laser.Step(Pressed(), 1.0f, PARAMS);
	EXPECT_NEAR(1.25f / 3.0f, laser.GetCharge(PARAMS), 0.001f);
	EXPECT_EQ(LASER_FIRE_SHOT, laser.Step(release, 0.0f, PARAMS));

	laser.Reset();
	laser.Step(start, 0.0f, PARAMS);
	laser.Step(Pressed(), 2.8f, PARAMS);
	EXPECT_NEAR(1.0f, laser.GetCharge(PARAMS), 0.001f);
	EXPECT_EQ(LASER_FIRE_CANNON, laser.Step(release, 0.0f, PARAMS));
	EXPECT_FALSE(laser.IsCharging());
}

// 溜めている間に入力を取り上げられた（離したことにならない）ときは撃たずにやめる。
TEST(LaserCalcTest, LosingInputCancelsCharge)
{
	LaserCalc laser;
	ARM_INPUT start = Pressed(); start.HoldStarted = true;
	laser.Step(start, 0.0f, PARAMS);
	EXPECT_EQ(LASER_FIRE_NONE, laser.Step({}, 0.016f, PARAMS));
	EXPECT_FALSE(laser.IsCharging());
}

// 視線の先の最初の相手の手前を狙い、いなければ届く距離の点を狙う。
TEST(LaserCalcTest, AimPointStopsAtFirstTarget)
{
	const TENTACLE_TARGET targets[] = { Body(0.0f, 10.0f), Body(0.0f, 5.0f, false), Body(3.0f, 4.0f) };
	const VEC3 aim = LaserCalcAimPoint({ 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, 50.0f, 0.1f, targets, 3);
	EXPECT_NEAR(10.0f, aim.Z, 0.5f);
	const VEC3 far = LaserCalcAimPoint({ 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f }, 50.0f, 0.1f, targets, 1);
	EXPECT_NEAR(50.0f, far.X, 0.001f);
}

// 砲は線の上の相手を全部貫通し、線から外れた相手には当たらない。
TEST(LaserCalcTest, PierceHitsEveryoneOnTheBeam)
{
	const TENTACLE_TARGET targets[] = { Body(0.0f, 5.0f), Body(0.3f, 12.0f), Body(4.0f, 8.0f), Body(0.0f, 70.0f) };
	bool hits[4];
	EXPECT_EQ(2, LaserCalcPierce({ 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, 60.0f, 0.5f, targets, 4, hits));
	EXPECT_TRUE(hits[0]);
	EXPECT_TRUE(hits[1]);
	EXPECT_FALSE(hits[2]);
	EXPECT_FALSE(hits[3]);
}
