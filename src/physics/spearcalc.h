#pragma once

#include "physics/stagecalc.h"
#include "physics/tentaclecalc.h"
#include "physics/vec3.h"

/// @file spearcalc.h
/// @brief 槍の投げた穂先の飛び方・刺さった相手の判定と、突っ込むのをやめる判定（DirectX・Windows なし）

/// @brief 投げた槍の速さ・届く距離・太さ
struct SPEAR_THROW_PARAMS
{
	float Speed; ///< 1秒あたりに穂先が進む距離
	float Range; ///< 投げた所から届く距離（これを超えたら外れ）
	float Radius; ///< 穂先の当たりの太さ（半径）
};

/// @brief 1フレームに投げた槍に起きたこと
enum SPEAR_THROW_EVENT
{
	SPEAR_THROW_NONE, ///< まだ飛んでいる（または投げていない）
	SPEAR_THROW_STUCK, ///< 相手に刺さった（刺さった相手の番号は GetStuckTarget）
	SPEAR_THROW_MISSED, ///< 誰にも刺さらず届く距離を超えた
};

/// @brief 投げた槍を直線上に飛ばし、最初に触れた相手に刺す
class SpearCalc
{
public:
	/// @brief 槍を手に戻す（飛んでいない・刺さっていない状態にする）
	void Reset();
	/// @brief 槍を投げる
	/// @param origin 投げた所（穂先の始まり）
	/// @param direction 飛ぶ単位方向（上下も含む）
	void Throw(const VEC3& origin, const VEC3& direction);
	/// @brief 1フレーム進め、穂先が通った線の上で最初に触れた相手に刺す
	/// @param dt 1フレームの秒数（0 以下なら進めない）
	/// @param params 速さ・届く距離・太さ
	/// @param targets 相手の並び（Available が false の相手は見ない）
	/// @param count 相手の数
	/// @return このフレームに起きたこと
	SPEAR_THROW_EVENT Step(float dt, const SPEAR_THROW_PARAMS& params, const TENTACLE_TARGET* targets, int count);
	/// @brief 槍が飛んでいる間なら true を返す
	bool IsFlying() const { return m_Flying; }
	/// @brief 槍が相手に刺さっている間なら true を返す
	bool IsStuck() const { return m_StuckTarget >= 0; }
	/// @brief 刺さった相手の番号を取得する（刺さっていなければ -1）
	int GetStuckTarget() const { return m_StuckTarget; }
	/// @brief 穂先の位置を取得する
	VEC3 GetTip() const { return m_Tip; }
	/// @brief 飛ぶ単位方向を取得する
	VEC3 GetDirection() const { return m_Direction; }

private:
	bool m_Flying = false;
	int m_StuckTarget = -1;
	float m_Traveled = 0.0f; // 投げた所から進んだ距離
	VEC3 m_Tip = {};
	VEC3 m_Direction = { 0.0f, 0.0f, 1.0f };
};

/// @brief 吹っ飛んだ相手が浮島から落ちたか調べる（落ちた相手には突っ込まない）
/// @param stage ステージ
/// @param feet 相手の足元の位置
/// @return 浮島の範囲の外にいるか、浮島の上の面より下にいれば true
bool SpearCalcHasFallen(const STAGE& stage, const VEC3& feet);
