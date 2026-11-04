#pragma once

#include "game/arm.h"
#include "physics/armphasecalc.h"

/// @file arm_axe.h
/// @brief 片手のアックスの入力・横振り（ガード崩し）と描画

/// @brief 振りかぶって横に大きく振る腕。ガードしている相手はガードを崩してしびれさせる
class AxeArm : public Arm
{
public:
	/// @brief 待機状態にする（モデルはないので読み込まない）
	/// @param hand 左 0・右 1
	/// @param links 腕が使うゲームの部品への参照
	AxeArm(int hand, const ARM_LINKS& links);
	/// @brief 振りかぶり・振り・休みを進め、振りに入ったフレームに扇の中の相手へ効かせる
	/// @param trigger 入力を設定する（true：押した瞬間、false：それ以外）
	void Update(bool trigger) override;
	/// @brief 仮の形（腕・柄の円柱と刃の箱）を、振りに合わせて横へ回して描画する
	void Draw() override;
	/// @brief 待機へ戻す
	void Reset() override;
	/// @brief 待機以外なら true を返す
	bool IsActive() const override;
	/// @brief 敵をつかまないので、常に false を返す
	bool IsHolding() const override;
	/// @brief 振りかぶっている間だけ歩く速さを下げる
	float GetMoveScale() const override;

private:
	ArmPhaseCalc m_Phase;
	VEC3 m_Direction = { 0.0f, 0.0f, 1.0f }; // 振る扇の中心の水平の向き。
	float m_Straighten = 0.0f; // 描画だけのまっすぐ度（0：待機、1：振る向き）。
};
