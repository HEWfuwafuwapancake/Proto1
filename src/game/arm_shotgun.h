#pragma once

#include "game/arm.h"
#include "physics/armphasecalc.h"

/// @file arm_shotgun.h
/// @brief 片手のショットガンの入力・発射と描画

/// @brief 扇に並べた弾をすぐ当てる腕。近い相手ほど多く当たって強い
class ShotgunArm : public Arm
{
public:
	/// @brief 待機状態にする（モデルはないので読み込まない）
	/// @param hand 左 0・右 1
	/// @param links 腕が使うゲームの部品への参照
	ShotgunArm(int hand, const ARM_LINKS& links);
	/// @brief 押したフレームに撃って、当たった本数で効かせ、表示・休みを進める
	/// @param trigger 入力を設定する（true：押した瞬間、false：それ以外）
	void Update(bool trigger) override;
	/// @brief 仮の形（腕・銃身の円柱と、撃った直後の弾の線）を描画する
	void Draw() override;
	/// @brief 待機へ戻す
	void Reset() override;
	/// @brief 待機以外なら true を返す
	bool IsActive() const override;
	/// @brief 敵をつかまないので、常に false を返す
	bool IsHolding() const override;

private:
	ArmPhaseCalc m_Phase;
	VEC3 m_Direction = { 0.0f, 0.0f, 1.0f }; // 撃った水平の向き。
	float m_Straighten = 0.0f; // 描画だけのまっすぐ度（0：待機、1：撃つ向き）。
};
