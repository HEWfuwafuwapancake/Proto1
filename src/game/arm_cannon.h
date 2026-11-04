#pragma once

#include "game/arm.h"
#include "physics/armphasecalc.h"
#include "physics/projectilecalc.h"

/// @file arm_cannon.h
/// @brief 片手のキャノンの入力・砲弾・爆発と描画（グレネードをまとめた腕）

/// @brief 山なりの砲弾を撃ち、落ちた所・当たった所で爆発させる腕
class CannonArm : public Arm
{
public:
	/// @brief 待機状態にする（モデルはないので読み込まない）
	/// @param hand 左 0・右 1
	/// @param links 腕が使うゲームの部品への参照
	CannonArm(int hand, const ARM_LINKS& links);
	/// @brief 構え・休みを進め、構え終わりに撃つ。砲弾を動かし、当たったら爆発させる
	/// @param trigger 入力を設定する（true：押した瞬間、false：それ以外）
	void Update(bool trigger) override;
	/// @brief 仮の形（腕と砲身の円柱・砲弾の球・爆発の範囲の球）を描画する
	void Draw() override;
	/// @brief 待機へ戻し、砲弾を消す
	void Reset() override;
	/// @brief 構えている間だけ true を返す
	bool IsActive() const override;
	/// @brief 敵をつかまないので、常に false を返す
	bool IsHolding() const override;
	/// @brief 構えている間だけ歩く速さを下げる
	float GetMoveScale() const override;

private:
	ArmPhaseCalc m_Phase;
	ProjectileCalc m_Shells;
	VEC3 m_Blast = {}; // 最後の爆心。
	float m_BlastTime = 0.0f; // 爆発の範囲を見せる残りの秒数。
	float m_Straighten = 0.0f; // 描画だけのまっすぐ度（0：待機、1：狙いの向き）。
};
