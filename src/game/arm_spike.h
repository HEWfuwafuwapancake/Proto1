#pragma once

#include "game/arm.h"
#include "physics/armphasecalc.h"
#include "physics/collision.h"
#include "physics/spikecalc.h"

/// @file arm_spike.h
/// @brief 片手のトゲの腕：地面を殴って前へトゲを生やす・周りに茨を置く

/// @brief 単押しで前へ手前から順にトゲを生やし、長押しで周りに数秒残る茨を置く腕（空中では急降下してから殴る）
class SpikeArm : public Arm
{
public:
	/// @brief モデルを読み、待機状態にする
	/// @param hand 左 0・右 1
	/// @param links 腕が使うゲームの部品への参照
	SpikeArm(int hand, const ARM_LINKS& links);
	/// @brief 殴る入力を受け取り、トゲの列と茨を進める
	/// @param input このフレームの腕の入力（単押し：前へトゲ、長押し：茨）
	void Update(const ARM_INPUT& input) override;
	/// @brief 腕とこぶしを描画する（トゲの列と茨は DrawLeftover で描く）
	void Draw() override;
	/// @brief 殴る動きをやめ、トゲの列と茨も全部消す
	void Reset() override;
	/// @brief 殴る動きだけやめる（生えているトゲと茨は残して進める）
	void Interrupt() override;
	/// @brief 殴る動きだけやめる（生えているトゲと茨は残し、外したあとも進める）
	void Unequip() override;
	/// @brief 外したあとも、生えているトゲと茨を進める
	void UpdateLeftover() override;
	/// @brief 生えているトゲの列と茨を描画する
	void DrawLeftover() override;
	/// @brief 生えているトゲの列と茨を全部消す
	void ClearLeftover() override;
	/// @brief 急降下・ため・殴り・休みの間なら true を返す
	bool IsActive() const override;
	/// @brief 敵をつかまないので、常に false を返す
	bool IsHolding() const override;
	/// @brief 殴る動きの間は歩く速さを落とす
	float GetMoveScale() const override;
	/// @brief 単押し・長押しに対応する体のクリップ（ハンマーの叩き・叩きつけを使う）を取得する
	ARM_BODY_CLIP GetBodyClip() const override;

private:
	const float* GetSeconds() const;
	void StopAction();
	void StopDive();
	void StepDive(float dt);
	void Strike();
	void StepEffects(float dt);
	void StepLine(float dt);
	void StepFields(float dt);
	void DrawSpike(const VEC3& base, float height, float radius, const VEC3& lean, const XMFLOAT4& color) const;
	ArmPhaseCalc m_Phase;
	ThornFieldCalc m_Fields; // 置いた茨
	HitRecord m_LineHits; // トゲの列に1回当たった相手
	int m_ArmModel = -1;
	int m_FistModel = -1;
	bool m_Hold = false; // 今の動作が長押し（茨）なら true
	bool m_Diving = false; // 空中から急降下している間 true
	VEC3 m_Direction = { 0.0f, 0.0f, 1.0f }; // 殴る水平の向き
	float m_Straighten = 0.0f; // 描画用のまっすぐ度
	bool m_LineActive = false; // トゲの列が出ている間 true
	bool m_LineConsumed = true; // トゲの列の耐久を減らし終えたら true
	VEC3 m_LineFeet = {}; // トゲの列を出した足元
	VEC3 m_LineDirection = { 0.0f, 0.0f, 1.0f }; // トゲの列の水平の向き
	int m_LineCount = 0; // トゲの列の本数
	float m_LineElapsed = 0.0f; // トゲの列を出してからの秒数
};
