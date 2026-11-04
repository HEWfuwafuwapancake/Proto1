#pragma once

#include "game/arm.h"
#include "physics/armphasecalc.h"

/// @file arm_flame.h
/// @brief 片手の火炎放射の入力・炎と描画（ポイズンをまとめた腕）

/// @brief 前の扇に炎を出し続け、当たった相手の % を足して燃やす腕
class FlameArm : public Arm
{
public:
	/// @brief 待機状態にする（モデルはないので読み込まない）
	/// @param hand 左 0・右 1
	/// @param links 腕が使うゲームの部品への参照
	FlameArm(int hand, const ARM_LINKS& links);
	/// @brief 炎・休みを進め、炎の刻みごとに扇の中の相手へ効かせる。炎の間の再入力で止める
	/// @param trigger 入力を設定する（true：押した瞬間、false：それ以外）
	void Update(bool trigger) override;
	/// @brief 仮の形（腕・ノズルの円柱と、炎の橙の球）を描画する
	void Draw() override;
	/// @brief 待機へ戻す
	void Reset() override;
	/// @brief 炎を出している間だけ true を返す
	bool IsActive() const override;
	/// @brief 敵をつかまないので、常に false を返す
	bool IsHolding() const override;
	/// @brief 炎を出している間だけ歩く速さを下げる
	float GetMoveScale() const override;

private:
	ArmPhaseCalc m_Phase;
	VEC3 m_Direction = { 0.0f, 0.0f, 1.0f }; // 炎の水平の向き。
	float m_Straighten = 0.0f; // 描画だけのまっすぐ度（0：待機、1：炎の向き）。
};
