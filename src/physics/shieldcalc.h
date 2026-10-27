#pragma once

#include "physics/collision.h"

/// @file shieldcalc.h
/// @brief デカ盾の構え・押し出し・休みと、押し出す範囲・速度（DirectX と Windows は使わない）

/// @brief デカ盾の状態
enum SHIELD_STATE
{
	SHIELD_IDLE, ///< 待機
	SHIELD_GUARDING, ///< 前に構えている
	SHIELD_BASHING, ///< 盾を前へ突き出している
	SHIELD_COOLDOWN ///< 次の入力まで休む
};

/// @brief デカ盾の調整値（秒数・距離・半径は正の値を指定する）
struct SHIELD_PARAMS
{
	float GuardSeconds; ///< 構えを続けられる秒数
	float BashSeconds; ///< 押し出しの姿勢を見せる秒数
	float CooldownSeconds; ///< 次の入力を受け付けるまでの休みの秒数
	float BashRange; ///< 肩から押し出しのカプセルの終点までの長さ
	float BashRadius; ///< 押し出しのカプセル半径
};

/// @brief 肩から水平方向へ伸びる押し出しのカプセルを求める
/// @param shoulder 肩の位置
/// @param forward 正面の向き（上下を除いて正規化。水平成分がゼロなら +Z）
/// @param params 押し出しの長さと半径
CAPSULE ShieldCalcBashCapsule(const VEC3& shoulder, const VEC3& forward, const SHIELD_PARAMS& params);

/// @brief 水平の正面に上向きを足し、指定した速さの押し出し速度を求める
/// @param forward 正面の向き（上下を除いて正規化。水平成分がゼロなら +Z）
/// @param speed 速度の長さ（ダメージ%に依存しない固定の速さ）
/// @param up 水平の単位方向へ足す上向き成分
VEC3 ShieldCalcPushVelocity(const VEC3& forward, float speed, float up);

/// @brief 片手のデカ盾の状態と、その状態になってからの時間を管理する
class ShieldCalc
{
public:
	/// @brief 待機状態で初期化する
	ShieldCalc() = default;
	/// @brief 時間をゼロにして待機へ戻す
	void Reset();
	/// @brief 構え・押し出し・時間切れ・取りやめ・休みを1フレーム進める
	/// @details 状態を切り替えたフレームの終わりから、その状態の時間を数える。
	/// @param trigger 入力を設定する（true：押した瞬間、false：入力なし）
	/// @param cancel 取りやめを設定する（true：復活で押し出さず休む、false：通常更新）
	/// @param dt 進める秒数（0 以下なら停止。ただし cancel は処理する）
	/// @param params 調整値
	/// @return 押し出しを始めたフレームだけ true（命中判定は呼び出し側で行う）
	bool Step(bool trigger, bool cancel, float dt, const SHIELD_PARAMS& params);
	/// @brief 現在の状態を取得する
	SHIELD_STATE GetState() const { return m_State; }
	/// @brief 今の状態になってからの秒数を取得する
	float GetTime() const { return m_Time; }

private:
	void Cooldown();
	SHIELD_STATE m_State = SHIELD_IDLE; ///< 現在の状態
	float m_Time = 0.0f; ///< 今の状態になってからの秒数
};
