#pragma once

/// @file armcostcalc.h
/// @brief 腕の耐久が攻撃ごとに減る量の表（DirectX・Windows なし。デバッグ用ウィンドウで書き換える）

/// @brief 表の行（腕の並びは ARM_TYPE の触手〜刀と同じ。最後はガード）
enum ARM_COST_ROW
{
	ARM_COST_ROW_TENTACLE, ///< 触手
	ARM_COST_ROW_HAMMER, ///< ハンマー
	ARM_COST_ROW_KNUCKLE, ///< ナックル
	ARM_COST_ROW_C4, ///< C4
	ARM_COST_ROW_GUNLANCE, ///< ガンランス
	ARM_COST_ROW_KATANA, ///< 刀
	ARM_COST_ROW_GUARD, ///< ガード（パリィできずに防いだ）
	ARM_COST_ROW_COUNT ///< 行の数
};

/// @brief 表の列（弱攻撃・強攻撃の当たり／外れと、そのほか）
enum ARM_COST_ITEM
{
	ARM_COST_TAP_HIT, ///< 弱攻撃（単押し）が1人でも効いた
	ARM_COST_TAP_MISS, ///< 弱攻撃（単押し）が誰にも効かなかった
	ARM_COST_HOLD_HIT, ///< 強攻撃（長押し）が1人でも効いた
	ARM_COST_HOLD_MISS, ///< 強攻撃（長押し）が誰にも効かなかった
	ARM_COST_EXTRA, ///< そのほか（空中の跳ね・空中の突進・場外で消えた爆弾・弾1発・ガード）
	ARM_COST_ITEM_COUNT ///< 列の数
};

/// @brief 腕の耐久が減る量の表
struct ARM_COST_TABLE
{
	int Values[ARM_COST_ROW_COUNT][ARM_COST_ITEM_COUNT]; ///< 行・列ごとの減る量（0〜100。使わない欄は0）
};

/// @brief 耐久の最大（減る量の上限にも使う）
static constexpr int ARM_COST_MAX = 100;

/// @brief はじめの表（docs/plan/06_腕の耐久/00_全体と進め方.md の値）を取得する
ARM_COST_TABLE ArmCostCalcDefault();
/// @brief その欄を使う腕か調べる（触手のそのほかなど、使わない欄は false）
bool ArmCostCalcIsUsed(int row, int item);
/// @brief 表の1欄を0〜100へ収めて書き換える（使わない欄・範囲外は何もしない）
/// @param table 書き換える表
/// @param row 行（ARM_COST_ROW）
/// @param item 列（ARM_COST_ITEM）
/// @param value 新しい減る量
void ArmCostCalcSet(ARM_COST_TABLE& table, int row, int item, int value);
