#pragma once

#include "game/arm.h"
#include "physics/armphasecalc.h"
#include "physics/projectilecalc.h"

/// @file arm_missile.h
/// @brief 片手のミサイルの入力・発射・爆発と描画

/// @brief ロックオンした相手を追いかけるミサイルを3発続けて撃つ腕
class MissileArm : public Arm
{
public:
	/// @brief 待機状態にする（モデルはないので読み込まない）
	/// @param hand 左 0・右 1
	/// @param links 腕が使うゲームの部品への参照
	MissileArm(int hand, const ARM_LINKS& links);
	/// @brief 押したら3発を少しずつずらして撃ち、ミサイルを動かして、当たったら小さく爆発させる
	/// @param trigger 入力を設定する（true：押した瞬間、false：それ以外）
	void Update(bool trigger) override;
	/// @brief 仮の形（腕・発射筒の円柱・ミサイルの球・爆発の範囲の球）を描画する
	void Draw() override;
	/// @brief 待機へ戻し、ミサイルを消す
	void Reset() override;
	/// @brief 撃っている間だけ true を返す
	bool IsActive() const override;
	/// @brief 敵をつかまないので、常に false を返す
	bool IsHolding() const override;

private:
	void FireOne();
	ArmPhaseCalc m_Phase;
	ProjectileCalc m_Missiles;
	int m_Fired = 0; // 今の発射で撃った数。
	int m_Target = -1; // 押したときにロックオンしていた相手（なしは -1）。
	VEC3 m_Blast = {}; // 最後の爆心。
	float m_BlastTime = 0.0f; // 爆発の範囲を見せる残りの秒数。
	float m_Straighten = 0.0f; // 描画だけのまっすぐ度（0：待機、1：上向き）。
};
