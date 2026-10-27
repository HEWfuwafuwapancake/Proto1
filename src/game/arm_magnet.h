#pragma once

#include "game/arm.h"
#include "physics/magnetcalc.h"

/// @file arm_magnet.h
/// @brief 片手の磁石の入力・敵の保持・反発と描画

/// @brief 伸ばさない磁石の腕と、敵の引き寄せ・反発を管理する
class MagnetArm : public Arm
{
public:
	/// @brief 片手のモデルを読み、待機状態にする
	/// @param hand 左 0・右 1
	/// @param links 腕が使うゲームの部品への参照
	MagnetArm(int hand, const ARM_LINKS& links);
	/// @brief 保持している敵を吹っ飛ばさずに離して終了する
	~MagnetArm() override;
	/// @brief 入力・敵の引き寄せ・保持・反発と描画の構えを更新する
	/// @param trigger 入力を設定する（true：押した瞬間、false：それ以外）
	void Update(bool trigger) override;
	/// @brief 肩から固定長の磁石のモデル1個を描画する
	void Draw() override;
	/// @brief 保持している敵を吹っ飛ばさずに離し、待機と描画の構えを戻す
	void Reset() override;
	/// @brief 待機以外なら true を返す
	bool IsActive() const override;
	/// @brief 引き寄せ中・止めている間だけ true を返す（カメラを自由に回せる）
	bool IsHolding() const override;

private:
	MagnetCalc m_Arm;
	float m_Straighten = 0.0f; // 描画だけのまっすぐ度（0：待機、1：敵への水平の向き）。
	int m_Model = -1;
};
