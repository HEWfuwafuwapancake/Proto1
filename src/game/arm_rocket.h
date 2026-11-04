#pragma once

#include "game/arm.h"
#include "physics/rocketpunchcalc.h"

/// @file arm_rocket.h
/// @brief 片手のロケットパンチの入力・発射と描画

/// @brief 拳を飛ばし、当たった相手を吹っ飛ばして肩へ戻す腕
class RocketArm : public Arm
{
public:
	/// @brief 待機状態にする（モデルはないので読み込まない）
	/// @param hand 左 0・右 1
	/// @param links 腕が使うゲームの部品への参照
	RocketArm(int hand, const ARM_LINKS& links);
	/// @brief 待機なら拳を飛ばし、飛んでいる間の再入力で呼び戻す。当たった相手へ効かせる
	/// @param trigger 入力を設定する（true：押した瞬間、false：それ以外）
	void Update(bool trigger) override;
	/// @brief 仮の形（腕の円柱と拳の箱。飛んでいる間は手首までの円柱と飛ぶ拳）を描画する
	void Draw() override;
	/// @brief 拳を腕に戻して待機にする
	void Reset() override;
	/// @brief 拳が腕を離れている間は true を返す
	bool IsActive() const override;
	/// @brief 敵をつかまないので、常に false を返す
	bool IsHolding() const override;

private:
	RocketPunchCalc m_Fist;
	float m_Straighten = 0.0f; // 描画だけのまっすぐ度（0：待機、1：飛ばした向き）。
};
