#pragma once

#include "game/arm.h"
#include "physics/grapplecalc.h"

/// @file arm_grapple.h
/// @brief 片手のグラップリングと、左右の引き寄せの所有権

/// @brief 片手のグラップリングの状態・モデル・描画を管理する
class GrappleArm : public Arm
{
public:
	/// @brief 片手のモデルを読み、待機状態にする
	/// @param hand 左 0・右 1
	/// @param links 腕が使うゲームの部品への参照
	GrappleArm(int hand, const ARM_LINKS& links);
	/// @brief 自分が引き寄せ中なら解除して終了する
	~GrappleArm() override;
	/// @brief 左右の腕を接続し、引き寄せの所有権を渡せるようにする（起動時に1回）
	/// @param other 反対の手のグラップリング
	void ConnectOtherArm(GrappleArm& other);
	/// @brief 入力・命中・プレイヤーの引き寄せと描画の構えを更新する
	/// @param trigger 入力を設定する（true：押した瞬間、false：それ以外）
	void Update(bool trigger) override;
	/// @brief 上腕・前腕・フックとワイヤーを描画する
	void Draw() override;
	/// @brief 待機に戻し、自分が引き寄せ中なら解除する
	void Reset() override;
	/// @brief 待機以外なら true を返す
	bool IsActive() const override;
	/// @brief 敵はつかまないので常に false を返す
	bool IsHolding() const override;

protected:
	/// @brief 肩の箱の中央が体の取付口に来るよう、取付位置を少し上げる
	VEC3 GetMountOffset() const override;

private:
	bool MirrorHand(int hand) const;
	GrappleCalc m_Arm;
	float m_Straighten = 0.0f; // 描画だけのまっすぐ度（0：待機の構え、1：フックの向き）。
	int m_UpperModel = -1;
	int m_LowerModel = -1;
	int m_HookModel = -1;
	GrappleArm* m_OtherArm = nullptr;
	bool m_Pulling = false; // 左右のうち、後から刺さって引き寄せを受け持つ手だけ true。
};
