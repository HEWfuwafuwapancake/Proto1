#pragma once

#include "game/arm.h"
#include "physics/armphasecalc.h"
#include "physics/collision.h"
#include "physics/flailcalc.h"
#include "physics/versuscalc.h"

/// @file arm_flail.h
/// @brief 片手のモーニングスター（鎖でつないだトゲ付きの鉄球）の腕

/// @brief 単押しで前の狭い扇を振り、長押しで頭の上で回してため、離すと鉄球を投げて行きと帰りで当てる腕（ダメージは低く、吹っ飛ばしが強い）
class FlailArm : public Arm
{
public:
	/// @brief モデルを読み、待機状態にする
	/// @param hand 左 0・右 1
	/// @param links 腕が使うゲームの部品への参照
	FlailArm(int hand, const ARM_LINKS& links);
	/// @brief 振る・回す・離して投げる入力を受け取り、投げた鉄球を進める
	/// @param input このフレームの腕の入力（単押し：扇、長押し：回してため、離す：投げる）
	void Update(const ARM_INPUT& input) override;
	/// @brief 腕・鎖・鉄球を描画する
	void Draw() override;
	/// @brief 振り・回し・投げた鉄球を全部やめて待機へ戻す
	void Reset() override;
	/// @brief 振り・回しをやめる（投げた鉄球は戻るまで進める）
	void Interrupt() override;
	/// @brief 扇を振っている間か、頭の上で回している間なら true を返す
	bool IsActive() const override;
	/// @brief 投げた鉄球が戻るまで true を返す
	bool IsBusy() const override;
	/// @brief 敵をつかまないので、常に false を返す
	bool IsHolding() const override;
	/// @brief 待機中で鉄球が手元にあるときだけ単押しを受け付ける
	bool CanTakeTap() const override;
	/// @brief 待機中で鉄球が手元にあるときだけ長押しを受け付ける
	bool CanTakeHold() const override;
	/// @brief 回している間は歩く速さを半分にする
	float GetMoveScale() const override;
	/// @brief 回している間はジャンプとダッシュをできなくする
	bool BlocksJumpAndDash() const override;
	/// @brief 扇と投げにナックルの殴りのクリップを使う
	ARM_BODY_CLIP GetBodyClip() const override;

private:
	void StrikeFan();
	void StepSpin(float dt);
	void ThrowBall();
	void StepBall(float dt);
	VEC3 GetSpinCenter() const;
	VEC3 GetThrowDirection() const;
	void DrawBall(const VEC3& center, float scale) const;
	void DrawChain(const VEC3& from, const VEC3& to, float scale) const;
	ArmPhaseCalc m_Phase; // 扇の段階
	FlailCalc m_Ball; // 投げた鉄球
	HitRecord m_OutHits; // 行きで当たった相手
	HitRecord m_BackHits; // 帰りで当たった相手
	float m_SpinCooldowns[FIGHTER_COUNT] = {}; // 回している鉄球が同じ相手にまた当たるまでの秒数
	int m_ArmModel = -1;
	bool m_Spinning = false; // 頭の上で回している間 true
	float m_SpinSeconds = 0.0f; // 回し始めてからの秒数
	float m_SpinAngle = 0.0f; // 回している角度
	VEC3 m_SpinPrevious = {}; // 前のフレームの回している鉄球の位置
	bool m_FanConsumed = true; // 扇の耐久を減らし終えたら true
	bool m_ThrowHit = false; // 投げで1人でも当たったら true
	float m_ThrowPower = 0.0f; // 行きの吹っ飛ばす強さ（ためた分）
	VEC3 m_ThrowDirection = { 0.0f, 0.0f, 1.0f }; // 投げた向き
	float m_ThrowClipSeconds = -1.0f; // 投げのクリップを見せている秒数（見せていなければ -1。描画だけ）
	VEC3 m_Direction = { 0.0f, 0.0f, 1.0f }; // 扇を振る水平の向き
	float m_Straighten = 0.0f; // 描画用のまっすぐ度
};
