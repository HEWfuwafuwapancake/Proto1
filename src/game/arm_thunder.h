#pragma once

#include "game/arm.h"
#include "physics/armphasecalc.h"

/// @file arm_thunder.h
/// @brief 片手のサンダーの入力・雷と描画

/// @brief 雷で前の相手をしびれさせ、近くの相手へ連鎖させる腕
class ThunderArm : public Arm
{
public:
	/// @brief 待機状態にする（モデルはないので読み込まない）
	/// @param hand 左 0・右 1
	/// @param links 腕が使うゲームの部品への参照
	ThunderArm(int hand, const ARM_LINKS& links);
	/// @brief ため・休みを進め、ため終わったフレームに雷を落として連鎖させる
	/// @param trigger 入力を設定する（true：押した瞬間、false：それ以外）
	void Update(bool trigger) override;
	/// @brief 仮の形（腕の円柱と、雷の黄色い円柱）を描画する
	void Draw() override;
	/// @brief 待機へ戻す
	void Reset() override;
	/// @brief ためている間だけ true を返す
	bool IsActive() const override;
	/// @brief 敵をつかまないので、常に false を返す
	bool IsHolding() const override;

private:
	static constexpr int MAX_CHAIN = 3; // 最初の相手を含めた連鎖の最大人数
	ArmPhaseCalc m_Phase;
	VEC3 m_Direction = { 0.0f, 0.0f, 1.0f }; // 雷を出す水平の向き。
	VEC3 m_Points[MAX_CHAIN + 1] = {}; // 雷の線の点（手 → 1人目 → …）。
	int m_PointCount = 0; // 雷の線の点の数。
	float m_BoltTime = 0.0f; // 雷を見せる残りの秒数。
	float m_Straighten = 0.0f; // 描画だけのまっすぐ度（0：待機、1：雷の向き）。
};
