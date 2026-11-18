#pragma once

#include "physics/cpucalc.h"
#include "physics/scenecalc.h"

class Stage;
class FighterManager;
class ArmManager;
class Camera;
class ArmCapsule;

/// @brief CPU 1体。試合の状況を集めて CpuCalc に渡し、人と同じ入力（PLAYER_INPUT）を作る
/// @details 腕・カメラ・当たり判定は人と同じものを使う。入力を作るだけで、体や腕を直接動かさない
class Cpu
{
public:
	/// @brief 試合の種・スロット・強さで最初の状態にする（試合の始めとやり直しで呼ぶ）
	/// @param seed 試合の乱数の種（スロットごとに混ぜて使う）
	/// @param slot 自分のスロット番号（0〜3）
	/// @param level 強さ（VERSUS_CPU_EASY〜VERSUS_CPU_HARD）
	void Reset(uint32_t seed, int slot, int level);
	/// @brief 今の試合の状況から、このフレームの入力を作る
	/// @param stage ステージ
	/// @param fighters 全員の体
	/// @param arms 自分の腕
	/// @param camera 自分のカメラ（ハードロックで使う）
	/// @param capsules 腕カプセル
	/// @param rule 試合のルール（GAME ならカプセルを拾いに行く）
	/// @return 人のパッドと同じ形の入力
	PLAYER_INPUT Update(const Stage& stage, const FighterManager& fighters, const ArmManager& arms,
		const Camera& camera, const ArmCapsule& capsules, GAME_RULE rule);

private:
	CpuCalc m_Calc;
	int m_Slot = 0;
	int m_Level = VERSUS_CPU_NORMAL; // 強さ
	uint32_t m_Seed = 0; // 考えを戻すときに使う乱数の種
};
