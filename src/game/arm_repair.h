#pragma once

#include "game/arm.h"
#include "physics/armphasecalc.h"

/// @file arm_repair.h
/// @brief 片手のリペアの入力・修理と描画

/// @brief その場で止まって直し、最後まで直せたら自分のダメージ%を減らす腕
class RepairArm : public Arm
{
public:
	/// @brief 待機状態にする（モデルはないので読み込まない）
	/// @param hand 左 0・右 1
	/// @param links 腕が使うゲームの部品への参照
	RepairArm(int hand, const ARM_LINKS& links);
	/// @brief 修理・休みを進め、最後まで直せたフレームに % を減らす。修理中の再入力で取りやめる
	/// @param trigger 入力を設定する（true：押した瞬間、false：それ以外）
	void Update(bool trigger) override;
	/// @brief 仮の形（胸の前へ寄せた腕と、修理中の緑の球）を描画する
	void Draw() override;
	/// @brief 待機へ戻す
	void Reset() override;
	/// @brief 修理している間だけ true を返す
	bool IsActive() const override;
	/// @brief 敵をつかまないので、常に false を返す
	bool IsHolding() const override;
	/// @brief 修理している間は歩けない（0 を返す）
	float GetMoveScale() const override;

private:
	ArmPhaseCalc m_Phase;
	float m_Straighten = 0.0f; // 描画だけのまっすぐ度（0：待機、1：胸の前）。
};
