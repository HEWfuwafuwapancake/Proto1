#pragma once

#include "game/arm.h"
#include "physics/armphasecalc.h"

/// @file arm_freeze.h
/// @brief 片手のフリーズの入力・冷気と描画

/// @brief 前の細い扇に冷気を出し、相手を遅くする腕。もう遅い相手は凍って動けなくなる
class FreezeArm : public Arm
{
public:
	/// @brief 待機状態にする（モデルはないので読み込まない）
	/// @param hand 左 0・右 1
	/// @param links 腕が使うゲームの部品への参照
	FreezeArm(int hand, const ARM_LINKS& links);
	/// @brief 押したフレームに扇の中の相手へ効かせ、冷気の表示・休みを進める
	/// @param trigger 入力を設定する（true：押した瞬間、false：それ以外）
	void Update(bool trigger) override;
	/// @brief 仮の形（腕・ノズルの円柱と、冷気の水色の球）を描画する
	void Draw() override;
	/// @brief 待機へ戻す
	void Reset() override;
	/// @brief 冷気を出している間だけ true を返す
	bool IsActive() const override;
	/// @brief 敵をつかまないので、常に false を返す
	bool IsHolding() const override;

private:
	ArmPhaseCalc m_Phase;
	VEC3 m_Direction = { 0.0f, 0.0f, 1.0f }; // 冷気の水平の向き。
	float m_Straighten = 0.0f; // 描画だけのまっすぐ度（0：待機、1：冷気の向き）。
};
