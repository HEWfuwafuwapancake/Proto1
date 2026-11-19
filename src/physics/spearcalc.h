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

/// @brief 槍の単押しの突きの、穂先と手元の強さ
struct SPEAR_THRUST_PARAMS
{
	float TipStart; ///< 肩からこの距離より先（体の手前の面まで）で当たれば穂先の当たり
	float TipDamage; ///< 穂先の当たりで足すダメージ（%）
	float TipPower; ///< 穂先の当たりの吹っ飛ばしの強さ
	float BaseDamage; ///< 手元の当たりで足すダメージ（%）
	float BasePower; ///< 手元の当たりの吹っ飛ばしの強さ
};

/// @brief 突きの当たりのダメージと吹っ飛ばしの強さ
struct SPEAR_THRUST_HIT
{
	float Damage; ///< 足すダメージ（%）
	float Power; ///< 吹っ飛ばしの強さ
	bool Tip; ///< 穂先の当たりなら true、手元なら false
};

/// @brief 突きの直線上の、肩から相手の体の手前の面までの距離を求める
/// @param origin 突きの始まり（肩）
/// @param direction 突きの単位方向
/// @param body 相手の体
/// @return 体の中心を直線へ映した距離から体の半径を引いた距離
float SpearCalcFrontDistance(const VEC3& origin, const VEC3& direction, const CAPSULE& body);

/// @brief 突きが当たった相手への強さを、穂先か手元かで決める
/// @param origin 突きの始まり（肩）
/// @param direction 突きの単位方向
/// @param body 当たった相手の体
/// @param params 穂先と手元の強さ
/// @return ダメージ・吹っ飛ばしの強さと、穂先の当たりか
SPEAR_THRUST_HIT SpearCalcThrustHit(const VEC3& origin, const VEC3& direction, const CAPSULE& body, const SPEAR_THRUST_PARAMS& params);

/// @brief 踏み込みを止めるか調べる（突きの直線上で、穂先の距離より近くに相手がいれば止める）
/// @param origin 突きの始まり（肩）
/// @param direction 突きの単位方向
/// @param params 穂先の始まる距離（TipStart）を使う
/// @param radius 突きの太さ（半径）
/// @param targets 相手の並び（Available が false の相手は見ない）
/// @param count 相手の数
/// @return 穂先の距離より近くに相手がいれば true
bool SpearCalcShouldStopLunge(const VEC3& origin, const VEC3& direction, const SPEAR_THRUST_PARAMS& params, float radius,
	const TENTACLE_TARGET* targets, int count);
