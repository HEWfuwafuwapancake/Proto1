#pragma once

#include "physics/armpresscalc.h"
#include "physics/tentaclecalc.h"
#include "physics/vec3.h"

/// @file lasercalc.h
/// @brief レーザーの溜め・発射の判定と、光線の当たりの計算（DirectX・Windows なし）

/// @brief 1フレームに撃ったもの
enum LASER_FIRE
{
	LASER_FIRE_NONE, ///< 撃っていない
	LASER_FIRE_SHOT, ///< 単発のレーザー銃（単押し、または溜めきる前に離した）
	LASER_FIRE_CANNON, ///< 全員を貫通するレーザー砲（溜めきってから離した）
};

/// @brief 溜めと発射の時間
struct LASER_PARAMS
{
	float ChargeSeconds; ///< 押し始めから砲を撃てるようになるまでの秒数
	float HoldStartSeconds; ///< 長押しと判定されるまでの秒数（溜めはこの秒数から数え始める）
	float ShotRecoverSeconds; ///< 単発を撃ってから次を撃てるまでの秒数
	float CannonRecoverSeconds; ///< 砲を撃ってから次を撃てるまでの秒数
};

/// @brief 片手のレーザーの溜めと発射を進める
class LaserCalc
{
public:
	/// @brief 溜めと撃ったあとの休みを消し、待機に戻す
	void Reset();
	/// @brief 1フレーム進める（単押しで単発。長押しで溜め、離したときに溜めきっていれば砲、まだなら単発）
	/// @param input このフレームの腕の入力
	/// @param dt 1フレームの秒数
	/// @param params 溜めと発射の時間
	/// @return このフレームに撃ったもの
	LASER_FIRE Step(const ARM_INPUT& input, float dt, const LASER_PARAMS& params);
	/// @brief 溜めている間なら true を返す
	bool IsCharging() const { return m_Charging; }
	/// @brief 撃ったあとの休みの間なら true を返す
	bool IsRecovering() const { return m_Recover > 0.0f; }
	/// @brief 溜めの割合を取得する（0：溜め始め、1：砲を撃てる。溜めていなければ 0）
	/// @param params 溜めと発射の時間
	float GetCharge(const LASER_PARAMS& params) const;
	/// @brief 最後に撃ったものを取得する（休みの描画に使う）
	LASER_FIRE GetLastFire() const { return m_LastFire; }
	/// @brief 撃ったあとの休みの残り秒数を取得する
	float GetRecover() const { return m_Recover; }

private:
	LASER_FIRE Fire(LASER_FIRE fire, const LASER_PARAMS& params);
	bool m_Charging = false;
	float m_Seconds = 0.0f; // 押し始めからの秒数（溜めている間だけ）
	float m_Recover = 0.0f;
	LASER_FIRE m_LastFire = LASER_FIRE_NONE;
};

/// @brief 視線（カメラの真ん中）の先で、最初に当たる相手か、いなければ届く距離の点を取得する（フリーカメラの狙い）
/// @param eye 視線の始まり（カメラの位置）
/// @param direction 視線の単位方向
/// @param length 視線の長さ
/// @param radius 視線の太さ（半径）
/// @param targets 相手の並び（Available が false の相手は見ない）
/// @param count 相手の数
/// @return 狙う点（ワールド座標）
VEC3 LaserCalcAimPoint(const VEC3& eye, const VEC3& direction, float length, float radius, const TENTACLE_TARGET* targets, int count);

/// @brief 太い光線に入っている相手を全部調べる（貫通する砲）
/// @param origin 光線の始まり
/// @param direction 光線の単位方向
/// @param length 光線の長さ
/// @param radius 光線の太さ（半径）
/// @param targets 相手の並び（Available が false の相手は見ない）
/// @param count 相手の数
/// @param hits 当たったかを書く先（count 個。当たった相手だけ true）
/// @return 当たった相手の数
int LaserCalcPierce(const VEC3& origin, const VEC3& direction, float length, float radius,
	const TENTACLE_TARGET* targets, int count, bool* hits);
