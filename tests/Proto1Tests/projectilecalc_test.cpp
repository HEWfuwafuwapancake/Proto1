#include <gtest/gtest.h>
#include "physics/projectilecalc.h"

namespace Proto1Tests
{
	static constexpr STAGE SHOT_STAGE{ 0.0f, 0.0f, 20.0f, 20.0f, 0.0f, 1.0f, -10.0f };
	static constexpr float SHOT_DT = 1.0f / 60.0f;

	// 足元が (x, 0, z) の高さ 1.8・半径 0.5 の体。
	static TENTACLE_TARGET ShotTarget(float x, float z)
	{
		return { { { x, 0.5f, z }, { x, 1.3f, z }, 0.5f }, true };
	}

	// 当たり・浮島・時間切れのどれかが起きるまで進め、最初の出来事を返す。
	static PROJECTILE_EVENT RunUntilEvent(ProjectileCalc& shots, const TENTACLE_TARGET* targets, int count, const PROJECTILE_PARAMS& params)
	{
		PROJECTILE_EVENT events[4] = {};
		for (int f = 0; f < 600; ++f) {
			if (shots.Step(SHOT_DT, targets, count, SHOT_STAGE, params, events, 4) > 0) return events[0];
		}
		return { PROJECTILE_EXPIRED, -1, -1, {}, {} };
	}

	// 速い弾でも体をすり抜けずに当たり、時間が来れば消える。
	TEST(ProjectileCalcTest, Straight_HitsBodyAndExpires)
	{
		const PROJECTILE_PARAMS params{ 0.1f, 0.0f, 2.0f, 0.0f, false };
		TENTACLE_TARGET target = ShotTarget(0.0f, 10.0f);
		ProjectileCalc shots;
		shots.Fire({ 0.0f, 1.0f, 0.3f }, { 0.0f, 0.0f, 180.0f }, -1, params); // 1フレームに 3 進む
		const PROJECTILE_EVENT hit = RunUntilEvent(shots, &target, 1, params);
		EXPECT_EQ((int)PROJECTILE_HIT_FIGHTER, (int)hit.Type);
		EXPECT_EQ(0, hit.Target);
		EXPECT_EQ(0, shots.GetCount());
		shots.Fire({ 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f }, -1, params);
		EXPECT_EQ((int)PROJECTILE_EXPIRED, (int)RunUntilEvent(shots, &target, 1, params).Type);
	}

	// 山なりの速度で撃つと、狙った所の近くで浮島に落ちる。
	TEST(ProjectileCalcTest, Lob_LandsNearTarget)
	{
		const PROJECTILE_PARAMS params{ 0.1f, 12.0f, 5.0f, 0.0f, false };
		const VEC3 from{ 0.0f, 1.2f, 0.0f };
		ProjectileCalc shots;
		shots.Fire(from, ProjectileCalcLobVelocity(from, { 0.0f, 0.0f, 8.0f }, 14.0f, 12.0f, 16.0f), -1, params);
		const PROJECTILE_EVENT land = RunUntilEvent(shots, nullptr, 0, params);
		EXPECT_EQ((int)PROJECTILE_HIT_STAGE, (int)land.Type);
		EXPECT_NEAR(8.0f, land.Position.Z, 0.3f);
	}

	// 追いかける弾は横の相手へ曲がって当たる。
	TEST(ProjectileCalcTest, Homing_TurnsToTarget)
	{
		const PROJECTILE_PARAMS params{ 0.2f, 0.0f, 3.0f, 3.5f, false };
		TENTACLE_TARGET targets[2] = { ShotTarget(6.0f, 6.0f), ShotTarget(0.0f, 30.0f) };
		ProjectileCalc shots;
		shots.Fire({ 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 11.0f }, 0, params);
		const PROJECTILE_EVENT hit = RunUntilEvent(shots, targets, 2, params);
		EXPECT_EQ((int)PROJECTILE_HIT_FIGHTER, (int)hit.Type);
		EXPECT_EQ(0, hit.Target);
	}

	// 貫通する弾は並んだ2体に1回ずつ当たる。
	TEST(ProjectileCalcTest, Pierce_HitsEachOnce)
	{
		const PROJECTILE_PARAMS params{ 0.1f, 0.0f, 1.0f, 0.0f, true };
		TENTACLE_TARGET targets[2] = { ShotTarget(0.0f, 4.0f), ShotTarget(0.0f, 8.0f) };
		ProjectileCalc shots;
		shots.Fire({ 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 20.0f }, -1, params);
		PROJECTILE_EVENT events[4] = {};
		int hits[2] = { 0, 0 };
		for (int f = 0; f < 60; ++f) {
			const int n = shots.Step(SHOT_DT, targets, 2, SHOT_STAGE, params, events, 4);
			for (int e = 0; e < n; ++e) {
				if (events[e].Type == PROJECTILE_HIT_FIGHTER) ++hits[events[e].Target];
			}
		}
		EXPECT_EQ(1, hits[0]);
		EXPECT_EQ(1, hits[1]);
	}
}
