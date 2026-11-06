#include <gtest/gtest.h>
#include "physics/statuscalc.h"

namespace Proto1Tests
{
	static constexpr float STATUS_DT = 1.0f / 60.0f;

	// しびれは時間で切れ、その間は歩けない。遅いは倍率を返す。
	TEST(StatusCalcTest, StunAndSlow_ExpireAndScale)
	{
		StatusCalc status;
		status.Apply(STATUS_STUN, 0.5f, 0.0f, 1);
		status.Apply(STATUS_SLOW, 2.0f, 0.4f, 1);
		EXPECT_EQ(0.0f, status.GetMoveScale());
		for (int f = 0; f < 31; ++f) status.Step(STATUS_DT);
		EXPECT_FALSE(status.IsActive(STATUS_STUN));
		EXPECT_NEAR(0.4f, status.GetMoveScale(), 0.0001f);
	}

	// 2秒燃えると 1秒あたりの % × 2 だけ増え、かけ直すと長い方が残る。
	TEST(StatusCalcTest, Burn_TotalAndRefresh)
	{
		StatusCalc status;
		status.Apply(STATUS_BURN, 2.0f, 2.0f, 3);
		status.Apply(STATUS_BURN, 1.0f, 2.0f, 3);
		float total = 0.0f;
		int attacker = -1;
		for (int f = 0; f < 180; ++f) {
			const STATUS_STEP step = status.Step(STATUS_DT);
			total += step.Damage;
			if (step.Attacker >= 0) attacker = step.Attacker;
		}
		EXPECT_NEAR(4.0f, total, 0.01f);
		EXPECT_EQ(3, attacker);
		EXPECT_FALSE(status.IsActive(STATUS_BURN));
	}
}
