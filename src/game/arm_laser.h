#pragma once

#include "game/arm.h"
#include "physics/armphasecalc.h"

/// @file arm_laser.h
/// @brief 片手のレーザーの入力・照射と描画

/// @brief 溜めてから光線を出し、線の上の相手全員の % を小刻みに増やす腕
class LaserArm : public Arm
{
public:
	/// @brief 待機状態にする（モデルはないので読み込まない）
	/// @param hand 左 0・右 1
	/// @param links 腕が使うゲームの部品への参照
	LaserArm(int hand, const ARM_LINKS& links);
	/// @brief 溜め・照射・休みを進め、照射の刻みごとに線の上の相手へ効かせる。照射中の再入力で止める
	/// @param trigger 入力を設定する（true：押した瞬間、false：それ以外）
	void Update(bool trigger) override;
	/// @brief 仮の形（腕の円柱・溜めの球・光線の円柱）を描画する
	void Draw() override;
	/// @brief 待機へ戻す
	void Reset() override;
	/// @brief 待機以外なら true を返す
	bool IsActive() const override;
	/// @brief 敵をつかまないので、常に false を返す
	bool IsHolding() const override;
	/// @brief 溜め・照射の間だけ歩く速さを下げる
	float GetMoveScale() const override;

private:
	ArmPhaseCalc m_Phase;
	VEC3 m_Direction = { 0.0f, 0.0f, 1.0f }; // 光線の向き（照射を始めたときの狙い。上下も含む）。
	int m_Ticks = 0; // 今の照射で効かせた刻みの数。
	float m_Straighten = 0.0f; // 描画だけのまっすぐ度（0：待機、1：狙いの向き）。
};
