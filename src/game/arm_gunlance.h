#pragma once

#include "game/arm.h"
#include "physics/gunlancecalc.h"
#include "physics/projectilecalc.h"

/// @file arm_gunlance.h
/// @brief 片手のガンランスの入力・ためて発射・低速の突進と描画

/// @brief 単押しで段階をためて爆発と弾を出し、長押しの間は遅く突進する腕
class GunlanceArm : public Arm
{
public:
	/// @brief モデルを読み、待機状態にする
	/// @param hand 左 0・右 1
	/// @param links 腕が使うゲームの部品への参照
	GunlanceArm(int hand, const ARM_LINKS& links);
	/// @brief ため・発射・突進・弾を進める
	/// @param input このフレームの腕の入力（単押しでためる、長押しの間は突進）
	void Update(const ARM_INPUT& input) override;
	/// @brief 腕・槍・砲身・飛んでいる弾を描画する
	void Draw() override;
	/// @brief ため・突進・弾を消して待機へ戻す
	void Reset() override;
	/// @brief ためと突進をやめる（飛んでいる弾は残して進める）
	void Interrupt() override;
	/// @brief ため・突進・発射の構えの間なら true を返す
	bool IsActive() const override;
	/// @brief 敵をつかまないので、常に false を返す
	bool IsHolding() const override;
	/// @brief 突進の間を除いて単押しを受け付ける（ためている間も段階を足せる）
	bool CanTakeTap() const override;
	/// @brief 突進しておらず、ためてもいないなら true を返す
	bool CanTakeHold() const override;
	/// @brief 射撃と突進の上半身の攻撃クリップを取得する
	ARM_BODY_CLIP GetBodyClip() const override;
	/// @brief 突進の間のワールド速度を取得する
	VEC3 GetRushVelocity() const override;

private:
	void Fire(int level);
	void StepRush(const ARM_INPUT& input, float dt);
	void StepShots(float dt);
	GunlanceCalc m_Calc;
	ProjectileCalc m_Shots;
	int m_ShotLevels[PROJECTILE_MAX] = {}; // 弾ごとの段階。
	int m_ArmModel = -1;
	int m_LanceModel = -1;
	int m_BarrelModel = -1;
	bool m_Rushing = false;
	float m_RushTime = 0.0f; // 突進を始めてからの秒数。
	VEC3 m_Direction = { 0.0f, 0.0f, 1.0f }; // 構えと突進の水平の向き。
	float m_Straighten = 0.0f; // 描画だけのまっすぐ度（0：待機、1：構えの向き）。
	float m_PoseSeconds = 0.0f; // 発射のあと構えを保つ残りの秒数。
	float m_RangeSeconds = 0.0f; // 爆発の範囲を見せる残りの秒数。
	VEC3 m_RangeCenter = {}; // 爆発の範囲の中心。
};
