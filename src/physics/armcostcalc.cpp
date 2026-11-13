#include "physics/armcostcalc.h"

// 調整用の値（はじめの減る量。-1 はその腕にない欄）
// 刀の強攻撃は、飛んだ瞬間に「強・外れ」を減らし、当たったら「強・当たり − 強・外れ」を足す。
static constexpr int DEFAULT_COSTS[ARM_COST_ROW_COUNT][ARM_COST_ITEM_COUNT] = {
	//  弱当たり 弱外れ 強当たり 強外れ そのほか
	{ 34, 15, 50, 25, -1 }, // 触手（弱：つかんだ・足場に付いた）
	{ 50, 25, 50, 30, 50 }, // ハンマー（強：衝撃波、そのほか：空中の跳ね）
	{ 34, 15, 50, 25, 50 }, // ナックル（強：地上の突進、そのほか：空中の突進）
	{ 34, 15, 50, 25, 5 }, // C4（弱：通常の爆弾、強：大きい爆弾、そのほか：場外で消えた爆弾1つ）
	{ -1, -1, 50, 25, 5 }, // ガンランス（強：突き刺し・突進の空振り、そのほか：弾1発）
	{ 34, 15, 50, 40, -1 }, // 刀（強：飛んで当たった・当たらない）
	{ -1, -1, -1, -1, 50 }, // ガード（そのほか：パリィできずに防いだ）
};

ARM_COST_TABLE ArmCostCalcDefault()
{
	ARM_COST_TABLE table{};
	for (int row = 0; row < ARM_COST_ROW_COUNT; ++row) {
		for (int item = 0; item < ARM_COST_ITEM_COUNT; ++item) {
			table.Values[row][item] = DEFAULT_COSTS[row][item] < 0 ? 0 : DEFAULT_COSTS[row][item];
		}
	}
	return table;
}

bool ArmCostCalcIsUsed(int row, int item)
{
	if (row < 0 || row >= ARM_COST_ROW_COUNT || item < 0 || item >= ARM_COST_ITEM_COUNT) return false;
	return DEFAULT_COSTS[row][item] >= 0;
}

void ArmCostCalcSet(ARM_COST_TABLE& table, int row, int item, int value)
{
	if (!ArmCostCalcIsUsed(row, item)) return;
	table.Values[row][item] = value < 0 ? 0 : (value > ARM_COST_MAX ? ARM_COST_MAX : value);
}
