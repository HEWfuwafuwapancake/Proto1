#pragma once

#include "physics/vec3.h"

/// @file dashcalc.h
/// @brief ダッシュの計算（残り時間・休み・空中で使ったか）
/// @details DirectX と Windows を使わないので、単体テスト（tests/Proto1Tests）からそのまま使える

/// @brief ダッシュの調整用の値
struct DASH_PARAMS
{
	float Speed;	///< ダッシュの速さ（1秒に進む距離）
	float Seconds;	///< ダッシュで進み続ける秒数
	float Cooldown;	///< ダッシュが終わってから、次を出せるまでの秒数
};

/// @brief 1体のダッシュの状態（進んでいる残り時間・休み・空中で使ったか）
class DashCalc
{
public:
	/// @brief ダッシュしていない・休みなしの状態に戻す
	void Reset();
	/// @brief ダッシュを始める（休みの間と、空中で着地までに2回目は始められない）
	/// @param direction 進む水平の向き（長さはいくつでもよい。長さ 0 なら始めない）
	/// @param onGround 足場の上にいるかを設定する（true：足場の上、false：空中。空中では着地まで1回だけ）
	/// @param params 調整用の値
	/// @param sustain 進み方を設定する（true：決めた秒数だけ進み続ける、false：休みと空中の1回だけを数え、速度は呼び出し側が1回足す）
	/// @return 始められたら true
	bool TryStart(const VEC3& direction, bool onGround, const DASH_PARAMS& params, bool sustain);
	/// @brief 1フレーム進め、このフレームのダッシュの速度を返す
	/// @param dt 1フレームの秒数
	/// @param onGround 足場の上にいるかを設定する（true：空中で使った記録を戻す、false：そのまま）
	/// @param params 調整用の値
	/// @return ダッシュの水平の速度（進んでいなければゼロ）
	VEC3 Step(float dt, bool onGround, const DASH_PARAMS& params);
	/// @brief 進んでいる途中のダッシュをやめる（休みは残る）
	void Cancel() { m_Time = 0.0f; }
	/// @brief ダッシュで進んでいる途中か取得する
	bool IsDashing() const { return m_Time > 0.0f; }
	/// @brief 進む水平の単位方向を取得する（最後に始めたダッシュの向き）
	const VEC3& GetDirection() const { return m_Direction; }

private:
	VEC3 m_Direction = { 0.0f, 0.0f, 1.0f }; ///< 進む水平の単位方向
	float m_Time = 0.0f; ///< 進み続ける残りの秒数
	float m_Cooldown = 0.0f; ///< 次を出せるまでの残りの秒数
	bool m_AirUsed = false; ///< 着地してから空中で使ったか
};
