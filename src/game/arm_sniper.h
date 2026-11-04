#pragma once

#include "game/arm.h"
#include "physics/armphasecalc.h"

/// @file arm_sniper.h
/// @brief 片手のスナイパーの入力・狙い・発射と描画

/// @brief 1秒狙ってから最初に当たる1人を撃つ腕。遠い相手ほど強く吹っ飛ぶ
class SniperArm : public Arm
{
public:
	/// @brief 待機状態にする（モデルはないので読み込まない）
	/// @param hand 左 0・右 1
	/// @param links 腕が使うゲームの部品への参照
	SniperArm(int hand, const ARM_LINKS& links);
	/// @brief 狙い・休みを進め、狙い終わったフレームに撃つ。狙っている間の再入力で取りやめる
	/// @param trigger 入力を設定する（true：押した瞬間、false：それ以外）
	void Update(bool trigger) override;
	/// @brief 仮の形（腕・銃身の円柱、狙いの赤い線、撃った直後の弾の線）を描画する
	void Draw() override;
	/// @brief 待機へ戻す
	void Reset() override;
	/// @brief 待機以外なら true を返す
	bool IsActive() const override;
	/// @brief 敵をつかまないので、常に false を返す
	bool IsHolding() const override;
	/// @brief 狙っている間だけ歩く速さを下げる
	float GetMoveScale() const override;

private:
	ArmPhaseCalc m_Phase;
	float m_Seconds[2] = {}; // 狙い・休みの秒数（取りやめたときは休みが短い）。
	VEC3 m_Direction = { 0.0f, 0.0f, 1.0f }; // 狙いの向き（上下も含む。狙っている間は追いかける）。
	float m_TraceLength = 0.0f; // 撃った弾の線の長さ（撃っていなければ 0）。
	float m_Straighten = 0.0f; // 描画だけのまっすぐ度（0：待機、1：狙いの向き）。
};
