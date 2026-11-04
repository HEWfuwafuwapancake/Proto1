#pragma once

#include "game/arm.h"
#include "physics/bladecalc.h"

/// @file arm_blade.h
/// @brief 片手のブレードの入力・連続斬りと描画（ビームサーベルをまとめた腕）

/// @brief 刃で前の扇を斬る腕。続けて押すと3回まで振り、3回目で吹っ飛ばす
class BladeArm : public Arm
{
public:
	/// @brief 待機状態にする（モデルはないので読み込まない）
	/// @param hand 左 0・右 1
	/// @param links 腕が使うゲームの部品への参照
	BladeArm(int hand, const ARM_LINKS& links);
	/// @brief 連続斬りを進め、振りが当たるフレームに扇の中の相手へ効かせる
	/// @param trigger 入力を設定する（true：押した瞬間、false：それ以外）
	void Update(bool trigger) override;
	/// @brief 仮の形（腕と刃の円柱）を、振りに合わせて横へ回して描画する
	void Draw() override;
	/// @brief 待機へ戻す
	void Reset() override;
	/// @brief 待機以外なら true を返す
	bool IsActive() const override;
	/// @brief 敵をつかまないので、常に false を返す
	bool IsHolding() const override;

private:
	BladeCalc m_Blade;
	VEC3 m_Direction = { 0.0f, 0.0f, 1.0f }; // 斬る水平の向き（1振り目を押したとき）。
	float m_Straighten = 0.0f; // 描画だけのまっすぐ度（0：待機、1：斬る向き）。
};
