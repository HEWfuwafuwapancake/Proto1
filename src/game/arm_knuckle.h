#pragma once

#include "game/arm.h"
#include "physics/armphasecalc.h"

/// @file arm_knuckle.h
/// @brief 片手のナックルの入力・打撃と描画（パイルバンカーをまとめた腕）

/// @brief 拳で殴る腕。殴った瞬間に相手が至近なら杭が出て強くなる
class KnuckleArm : public Arm
{
public:
	/// @brief 待機状態にする（モデルはないので読み込まない）
	/// @param hand 左 0・右 1
	/// @param links 腕が使うゲームの部品への参照
	KnuckleArm(int hand, const ARM_LINKS& links);
	/// @brief 引き・打撃・休みを進め、打撃に入ったフレームに当たりを調べる
	/// @param trigger 入力を設定する（true：押した瞬間、false：それ以外）
	void Update(bool trigger) override;
	/// @brief 仮の形（腕の円柱・拳の箱・杭の円柱）を描画する
	void Draw() override;
	/// @brief 待機へ戻す
	void Reset() override;
	/// @brief 待機以外なら true を返す
	bool IsActive() const override;
	/// @brief 敵をつかまないので、常に false を返す
	bool IsHolding() const override;

private:
	ArmPhaseCalc m_Phase;
	float m_Seconds[3] = {}; // 引き・打撃・休みの秒数（杭のときは休みが長い）。
	VEC3 m_Direction = { 0.0f, 0.0f, 1.0f }; // 殴る水平の向き。
	bool m_Pile = false; // この打撃で杭を出したか。
	float m_Straighten = 0.0f; // 描画だけのまっすぐ度（0：待機、1：殴る向き）。
};
