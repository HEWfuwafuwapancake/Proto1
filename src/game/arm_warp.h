#pragma once

#include "game/arm.h"
#include "physics/armphasecalc.h"

/// @file arm_warp.h
/// @brief 片手のワープの入力・瞬間移動と描画

/// @brief ためてから、ロックオンした相手の手前か正面へ瞬間移動する腕
class WarpArm : public Arm
{
public:
	/// @brief 待機状態にする（モデルはないので読み込まない）
	/// @param hand 左 0・右 1
	/// @param links 腕が使うゲームの部品への参照
	WarpArm(int hand, const ARM_LINKS& links);
	/// @brief ため・休みを進め、ため終わったフレームに瞬間移動する
	/// @param trigger 入力を設定する（true：押した瞬間、false：それ以外）
	void Update(bool trigger) override;
	/// @brief 仮の形（腕の円柱と、ためている間に大きくなる球）を描画する
	void Draw() override;
	/// @brief 待機へ戻す
	void Reset() override;
	/// @brief ためている間だけ true を返す（休みの間はキャラの向きを縛らない）
	bool IsActive() const override;
	/// @brief 敵をつかまないので、常に false を返す
	bool IsHolding() const override;

private:
	ArmPhaseCalc m_Phase;
	float m_Straighten = 0.0f; // 描画だけのまっすぐ度（0：待機、1：正面）。
};
