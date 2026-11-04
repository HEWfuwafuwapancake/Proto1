#pragma once

#include "game/arm.h"
#include "physics/armphasecalc.h"

/// @file arm_smoke.h
/// @brief 片手のスモークの入力・煙と描画

/// @brief 足元に煙を出し、煙の中の人をロックオンされないようにする腕
class SmokeArm : public Arm
{
public:
	/// @brief 待機状態にする（モデルはないので読み込まない）
	/// @param hand 左 0・右 1
	/// @param links 腕が使うゲームの部品への参照
	SmokeArm(int hand, const ARM_LINKS& links);
	/// @brief 押したら足元に煙を出し、煙がある間は中の人に「隠れる」をかけ直す
	/// @param trigger 入力を設定する（true：押した瞬間、false：それ以外）
	void Update(bool trigger) override;
	/// @brief 仮の形（腕・筒の円柱と、煙の灰色の球）を描画する
	void Draw() override;
	/// @brief 待機へ戻し、煙を消す
	void Reset() override;
	/// @brief 煙はキャラの向きを縛らないので、常に false を返す
	bool IsActive() const override;
	/// @brief 敵をつかまないので、常に false を返す
	bool IsHolding() const override;

private:
	ArmPhaseCalc m_Phase;
	VEC3 m_Center = {}; // 煙の中心。
};
