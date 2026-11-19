#pragma once

#include "game/arm.h"
#include "physics/armphasecalc.h"
#include "physics/missilecalc.h"
#include "physics/projectilecalc.h"

/// @file arm_missile.h
/// @brief 片手のミサイル腕：指と腕を相手を追う弾として撃つ

/// @brief 単押しで指を1本ずつ（5本のあとは腕ごと）、長押しで残りを全部、相手を追う弾として撃つ腕
/// @details 弾は敵か浮島・高台に当たるか、寿命が来ると爆発する。腕を撃つと耐久を使い切る。
///          撃った弾は、中断・腕を外す・壊れても飛び続ける（本人の復活・試合の始め・やり直しで消える）
class MissileArm : public Arm
{
public:
	/// @brief 指を全部付けた待機状態にする（モデルはなく、箱と球で描く）
	/// @param hand 左 0・右 1
	/// @param links 腕が使うゲームの部品への参照
	MissileArm(int hand, const ARM_LINKS& links);
	/// @brief 撃つ入力を受け取り、撃った弾と爆発を進める
	/// @param input このフレームの腕の入力（単押し：1回ぶん撃つ、長押し：離した瞬間に残りを全部撃つ）
	void Update(const ARM_INPUT& input) override;
	/// @brief 腕と残っている指を描画する（弾と爆発は DrawLeftover で描く）
	void Draw() override;
	/// @brief 撃つ動きをやめて指と腕を戻し、飛んでいる弾と爆発も消す
	void Reset() override;
	/// @brief 撃つ動きだけやめる（飛んでいる弾は残す）
	void Interrupt() override;
	/// @brief 撃つ動きをやめて指と腕を戻す（飛んでいる弾は残し、外したあとも進める）
	void Unequip() override;
	/// @brief 外したあとも、飛んでいる弾と爆発を進める
	void UpdateLeftover() override;
	/// @brief 飛んでいる弾と爆発を描画する
	void DrawLeftover() override;
	/// @brief 飛んでいる弾と爆発を全部消す
	void ClearLeftover() override;
	/// @brief 構え・撃つ・次に撃てるまでの間なら true を返す
	bool IsActive() const override;
	/// @brief 敵をつかまないので、常に false を返す
	bool IsHolding() const override;
	/// @brief 単押しで撃つ間は少し遅く、長押しの構えから撃ち終わるまでは止まる
	float GetMoveScale() const override;
	/// @brief 長押しの構えから撃ち終わるまでは、ジャンプをできなくするので true を返す
	bool BlocksJump() const override;
	/// @brief 長押しの構えから撃ち終わるまでは、ダッシュをできなくするので true を返す
	bool BlocksDash() const override;
	/// @brief 構えと撃った反動の上半身の攻撃クリップ（ガンランスの射撃を使い回す）を取得する
	ARM_BODY_CLIP GetBodyClip() const override;

private:
	int PickTarget() const;
	void FireVolley(bool all);
	void StopAction();
	void StepEffects(float dt);
	void AddBlast(const VEC3& position, float radius);
	VEC3 GetFireDirection() const;
	/// @brief 爆発の見た目（描画だけ）
	struct BLAST
	{
		VEC3 Position; ///< 爆心
		float Radius; ///< 爆発の半径
		float Seconds; ///< 残りの秒数（0 以下は消えている）
	};
	static constexpr int BLAST_MAX = 8;
	ArmPhaseCalc m_Phase;
	ProjectileCalc m_Shots; // 撃った指と腕
	bool m_ShotIsArm[PROJECTILE_MAX] = {}; // 弾が腕なら true（指なら false）
	BLAST m_Blasts[BLAST_MAX] = {};
	int m_FingersLeft = MISSILE_FINGER_COUNT; // 残っている指（小指から撃つ）
	bool m_ArmFired = false; // 腕を撃ったら true（撃つ動きが終わったら指と腕を戻す）
	bool m_Hold = false; // 今の動作が長押しなら true
	float m_Straighten = 0.0f; // 描画だけのまっすぐ度（0：待機、1：撃つ向き）
};
