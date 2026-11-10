#pragma once

#include "game/arm.h"

/// @brief 攻撃も武器の描画も行わない素手
class BareArm : public Arm
{
public:
	/// @brief 受け持つ手を設定する
	BareArm(int hand, const ARM_LINKS& links) : Arm(hand, links) {}
	/// @brief 入力を受け取っても何もしない
	void Update(const ARM_INPUT&) override {}
	/// @brief 武器は描かない
	void Draw() override {}
	/// @brief 戻す状態はない
	void Reset() override {}
	/// @brief 動作を持たないので false を返す
	bool IsActive() const override { return false; }
	/// @brief 相手をつかまないので false を返す
	bool IsHolding() const override { return false; }
};
