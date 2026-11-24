#pragma once

#include "physics/liftcalc.h"
#include "physics/mapcalc.h"

class ArmManager;
class FighterManager;
class Stage;

/// @file mapgimmick.h
/// @brief 対戦マップの仕掛け（Ｇで高所へ上がる・荷送りベルト・南の橋の空き缶）を動かして描く
/// @details 計算は physics/liftcalc.h と physics/mapcalc.h。試合の更新（60 Hz）の数で進めるので、通信でも全員同じに動く

/// @brief 対戦マップの仕掛けを、体の更新の前後に動かす
class MapGimmick
{
public:
	/// @brief ステージ・体・腕を受け取る（体と腕はスロット番号で使う）
	/// @param stage ステージ
	/// @param fighters 全員の体
	/// @param arms スロットごとの腕（FIGHTER_COUNT 個の並び）
	void Initialize(const Stage& stage, FighterManager& fighters, const ArmManager* arms);
	/// @brief 試合の最初に戻す（Ｇの予約・再使用の時刻・空き缶の合図を消す）
	void Reset();
	/// @brief 体を動かす前に呼ぶ：Ｇを押した人を受け付け、ベルトに乗れる人を覚える
	/// @return このフレームの入力（移動・ボタン）を使わないスロットのビット（Ｇで動いている人と、今受け付けた人）
	int PreUpdate();
	/// @brief 体を動かしたあとに呼ぶ：Ｇで動いている人の位置・ベルト・空き缶を進める
	void PostUpdate();
	/// @brief Ｇの印・着地点・補助触腕・空き缶を描く（ステージを描いたあと）
	/// @param viewer 画面を見ている人のスロット番号（起動できるＧを点灯する。いなければ -1）
	void Draw(int viewer) const;

private:
	bool CanStartLift(int slot) const;
	void DrawLift(int index, bool lit) const;

	const Stage* m_Stage = nullptr;
	FighterManager* m_Fighters = nullptr;
	const ArmManager* m_Arms = nullptr;
	LiftCalc m_Lift;
	CanBellCalc m_Cans;
	int m_Frame = 0; // 試合の更新番号（試合中だけ進む）
	bool m_BeltReady[FIGHTER_COUNT] = {}; // 更新の最初に立っていて、跳ばずに歩いていた（ベルトで運ぶ）
};
