#pragma once

#include "game/arm.h"
#include "physics/armphasecalc.h"
#include "physics/projectilecalc.h"

/// @file arm_gatling.h
/// @brief 片手のガトリングの入力・連射と描画

/// @brief 銃身を回してから撃ち続け、当たるたびに % を少し足して押す腕
class GatlingArm : public Arm
{
public:
	/// @brief 待機状態にする（モデルはないので読み込まない）
	/// @param hand 左 0・右 1
	/// @param links 腕が使うゲームの部品への参照
	GatlingArm(int hand, const ARM_LINKS& links);
	/// @brief 回す・撃つ・休みを進め、撃つ間は刻みごとに弾を出す。撃つ間の再入力で止める
	/// @param trigger 入力を設定する（true：押した瞬間、false：それ以外）
	void Update(bool trigger) override;
	/// @brief 仮の形（腕の円柱・回る3本の銃身・弾の球）を描画する
	void Draw() override;
	/// @brief 待機へ戻し、弾を消す
	void Reset() override;
	/// @brief 回す・撃つ間だけ true を返す
	bool IsActive() const override;
	/// @brief 敵をつかまないので、常に false を返す
	bool IsHolding() const override;
	/// @brief 回す・撃つ間だけ歩く速さを下げる
	float GetMoveScale() const override;

private:
	ArmPhaseCalc m_Phase;
	ProjectileCalc m_Bullets;
	int m_Shots = 0; // 今の連射で撃った数（ぶれの並びに使う）。
	VEC3 m_Direction = { 0.0f, 0.0f, 1.0f }; // 最後に撃った向き（描画用）。
	float m_Straighten = 0.0f; // 描画だけのまっすぐ度（0：待機、1：狙いの向き）。
};
