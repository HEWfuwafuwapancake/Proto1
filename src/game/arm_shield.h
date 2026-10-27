#pragma once

#include "game/arm.h"
#include "physics/shieldcalc.h"

/// @file arm_shield.h
/// @brief 片手のデカ盾の入力・押し出し・歩く速さと描画

/// @brief 伸ばさないデカ盾の腕で構え、再入力で前の敵を押し出す
class ShieldArm : public Arm
{
public:
	/// @brief 片手の付け根・板のモデルを読み、待機状態にする
	/// @param hand 左 0・右 1
	/// @param links 腕が使うゲームの部品への参照
	ShieldArm(int hand, const ARM_LINKS& links);
	/// @brief 構え・押し出し・休みと描画の構えを更新する
	/// @param trigger 入力を設定する（true：押した瞬間、false：それ以外）
	void Update(bool trigger) override;
	/// @brief 固定長の付け根と、その取付位置にある板を描画する
	void Draw() override;
	/// @brief 状態と描画の構えを待機へ戻す
	void Reset() override;
	/// @brief 待機以外なら true を返す
	bool IsActive() const override;
	/// @brief 敵をつかまないので、常に false を返す
	bool IsHolding() const override;
	/// @brief 構え・押し出し中だけ歩く速さの倍率を下げる
	float GetMoveScale() const override;

private:
	ShieldCalc m_Arm;
	float m_Straighten = 0.0f; // 描画だけのまっすぐ度（0：待機、1：キャラの正面）。
	int m_BaseModel = -1;
	int m_PlateModel = -1;
};
