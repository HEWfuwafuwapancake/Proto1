#pragma once

#include "physics/tentaclecalc.h"

/// @file magnetcalc.h
/// @brief 磁石の対象選択・引き寄せ・保持・反発（DirectX と Windows は使わない）

/// @brief 磁石の状態
enum MAGNET_STATE
{
	MAGNET_IDLE, ///< 待機
	MAGNET_PULLING, ///< 敵を自分の前へ引き寄せている
	MAGNET_HOLDING, ///< 自分の前で敵を止めている
	MAGNET_COOLDOWN ///< 次の入力まで休む
};

/// @brief 磁石の調整値（距離・速さ・秒数は正の値を指定する）
struct MAGNET_PARAMS
{
	float Range; ///< 肩から磁力の線の終点までの長さ
	float BeamRadius; ///< 磁力の線のカプセル半径
	float PullSpeed; ///< 敵の中心を引き寄せる速さ（1秒あたり）
	float ArrivalDistance; ///< 引き寄せ先に着いたとみなす残り距離
	float HoldSeconds; ///< 着いてから吹っ飛ばさずに離すまでの秒数
	float CooldownSeconds; ///< 次の入力を受け付けるまでの休みの秒数
};

/// @brief 1フレームのつかみ・解放（状態が休みになったあとも対象を取得できる）
struct MAGNET_EVENT
{
	int Grabbed; ///< 新たにつかんだ敵の番号（なしは -1）
	int Released; ///< 離した敵の番号（なしは -1）
	bool Repel; ///< true：反発して吹っ飛ばす、false：ダメージなしで離す
};

/// @brief 肩から水平方向へ伸びる、見えない磁力の線のカプセルを求める
/// @param shoulder 肩の位置
/// @param aim 狙いの向き（上下を除いて正規化。水平成分がゼロなら +Z）
/// @param params 線の長さと半径
CAPSULE MagnetCalcBeam(const VEC3& shoulder, const VEC3& aim, const MAGNET_PARAMS& params);

/// @brief 片手の磁石の計算状態と、引き寄せている敵の中心を管理する
class MagnetCalc
{
public:
	/// @brief 対象を持たない待機状態で初期化する
	MagnetCalc() = default;

	/// @brief 待機に戻す（保持中の敵の解放は呼び出し側で行う）
	void Reset();

	/// @brief 対象の選択・引き寄せ・保持・反発・取りやめ・休みを1フレーム進める
	/// @details つかんだフレームの終わりから引き寄せ、到着フレームの終わりから保持時間を数える。
	///          保持対象自身の Available は false でも有効。消失は cancel で通知する。
	/// @param shoulder 移動後の肩の位置（待機中の対象選択で使う）
	/// @param aim 待機中に押した瞬間だけ使う水平の狙い方向
	/// @param holdPosition 移動後の引き寄せ先（敵の体の中心を置く位置）
	/// @param trigger 入力を設定する（true：押した瞬間、false：入力なし）
	/// @param cancel 取りやめを設定する（true：敵の消失・復活で吹っ飛ばさずに離す、false：通常更新）
	/// @param dt 進める秒数（0 以下なら停止。ただし cancel は処理する）
	/// @param params 調整値
	/// @param targets 敵の配列（選択時は Available が true の敵だけ有効。nullptr は候補なし）
	/// @param count 敵の数（番号は配列の添字）
	/// @return 新たにつかんだ敵・離した敵・反発するか
	MAGNET_EVENT Step(const VEC3& shoulder, const VEC3& aim, const VEC3& holdPosition,
		bool trigger, bool cancel, float dt, const MAGNET_PARAMS& params,
		const TENTACLE_TARGET* targets, int count);

	/// @brief 現在の状態を取得する
	MAGNET_STATE GetState() const { return m_State; }
	/// @brief 今の状態になってからの秒数を取得する
	float GetTime() const { return m_Time; }
	/// @brief 引き寄せ・保持中の敵の番号（なしは -1）を取得する
	int GetTarget() const { return m_Target; }
	/// @brief 引き寄せ・保持中の敵の中心（離したフレームも最後の位置）を取得する
	const VEC3& GetCenter() const { return m_Center; }

private:
	void Cooldown();
	MAGNET_STATE m_State = MAGNET_IDLE; ///< 現在の状態
	float m_Time = 0.0f; ///< 今の状態になってからの秒数
	int m_Target = -1; ///< 引き寄せた敵の番号（なしは -1）
	VEC3 m_Center{}; ///< 引き寄せた敵の体の中心
};
