#pragma once

#include "physics/projectilecalc.h"

/// @file rocketpunchcalc.h
/// @brief ロケットパンチの拳（飛ぶ → 戻る → 待機）の流れ（DirectX と Windows は使わない）

/// @brief ロケットパンチの拳の状態
enum ROCKET_STATE
{
	ROCKET_IDLE, ///< 腕に付いている
	ROCKET_FLYING, ///< 飛んでいる
	ROCKET_RETURNING ///< 肩へ戻っている（当たらない）
};

/// @brief ロケットパンチの調整値（正の値を指定する）
struct ROCKET_PARAMS
{
	float Speed; ///< 飛ぶ速さ
	float Range; ///< 飛ぶ距離（これだけ進むと戻る）
	float Radius; ///< 拳の当たりの半径
	float ReturnSpeed; ///< 戻る速さ
};

/// @brief 片手のロケットパンチの拳を管理する
class RocketPunchCalc
{
public:
	/// @brief 待機で初期化する
	RocketPunchCalc() = default;
	/// @brief 拳を腕に戻して待機にする
	void Reset();
	/// @brief 待機なら拳を飛ばす
	/// @param origin 出す位置（肩）
	/// @param direction 飛ばす単位方向
	/// @param params 調整値
	/// @return 飛ばせたら true
	bool Launch(const VEC3& origin, const VEC3& direction, const ROCKET_PARAMS& params);
	/// @brief 1フレーム進める。飛んでいる拳は当たる・浮島・距離で戻り始め、戻る拳は肩に着くと待機になる
	/// @param recall 呼び戻しを設定する（true：飛んでいれば戻し始める、false：通常更新）
	/// @param dt 進める秒数（0 以下なら止まる）
	/// @param shoulder 今の肩の位置（戻る先）
	/// @param targets 相手の並び
	/// @param count 相手の数
	/// @param stage 浮島と場外の線
	/// @param params 調整値
	/// @return 当たった相手の番号（当たらなければ -1）
	int Step(bool recall, float dt, const VEC3& shoulder, const TENTACLE_TARGET* targets, int count,
		const STAGE& stage, const ROCKET_PARAMS& params);
	/// @brief 今の状態を取得する
	ROCKET_STATE GetState() const { return m_State; }
	/// @brief 拳の位置を取得する（待機のときは最後の位置）
	VEC3 GetFist() const;
	/// @brief 飛ばした単位方向を取得する
	VEC3 GetDirection() const { return m_Direction; }

private:
	PROJECTILE_PARAMS ShotParams(const ROCKET_PARAMS& params) const;
	ProjectileCalc m_Shot; ///< 飛んでいる拳（1発だけ使う）
	ROCKET_STATE m_State = ROCKET_IDLE; ///< 今の状態
	VEC3 m_Position = {}; ///< 戻っている拳の位置
	VEC3 m_Direction = { 0.0f, 0.0f, 1.0f }; ///< 飛ばした単位方向
	int m_Index = -1; ///< 飛んでいる拳の弾の番号
};
