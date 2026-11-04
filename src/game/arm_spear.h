#pragma once

#include "game/arm.h"
#include "physics/armphasecalc.h"

/// @file arm_spear.h
/// @brief 片手のスピアの入力・突きと描画

/// @brief 長い槍でまっすぐ突く腕。線に並んだ相手を全員突く
class SpearArm : public Arm
{
public:
	/// @brief 待機状態にする（モデルはないので読み込まない）
	/// @param hand 左 0・右 1
	/// @param links 腕が使うゲームの部品への参照
	SpearArm(int hand, const ARM_LINKS& links);
	/// @brief 溜め・突き・休みを進め、突きに入ったフレームに線の上の相手へ効かせる
	/// @param trigger 入力を設定する（true：押した瞬間、false：それ以外）
	void Update(bool trigger) override;
	/// @brief 仮の形（腕・柄の円柱と穂先の球）を描画する。突く間だけ柄が伸びる
	void Draw() override;
	/// @brief 待機へ戻す
	void Reset() override;
	/// @brief 待機以外なら true を返す
	bool IsActive() const override;
	/// @brief 敵をつかまないので、常に false を返す
	bool IsHolding() const override;

private:
	ArmPhaseCalc m_Phase;
	VEC3 m_Direction = { 0.0f, 0.0f, 1.0f }; // 突く水平の向き。
	float m_Straighten = 0.0f; // 描画だけのまっすぐ度（0：待機、1：突く向き）。
};
