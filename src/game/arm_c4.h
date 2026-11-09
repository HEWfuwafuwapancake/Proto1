#pragma once

#include "game/arm.h"
#include "physics/armphasecalc.h"
#include "physics/c4calc.h"

/// @file arm_c4.h
/// @brief 片手の C4 の入力・投げる・起爆と描画

/// @brief 長押しで山なりに投げて設置し、単押しで設置済みを全部起爆する腕
class C4Arm : public Arm
{
public:
	/// @brief モデルを読み、待機状態にする
	/// @param hand 左 0・右 1
	/// @param links 腕が使うゲームの部品への参照
	C4Arm(int hand, const ARM_LINKS& links);
	/// @brief 投げる・起爆の入力を受け取り、弾を進める
	/// @param input このフレームの腕の入力（単押しで起爆、長押しで投げる）
	void Update(const ARM_INPUT& input) override;
	/// @brief 腕・手の C4・飛んでいる C4・設置済みを描画する
	void Draw() override;
	/// @brief 投げた C4 も設置済みも消して待機へ戻す
	void Reset() override;
	/// @brief 投げる動きだけやめる（飛んでいる C4 と設置済みは残して進める）
	void Interrupt() override;
	/// @brief 投げる動きの間なら true を返す
	bool IsActive() const override;
	/// @brief 敵をつかまないので、常に false を返す
	bool IsHolding() const override;

private:
	void ThrowBomb();
	void Detonate();
	void StepBombs(float dt);
	void DrawBomb(const VEC3& feet, const XMFLOAT4& color) const;
	C4Calc m_Bombs;
	ArmPhaseCalc m_Phase;
	int m_ArmModel = -1;
	int m_BodyModel = -1;
	VEC3 m_Direction = { 0.0f, 0.0f, 1.0f }; // 投げた水平の向き（描画用）。
	float m_Straighten = 0.0f; // 描画だけのまっすぐ度（0：待機、1：投げる向き）。
	float m_RangeSeconds = 0.0f; // 爆発の範囲を見せる残りの秒数。
	VEC3 m_RangeCenters[C4_MAX] = {}; // 爆発の範囲の中心。
	int m_RangeCount = 0; // 爆発の範囲の数。
};
