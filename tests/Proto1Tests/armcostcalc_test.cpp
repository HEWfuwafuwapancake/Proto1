#include "gtest/gtest.h"
#include "physics/armcostcalc.h"

// はじめの表は企画の値（ハンマーの弱当たり50・刀の強外れ40・ガード50）で、腕にない欄は0。
TEST(ArmCostCalcTest, DefaultMatchesPlan)
{
	const ARM_COST_TABLE table = ArmCostCalcDefault();
	EXPECT_EQ(50, table.Values[ARM_COST_ROW_HAMMER][ARM_COST_TAP_HIT]);
	EXPECT_EQ(30, table.Values[ARM_COST_ROW_HAMMER][ARM_COST_HOLD_MISS]);
	EXPECT_EQ(40, table.Values[ARM_COST_ROW_KATANA][ARM_COST_HOLD_MISS]);
	EXPECT_EQ(5, table.Values[ARM_COST_ROW_GUNLANCE][ARM_COST_EXTRA]);
	EXPECT_EQ(50, table.Values[ARM_COST_ROW_GUARD][ARM_COST_EXTRA]);
	EXPECT_FALSE(ArmCostCalcIsUsed(ARM_COST_ROW_TENTACLE, ARM_COST_EXTRA));
	EXPECT_EQ(0, table.Values[ARM_COST_ROW_TENTACLE][ARM_COST_EXTRA]);
}

// 書き換えは0〜100に収まり、腕にない欄は変わらない。
TEST(ArmCostCalcTest, SetClampsAndSkipsUnusedCells)
{
	ARM_COST_TABLE table = ArmCostCalcDefault();
	ArmCostCalcSet(table, ARM_COST_ROW_KNUCKLE, ARM_COST_TAP_MISS, 20);
	EXPECT_EQ(20, table.Values[ARM_COST_ROW_KNUCKLE][ARM_COST_TAP_MISS]);
	ArmCostCalcSet(table, ARM_COST_ROW_KNUCKLE, ARM_COST_TAP_MISS, 250);
	EXPECT_EQ(100, table.Values[ARM_COST_ROW_KNUCKLE][ARM_COST_TAP_MISS]);
	ArmCostCalcSet(table, ARM_COST_ROW_GUNLANCE, ARM_COST_TAP_HIT, 30);
	EXPECT_EQ(0, table.Values[ARM_COST_ROW_GUNLANCE][ARM_COST_TAP_HIT]);
}
