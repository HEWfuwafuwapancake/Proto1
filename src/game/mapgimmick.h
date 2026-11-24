#pragma once

#include <cstdint>
#include "physics/liftcalc.h"
#include "physics/mapcalc.h"

class ArmManager;
class FighterManager;
class Stage;

/// @file mapgimmick.h
/// @brief 対戦マップの仕掛け（触手でＧから高所へ上がる・ダッシュ板）を動かして描く
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
	/// @brief 試合の最初に戻す（Ｇの予約・再使用の時刻・ダッシュ板に乗っていた記録を消す）
	void Reset();
	/// @brief 体を動かす前に呼ぶ：Ｇの始点で触手を単押しした人を受け付ける
	/// @return このフレームの入力（移動・ボタン）を使わないスロットのビット（Ｇで動いている人と、今受け付けた人）
	int PreUpdate();
	/// @brief Ｇで使った腕のボタンのうち、まだ離していないビットを取得する（離すまで腕に渡さない。PreUpdate のあとに使う）
	/// @param slot スロット番号
	uint32_t GetHeldButtons(int slot) const { return m_HeldButtons[slot]; }
	/// @brief 体を動かしたあとに呼ぶ：Ｇで動いている人の位置とダッシュ板を進める
	void PostUpdate();
	/// @brief Ｇの印・着地点・補助触腕を描く（ステージを描いたあと）
	/// @param viewer 画面を見ている人のスロット番号（起動できるＧを点灯する。いなければ -1）
	void Draw(int viewer) const;

private:
	bool CanStartLift(int slot) const;
	int GetTentacleHand(int slot) const;
	void DrawLift(int index, bool lit) const;

	const Stage* m_Stage = nullptr;
	FighterManager* m_Fighters = nullptr;
	const ArmManager* m_Arms = nullptr;
	LiftCalc m_Lift;
	int m_Frame = 0; // 試合の更新番号（試合中だけ進む）
	bool m_OnBelt[FIGHTER_COUNT] = {}; // 前の更新の終わりにダッシュ板の上に立っていた
	uint32_t m_HeldButtons[FIGHTER_COUNT] = {}; // Ｇで使った腕のボタン（離すまで腕に渡さない）
};
