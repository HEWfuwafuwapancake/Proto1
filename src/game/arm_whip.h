#pragma once

#include "game/arm.h"
#include "physics/armphasecalc.h"

/// @file arm_whip.h
/// @brief 片手のウィップの入力・横振りと描画（チェーンをまとめた腕）

/// @brief 鞭を横に大きく振り、遠くまでの扇の相手を横へ吹っ飛ばす腕
class WhipArm : public Arm
{
public:
	/// @brief 待機状態にする（モデルはないので読み込まない）
	/// @param hand 左 0・右 1
	/// @param links 腕が使うゲームの部品への参照
	WhipArm(int hand, const ARM_LINKS& links);
	/// @brief 溜め・振り・休みを進め、振りに入ったフレームに扇の中の相手へ効かせる
	/// @param trigger 入力を設定する（true：押した瞬間、false：それ以外）
	void Update(bool trigger) override;
	/// @brief 仮の形（腕の円柱と、球をつないだ鞭）を描画する。振る間は鞭が扇を横切る
	void Draw() override;
	/// @brief 待機へ戻す
	void Reset() override;
	/// @brief 待機以外なら true を返す
	bool IsActive() const override;
	/// @brief 敵をつかまないので、常に false を返す
	bool IsHolding() const override;

private:
	float GetSweepSign() const;
	ArmPhaseCalc m_Phase;
	VEC3 m_Direction = { 0.0f, 0.0f, 1.0f }; // 振る扇の中心の水平の向き。
	float m_Straighten = 0.0f; // 描画だけのまっすぐ度（0：待機、1：振る向き）。
};
