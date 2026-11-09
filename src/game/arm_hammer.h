#pragma once

#include "game/arm.h"
#include "physics/armphasecalc.h"

/// @file arm_hammer.h
/// @brief 片手のハンマーの入力・叩きつけと描画

/// @brief 振り上げてから前の足元を叩き、まわりの相手を打ち上げる腕（遅いが強い）
class HammerArm : public Arm
{
public:
	/// @brief 待機状態にする（モデルはないので読み込まない）
	/// @param hand 左 0・右 1
	/// @param links 腕が使うゲームの部品への参照
	HammerArm(int hand, const ARM_LINKS& links);
	/// @brief 振り上げ・叩き・休みを進め、叩きに入ったフレームに着いた所のまわりへ効かせる
	/// @param input このフレームの腕の入力（単押しで攻撃する）
	void Update(const ARM_INPUT& input) override;
	/// @brief 仮の形（腕・柄の円柱と頭の箱）を、振り上げ・叩きに合わせて上下させて描画する
	void Draw() override;
	/// @brief 待機へ戻す
	void Reset() override;
	/// @brief 待機以外なら true を返す
	bool IsActive() const override;
	/// @brief 敵をつかまないので、常に false を返す
	bool IsHolding() const override;
	/// @brief 振り上げ・叩き・休みの間だけ歩く速さを下げる
	float GetMoveScale() const override;

private:
	ArmPhaseCalc m_Phase;
	VEC3 m_Direction = { 0.0f, 0.0f, 1.0f }; // 叩く水平の向き。
	float m_Straighten = 0.0f; // 描画だけのまっすぐ度（0：待機、1：叩く向き）。
};
