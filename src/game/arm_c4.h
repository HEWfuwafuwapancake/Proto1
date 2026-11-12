#pragma once

#include "game/arm.h"
#include "physics/armphasecalc.h"
#include "physics/c4calc.h"

/// @file arm_c4.h
/// @brief 片手の C4 の投擲・着弾爆発と描画

/// @brief 単押しで通常の C4、長押しでためて大型の C4 を投げ、着弾した瞬間に爆発させる腕
class C4Arm : public Arm
{
public:
	/// @brief モデルを読み、待機状態にする
	/// @param hand 左 0・右 1
	/// @param links 腕が使うゲームの部品への参照
	C4Arm(int hand, const ARM_LINKS& links);
	/// @brief 投げる入力を受け取り、弾を進めて着弾時に爆発させる
	/// @param input このフレームの腕の入力（単押し：通常、長押し：大型）
	void Update(const ARM_INPUT& input) override;
	/// @brief 腕・手元と飛行中の C4・爆発の範囲を描画する
	void Draw() override;
	/// @brief 飛んでいる C4 も消して待機へ戻す
	void Reset() override;
	/// @brief 投げる動きだけやめる（飛行中の C4 は残して進める）
	void Interrupt() override;
	/// @brief ためている間か投げたあとの構えの間なら true を返す
	bool IsActive() const override;
	/// @brief 飛行中の弾か爆発の表示が残っていれば true を返す
	bool IsBusy() const override;
	/// @brief 敵をつかまないので、常に false を返す
	bool IsHolding() const override;
	/// @brief 大型の爆弾をためている間だけ歩く速さを半分にする
	float GetMoveScale() const override;

private:
	void ThrowBomb(bool large);
	void StepBombs(float dt);
	void DrawBomb(const VEC3& feet, bool large, const XMFLOAT4& color) const;
	C4Calc m_Bombs;
	ArmPhaseCalc m_Phase;
	int m_ArmModel = -1;
	int m_BodyModel = -1;
	VEC3 m_Direction = { 0.0f, 0.0f, 1.0f }; // 投げる水平の向き（描画用）
	float m_Straighten = 0.0f; // 描画用のまっすぐ度
	float m_RangeSeconds = 0.0f; // 爆発の範囲を見せる残り秒数
	VEC3 m_RangeCenters[PROJECTILE_MAX] = {}; // 爆発した場所
	float m_RangeRadii[PROJECTILE_MAX] = {}; // 爆発ごとの半径
	int m_RangeCount = 0; // 表示する爆発の数
};
