#pragma once

#include "game/arm.h"
#include "physics/armphasecalc.h"
#include "physics/risecalc.h"

/// @file arm_hammer.h
/// @brief 片手のハンマーの入力・叩きつけ・衝撃波と描画

/// @brief 単押しで前を強く叩き、長押しで長くためて衝撃波としびれを出す腕（空中の単押しは短いためで振り、上へ跳ねて場外から戻れる）
class HammerArm : public Arm
{
public:
	/// @brief モデルを読み、待機状態にする
	/// @param hand 左 0・右 1
	/// @param links 腕が使うゲームの部品への参照
	HammerArm(int hand, const ARM_LINKS& links);
	/// @brief ため・叩き・休みを進め、叩きに入ったフレームに範囲へ効かせる
	/// @param input このフレームの腕の入力（単押しで強打、長押しで衝撃波）
	void Update(const ARM_INPUT& input) override;
	/// @brief 腕とハンマーを、ため・叩きに合わせて上下させて描画する
	void Draw() override;
	/// @brief 待機へ戻す（跳ねている途中なら、止めていた体の計算も戻す）
	void Reset() override;
	/// @brief 待機以外なら true を返す
	bool IsActive() const override;
	/// @brief 敵をつかまないので、常に false を返す
	bool IsHolding() const override;
	/// @brief 単押しは上半身、長押しは全身の攻撃クリップを取得する
	ARM_BODY_CLIP GetBodyClip() const override;
	/// @brief ため・叩き・休みの間だけ歩く速さを下げる
	float GetMoveScale() const override;

private:
	void Impact();
	const float* GetSeconds() const;
	void StartHop();
	void StopHop();
	ArmPhaseCalc m_Phase;
	RiseCalc m_Rise; // 空中の単押しで体を上へ動かす計算
	bool m_AirTap = false; // true：空中で始めた単押し（短いため。振った瞬間に跳ねる）
	bool m_Hopping = false; // true：跳ねるために体を自分で動かしている
	bool m_UseConsumed = false; // 今の動作で使用回数を消費したか
	int m_ArmModel = -1;
	int m_HeadModel = -1;
	bool m_Wave = false; // true：長押しの衝撃波、false：単押しの強打
	VEC3 m_Direction = { 0.0f, 0.0f, 1.0f }; // 叩く水平の向き。
	float m_Straighten = 0.0f; // 描画だけのまっすぐ度（0：待機、1：叩く向き）。
	float m_RangeSeconds = 0.0f; // 当たる範囲を見せる残りの秒数。
	VEC3 m_RangeCenter = {}; // 当たる範囲の中心。
	float m_RangeRadius = 0.0f; // 当たる範囲の半径。
};
