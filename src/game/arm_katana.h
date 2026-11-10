#pragma once

#include "game/arm.h"
#include "physics/armphasecalc.h"
#include "physics/collision.h"

/// @file arm_katana.h
/// @brief 片手の刀の入力・居合・狙った場所へのワープと描画

/// @brief 単押しで納刀の構えから居合で斬り、長押しで着地点を狙って離すと一直線に飛ぶ腕
class KatanaArm : public Arm
{
public:
	/// @brief モデルを読み、待機状態にする
	/// @param hand 左 0・右 1
	/// @param links 腕が使うゲームの部品への参照
	KatanaArm(int hand, const ARM_LINKS& links);
	/// @brief 居合とワープの段階を進める
	/// @param input このフレームの腕の入力（単押しで居合、長押しで狙い、離すと飛ぶ）
	void Update(const ARM_INPUT& input) override;
	/// @brief 腕と拳、握った刀（構えの間は鞘も）、狙いの輪と裂け目を描画する
	void Draw() override;
	/// @brief 待機へ戻す（飛んでいる途中なら引っぱられ状態も解く）
	void Reset() override;
	/// @brief 待機以外なら true を返す
	bool IsActive() const override;
	/// @brief 敵をつかまないので、常に false を返す
	bool IsHolding() const override;
	/// @brief 居合の間の歩く速さの倍率を取得する
	float GetMoveScale() const override;
	/// @brief 居合の一閃で踏み込むワールド速度を取得する
	VEC3 GetRushVelocity() const override;
	/// @brief 構えの出だしと飛んでいる間は全方向のガードを取得する
	GUARD GetGuard() const override;

private:
	enum BREACH_STATE { BREACH_NONE, BREACH_AIM, BREACH_DASH, BREACH_RECOVER };
	void StartIai();
	void BeginDraw();
	void SlashTargets();
	void StepIai(float dt);
	void StepBreach(const ARM_INPUT& input, float dt);
	void UpdateLanding();
	void StartDash();
	void StopDash();
	void GetPose(VEC3& arm, VEC3& sword, bool& sheathed) const;
	void DrawRift(const VEC3& feet) const;
	ArmPhaseCalc m_Phase; // 居合の構え → 一閃 → 残心
	HitRecord m_Hits; // 1回の一閃で当たった相手
	BREACH_STATE m_Breach = BREACH_NONE;
	float m_BreachSeconds = 0.0f; // 飛び終わりの休みの残りの秒数。
	int m_ArmModel = -1;
	int m_FistModel = -1;
	int m_BladeModel = -1;
	int m_GripModel = -1;
	int m_SheathModel = -1;
	bool m_Countered = false; // true：構えで攻撃を受け流した強い一閃、false：通常の一閃
	bool m_HasLanding = false; // 狙っている着地点があるか
	VEC3 m_Landing = {}; // 飛ぶ先の足元の位置。
	VEC3 m_Direction = { 0.0f, 0.0f, 1.0f }; // 斬る水平の向き。飛んでいる間は飛ぶ向き。
	float m_Straighten = 0.0f; // 描画だけの構え度（0：待機、1：動作の形）。
	float m_RiftSeconds = 0.0f; // 裂け目を見せる残りの秒数。
	VEC3 m_RiftStart = {}; // 飛び始めの裂け目の足元。
	VEC3 m_RiftEnd = {}; // 飛ぶ先の裂け目の足元。
};
