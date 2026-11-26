#pragma once

#include "game/arm.h"
#include "physics/armphasecalc.h"
#include "physics/combocalc.h"
#include "physics/risecalc.h"

/// @file arm_knuckle.h
/// @brief 片手のナックルの入力・殴り・突進と描画

/// @brief 単押しで3段の殴り（2回殴って、3段目のアッパーで斜め上へ飛ばす）、長押しでためて前へ突進する腕（空中の突進は前斜め上へ進み、場外から戻れる）
class KnuckleArm : public Arm
{
public:
	/// @brief モデルを読み、待機状態にする
	/// @param hand 左 0・右 1
	/// @param links 腕が使うゲームの部品への参照
	KnuckleArm(int hand, const ARM_LINKS& links);
	/// @brief ため・攻撃・休みを進め、殴りと突進の当たりを調べる
	/// @param input このフレームの腕の入力（単押しで殴る、長押しで突進）
	void Update(const ARM_INPUT& input) override;
	/// @brief 腕と拳を描画する（殴る間は腕を伸ばす）
	void Draw() override;
	/// @brief 待機へ戻す（空中の突進の途中なら、止めていた体の計算も戻す）
	void Reset() override;
	/// @brief 待機以外なら true を返す
	bool IsActive() const override;
	/// @brief 敵をつかまないので、常に false を返す
	bool IsHolding() const override;
	/// @brief 単押しの1・2段目は上半身、アッパーと長押しは全身の攻撃クリップを取得する
	ARM_BODY_CLIP GetBodyClip() const override;
	/// @brief 待機中と、段階攻撃の次の段を受け付けている休みの間は true を返す（アッパーのあとの攻撃できない間は false）
	bool CanTakeTap() const override;
	/// @brief 待機中は true を返す（アッパーのあとの攻撃できない間は false）
	bool CanTakeHold() const override;
	/// @brief 突進のためと休みの間だけ歩く速さを下げる
	float GetMoveScale() const override;
	/// @brief 地上の突進の間のワールド速度を取得する（空中の突進は自分で体を動かすのでゼロ）
	VEC3 GetRushVelocity() const override;

private:
	int FindTarget(float reach, float radius) const;
	VEC3 GetPunchDirection() const; // 突きの線の向き（空中の突進は進む向き、それ以外は狙う相手の高さへ）
	void StrikePillarsOnPunch(float reach, float radius, float damage) const; // 突きの線にかかる柱に当てる
	const float* GetSeconds() const;
	bool IsOffStage() const;
	float GetAirRushPitch() const;
	void Strike(int target, float up, float damage, float power);
	void StopAirRush();
	ArmPhaseCalc m_Phase;
	RiseCalc m_Rise; // 空中の突進で体を動かす計算
	bool m_AirRush = false; // true：空中の突進で体を自分で動かしている
	float m_AirRushPitch = 0.0f; // 空中の突進で上へ傾けた角度（ラジアン）
	bool m_AirRushClip = false; // 長押し開始時に決め、休みが終わるまで保つ空中クリップの指定（描画だけ）
	bool m_UseConsumed = false; // 今の動作で耐久を消費したか
	int m_ArmModel = -1;
	int m_FistModel = -1;
	bool m_Rush = false; // true：長押しの突進、false：単押しの殴り
	ComboCalc m_Combo; // 単押しの段階攻撃の段と受付
	int m_TapStep = 0; // 今の単押しの段（0：1段目、1：2段目、2：アッパー）
	VEC3 m_Direction = { 0.0f, 0.0f, 1.0f }; // 攻撃の水平の向き。
	float m_Straighten = 0.0f; // 描画だけのまっすぐ度（0：待機、1：攻撃の向き）。
	float m_Extend = 0.0f; // 描画だけの腕の伸び（0：元の長さ、1：殴りの届く長さ）。
};
