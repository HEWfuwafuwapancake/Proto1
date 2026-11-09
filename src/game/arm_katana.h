#pragma once

#include "game/arm.h"
#include "physics/armphasecalc.h"

/// @file arm_katana.h
/// @brief 片手の刀の入力・薙ぎ払い・進みながらの連続斬りと描画

/// @brief 単押しで前を扇形に斬り、長押しの間は進みながら連続で斬る腕
class KatanaArm : public Arm
{
public:
	/// @brief モデルを読み、待機状態にする
	/// @param hand 左 0・右 1
	/// @param links 腕が使うゲームの部品への参照
	KatanaArm(int hand, const ARM_LINKS& links);
	/// @brief 斬りの段階を進め、範囲の相手へ効かせる
	/// @param input このフレームの腕の入力（単押しで薙ぎ払い、長押しの間は連続斬り）
	void Update(const ARM_INPUT& input) override;
	/// @brief 腕と刀を、斬りに合わせて左右に振って描画する
	void Draw() override;
	/// @brief 待機へ戻す
	void Reset() override;
	/// @brief 待機以外なら true を返す
	bool IsActive() const override;
	/// @brief 敵をつかまないので、常に false を返す
	bool IsHolding() const override;
	/// @brief 連続斬りの間のワールド速度を取得する
	VEC3 GetRushVelocity() const override;

private:
	bool IsRushing() const;
	void Slash(float damage, float power);
	void AddCutDamage();
	void FinishRush();
	ArmPhaseCalc m_Phase;
	int m_ArmModel = -1;
	int m_BladeModel = -1;
	bool m_Rush = false; // true：長押しの連続斬り、false：単押しの薙ぎ払い
	VEC3 m_Direction = { 0.0f, 0.0f, 1.0f }; // 斬る水平の向き。
	float m_Straighten = 0.0f; // 描画だけのまっすぐ度（0：待機、1：斬る向き）。
};
