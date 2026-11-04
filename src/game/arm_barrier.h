#pragma once

#include "game/arm.h"
#include "physics/armphasecalc.h"

/// @file arm_barrier.h
/// @brief 片手のバリアの入力・ガード・弾けと描画

/// @brief 体のまわりにバリアを張って全方向をガードし、もう一度押すと弾けてまわりを押す腕
class BarrierArm : public Arm
{
public:
	/// @brief 待機状態にする（モデルはないので読み込まない）
	/// @param hand 左 0・右 1
	/// @param links 腕が使うゲームの部品への参照
	BarrierArm(int hand, const ARM_LINKS& links);
	/// @brief バリア・休みを進め、バリアの間の再入力で弾けてまわりの相手を押す
	/// @param trigger 入力を設定する（true：押した瞬間、false：それ以外）
	void Update(bool trigger) override;
	/// @brief 仮の形（腕の円柱と、体のまわりに並べたバリアの球）を描画する
	void Draw() override;
	/// @brief 待機へ戻す
	void Reset() override;
	/// @brief バリアはキャラの向きを縛らないので、常に false を返す
	bool IsActive() const override;
	/// @brief 敵をつかまないので、常に false を返す
	bool IsHolding() const override;
	/// @brief バリアの間だけ歩く速さを下げる
	float GetMoveScale() const override;
	/// @brief バリアの間は全方向をガードする
	GUARD GetGuard() const override;

private:
	void DrawRing(float radius, const XMFLOAT4& color) const;
	ArmPhaseCalc m_Phase;
	float m_BurstTime = 0.0f; // 弾けた範囲を見せる残りの秒数。
};
