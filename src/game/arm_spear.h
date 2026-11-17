#pragma once

#include "game/arm.h"
#include "physics/armphasecalc.h"
#include "physics/spearcalc.h"

/// @file arm_spear.h
/// @brief 片手の槍の突き・投げと、刺さった相手への突っ込み、描画

/// @brief 単押しで中距離の直線上を突き、長押しで槍を投げて刺さった相手へ突っ込む腕
/// @details 刺さった相手は吹っ飛び、吹っ飛んでいる間はしびれる。相手が浮島から落ちたら突っ込まない（突っ込んでいる途中でもやめる）。
class SpearArm : public Arm
{
public:
	/// @brief モデルを読み、待機状態にする
	/// @param hand 左 0・右 1
	/// @param links 腕が使うゲームの部品への参照
	SpearArm(int hand, const ARM_LINKS& links);
	/// @brief 構え・突き・投げ・休みを進め、投げた槍と突っ込みを進める
	/// @param input このフレームの腕の入力（単押しで突き、長押しで構えて離すと投げる）
	void Update(const ARM_INPUT& input) override;
	/// @brief 腕と槍（手の中・飛んでいる・刺さっている）を描画する
	void Draw() override;
	/// @brief 待機へ戻す（槍を手に戻し、突っ込んでいる途中なら止めていた体の計算も戻す）
	void Reset() override;
	/// @brief 構え・突き・休みの間と、槍が飛んでいる・刺さっている間なら true を返す
	bool IsActive() const override;
	/// @brief 敵をつかまないので、常に false を返す
	bool IsHolding() const override;
	/// @brief 突きは上半身、投げは上半身、突っ込みは全身の攻撃クリップを取得する
	ARM_BODY_CLIP GetBodyClip() const override;
	/// @brief 投げる構えの間だけ歩く速さを下げる
	float GetMoveScale() const override;
	/// @brief 刺さった相手へ突っ込んでいる間は、ジャンプとダッシュをできなくするので true を返す
	bool BlocksJumpAndDash() const override;

private:
	void Thrust();
	void StepSpear(float dt);
	void OnStuck();
	void StepRush(float dt);
	void EndRush();
	void Consume(ARM_COST_ITEM item);
	VEC3 GetStuckTip() const;
	ArmPhaseCalc m_Phase;
	SpearCalc m_Spear;
	bool m_Throw = false; // true：長押しの投げ、false：単押しの突き
	bool m_UseConsumed = false; // 今の動作で耐久を消費したか
	bool m_Rushing = false; // true：刺さった相手へ突っ込んでいる
	bool m_OwnPull = false; // true：突っ込むために自分で体の計算を止めている
	float m_StuckSeconds = 0.0f; // 刺さってからの秒数
	VEC3 m_Direction = { 0.0f, 0.0f, 1.0f }; // 攻撃の水平の向き
	int m_ArmModel = -1;
	float m_Straighten = 0.0f; // 描画だけのまっすぐ度（0：待機、1：攻撃の向き）
	float m_Extend = 0.0f; // 描画だけの突きの伸び（0：手元、1：突ききった）
};
