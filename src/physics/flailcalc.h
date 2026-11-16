#pragma once

#include "physics/vec3.h"

/// @file flailcalc.h
/// @brief モーニングスターの腕の計算：ためた分の伸び、頭の上で回す鉄球の位置、投げた鉄球の行きと帰り（DirectX と Windows は使わない）

/// @brief 投げた鉄球の状態
enum FLAIL_STATE
{
	FLAIL_IDLE, ///< 手元にある（投げていない）
	FLAIL_OUT, ///< 行き：投げた向きへまっすぐ飛んでいる
	FLAIL_BACK, ///< 帰り：今の手元へ戻っている
};

/// @brief 投げた鉄球の動きの設定
struct FLAIL_THROW_PARAMS
{
	float OutSpeed; ///< 行きの速さ
	float BackSpeed; ///< 帰りの速さ
	float CatchDistance; ///< 手元からこの距離まで戻ったら受け止める
};

/// @brief 回した秒数から、ためた割合を求める
/// @param spinSeconds 回した秒数
/// @param fullSeconds 最大までためる秒数
/// @return 0〜1
float FlailCalcCharge(float spinSeconds, float fullSeconds);

/// @brief ためた割合で、最小と最大の間の値を求める
/// @param minValue ためなしの値
/// @param maxValue 最大までためた値
/// @param charge ためた割合（0〜1 に収める）
/// @return 間の値
float FlailCalcMix(float minValue, float maxValue, float charge);

/// @brief 頭の上で回している鉄球の位置を求める（角度 0 で中心の +Z、増えると +X の側へ回る）
/// @param center 回す輪の中心
/// @param angle 回した角度（ラジアン）
/// @param radius 輪の半径
/// @return 鉄球の位置
VEC3 FlailCalcSpinPosition(const VEC3& center, float angle, float radius);

/// @brief 投げた鉄球の行きと帰りを進める
class FlailCalc
{
public:
	/// @brief 手元にある状態で初期化する
	FlailCalc() = default;
	/// @brief 鉄球を手元に戻す（投げていない状態）
	void Reset();
	/// @brief 鉄球を投げる
	/// @param origin 投げる位置（肩）
	/// @param direction 投げる単位方向（上下も含む）
	/// @param range 行きで飛ぶ距離
	void Throw(const VEC3& origin, const VEC3& direction, float range);
	/// @brief 鉄球を進める。行きは決めた距離まで進んでから帰りに変わり、帰りは今の手元を追って、着いたら手元に戻る
	/// @param dt 進める秒数（0 以下なら何もしない）
	/// @param hand 今の手元
	/// @param params 動きの設定
	/// @return 進めたあとの状態
	FLAIL_STATE Step(float dt, const VEC3& hand, const FLAIL_THROW_PARAMS& params);
	/// @brief 今の状態を取得する
	FLAIL_STATE GetState() const { return m_State; }
	/// @brief 投げている（行きか帰り）なら true を返す
	bool IsFlying() const { return m_State != FLAIL_IDLE; }
	/// @brief 鉄球の今の位置を取得する
	const VEC3& GetPosition() const { return m_Position; }
	/// @brief 前の Step の前の、鉄球の位置を取得する（通り道の当たり判定に使う）
	const VEC3& GetPrevious() const { return m_Previous; }
	/// @brief 最後に動いた単位方向を取得する（帰りなら手元の向き）
	const VEC3& GetMoveDirection() const { return m_MoveDirection; }

private:
	FLAIL_STATE m_State = FLAIL_IDLE; // 今の状態
	VEC3 m_Position = {}; // 鉄球の位置
	VEC3 m_Previous = {}; // 前の Step の前の位置
	VEC3 m_MoveDirection = { 0.0f, 0.0f, 1.0f }; // 最後に動いた向き
	float m_Remaining = 0.0f; // 行きの残りの距離
};
