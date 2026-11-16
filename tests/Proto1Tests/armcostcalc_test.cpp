#include "gtest/gtest.h"
#include "physics/armcostcalc.h"

// はじめの表は企画の値（ハンマーの弱当たり50・刀の強外れ40・ガード50）で、腕にない欄は0。
TEST(ArmCostCalcTest, DefaultMatchesPlan)
{
	const ARM_COST_TABLE table = ArmCostCalcDefault();
	EXPECT_EQ(50, table.Values[ARM_COST_ROW_HAMMER][ARM_COST_TAP_HIT]);
	EXPECT_EQ(30, table.Values[ARM_COST_ROW_HAMMER][ARM_COST_HOLD_MISS]);
	EXPECT_EQ(40, table.Values[ARM_COST_ROW_KATANA][ARM_COST_HOLD_MISS]);
	EXPECT_EQ(10, table.Values[ARM_COST_ROW_LASER][ARM_COST_TAP_HIT]);
	EXPECT_EQ(50, table.Values[ARM_COST_ROW_GUARD][ARM_COST_EXTRA]);
	EXPECT_FALSE(ArmCostCalcIsUsed(ARM_COST_ROW_TENTACLE, ARM_COST_EXTRA));
	EXPECT_EQ(0, table.Values[ARM_COST_ROW_TENTACLE][ARM_COST_EXTRA]);
}

// ナックルの空中の突進は地上と同じ欄を使い、レーザーもそのほかの欄はない。
TEST(ArmCostCalcTest, KnuckleAndLaserHaveNoExtraCost)
{
	EXPECT_FALSE(ArmCostCalcIsUsed(ARM_COST_ROW_LASER, ARM_COST_EXTRA));
	EXPECT_FALSE(ArmCostCalcIsUsed(ARM_COST_ROW_KNUCKLE, ARM_COST_EXTRA));
}

// 書き換えは0〜100に収まり、腕にない欄は変わらない。
TEST(ArmCostCalcTest, SetClampsAndSkipsUnusedCells)
{
	ARM_COST_TABLE table = ArmCostCalcDefault();
	ArmCostCalcSet(table, ARM_COST_ROW_KNUCKLE, ARM_COST_TAP_MISS, 20);
	EXPECT_EQ(20, table.Values[ARM_COST_ROW_KNUCKLE][ARM_COST_TAP_MISS]);
	ArmCostCalcSet(table, ARM_COST_ROW_KNUCKLE, ARM_COST_TAP_MISS, 250);
	EXPECT_EQ(100, table.Values[ARM_COST_ROW_KNUCKLE][ARM_COST_TAP_MISS]);
	ArmCostCalcSet(table, ARM_COST_ROW_TENTACLE, ARM_COST_EXTRA, 30);
	EXPECT_EQ(0, table.Values[ARM_COST_ROW_TENTACLE][ARM_COST_EXTRA]);
}

// オプションの画面で並べる欄は、使う欄だけを行・列の順に数える（触手のそのほかは飛ばす）。
TEST(ArmCostCalcTest, UsedCellsSkipUnusedInRowOrder)
{
	int row = -1, item = -1;
	ASSERT_TRUE(ArmCostCalcUsedCell(0, row, item));
	EXPECT_EQ(ARM_COST_ROW_TENTACLE, row);
	EXPECT_EQ(ARM_COST_TAP_HIT, item);
	ASSERT_TRUE(ArmCostCalcUsedCell(4, row, item));
	EXPECT_EQ(ARM_COST_ROW_HAMMER, row);
	EXPECT_EQ(ARM_COST_TAP_HIT, item);
	ASSERT_TRUE(ArmCostCalcUsedCell(ArmCostCalcUsedCount() - 1, row, item));
	EXPECT_EQ(ARM_COST_ROW_GUARD, row);
	EXPECT_EQ(ARM_COST_EXTRA, item);
	EXPECT_FALSE(ArmCostCalcUsedCell(ArmCostCalcUsedCount(), row, item));
}
