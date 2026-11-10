#pragma once

#include "game/arm.h"
#include "physics/gunlancecalc.h"
#include "physics/projectilecalc.h"

/// @file arm_gunlance.h
/// @brief 片手のガンランスの単発射撃・突進・撃ち込みと描画

/// @brief 単押しで弾を撃ち、長押しで突進して相手を突き上げる腕
class GunlanceArm : public Arm
{
public:
	/// @brief モデルを読み、待機状態にする
	/// @param hand 左 0・右 1
	/// @param links 腕が使うゲームの部品への参照
	GunlanceArm(int hand, const ARM_LINKS& links);
	/// @brief 射撃・突進・突き上げ・撃ち込みと飛んでいる弾を進める
	/// @param input このフレームの腕の入力（単押しで射撃、長押しで突進）
	void Update(const ARM_INPUT& input) override;
	/// @brief 腕・槍・砲身・飛んでいる弾を描画する
	void Draw() override;
	/// @brief 相手を離し、動作と弾を消して待機へ戻す
	void Reset() override;
	/// @brief 相手を離して動作を中断する（飛んでいる弾は残して進める）
	void Interrupt() override;
	/// @brief 突進・突き上げ・撃ち込み・発射の反動の間なら true を返す
	bool IsActive() const override;
	/// @brief 飛んでいる弾が残っていれば true を返す
	bool IsBusy() const override;
	/// @brief 突き上げ・撃ち込みで相手をつかんでいれば true を返す
	bool IsHolding() const override;
	/// @brief 次の単発射撃ができるときに true を返す
	bool CanTakeTap() const override;
	/// @brief 新しく突進を始められるときに true を返す
	bool CanTakeHold() const override;
	/// @brief 射撃・突進・突き上げの上半身の攻撃クリップを取得する
	ARM_BODY_CLIP GetBodyClip() const override;
	/// @brief 相手をつかんでいる間は歩く速さを 0 にする
	float GetMoveScale() const override;
	/// @brief 突進の間のワールド速度を取得する
	VEC3 GetRushVelocity() const override;

private:
	void Fire();
	void StepRush();
	void StepImpale(const GUNLANCE_STEP& step);
	void ReleaseHeld();
	void StepShots(float dt);
	GunlanceCalc m_Calc;
	ProjectileCalc m_Shots;
	int m_ArmModel = -1;
	int m_LanceModel = -1;
	int m_BarrelModel = -1;
	int m_HeldTarget = -1;
	VEC3 m_Direction = { 0.0f, 0.0f, 1.0f }; // 射撃や突進の向き。
	float m_Straighten = 0.0f; // 描画だけのまっすぐ度（0：待機、1：構えの向き）。
	float m_PoseSeconds = 0.0f; // 発射のあと構えを保つ残りの秒数。
	float m_RecoilSeconds = 0.0f; // 撃ち込みの反動を描く残りの秒数。
};
