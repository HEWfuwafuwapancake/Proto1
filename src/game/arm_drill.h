#pragma once

#include "game/arm.h"
#include "physics/drillcalc.h"

/// @file arm_drill.h
/// @brief 片手のドリルの攻撃と描画

/// @brief 片手のドリルの状態・モデル・回転と、ダメージ・押し出しを管理する
class DrillArm : public Arm
{
public:
	/// @brief 片手のモデルを読み、待機状態にする
	/// @param hand 左 0・右 1
	/// @param links 腕が使うゲームの部品への参照
	DrillArm(int hand, const ARM_LINKS& links);
	/// @brief 待機状態に戻して終了する
	~DrillArm() override;
	/// @brief 入力・くい込み・ダメージ・押し出しと描画の構えを更新する
	/// @param trigger 入力を設定する（true：押した瞬間、false：それ以外）
	void Update(bool trigger) override;
	/// @brief 付け根と回転するビットを描画する
	void Draw() override;
	/// @brief 待機状態に戻し、描画の構えと回転を戻す
	void Reset() override;
	/// @brief 待機以外なら true を返す
	bool IsActive() const override;
	/// @brief 敵はつかまないので常に false を返す
	bool IsHolding() const override;

private:
	bool IsThrusting() const;

	DrillCalc m_Arm;
	float m_Straighten = 0.0f; // 描画だけのまっすぐ度（0：待機、1：水平の突き）。
	float m_Spin = 0.0f;
	int m_BaseModel = -1;
	int m_BitModel = -1;
};
