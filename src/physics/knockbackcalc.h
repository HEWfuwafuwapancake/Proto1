#pragma once

#include "physics/playercalc.h"
#include "physics/vec3.h"

/// @file knockbackcalc.h
/// @brief ダメージ%と吹っ飛びの計算（スマブラ式：% が高いほど速く＝遠くへ飛ぶ）
/// @details DirectX と Windows を使わないので、単体テスト（tests/Proto1Tests）からそのまま使える

/// @brief 吹っ飛びの調整用の値
struct KNOCKBACK_PARAMS
{
	float BaseSpeed;		///< 0% のときに吹っ飛ぶ速さ（攻撃の強さ 1 のとき。1秒に進む距離）
	float Growth;			///< ダメージ 1% ごとに増える、吹っ飛ぶ速さ
	float LaunchAngle;		///< 吹っ飛ぶ向きの、水平からの上向きの角度（ラジアン）
	float AirDrag;			///< 空気の抵抗（空中で、横の速さが 1秒ごとに e^(-AirDrag) 倍になる）
	float GroundFriction;	///< 地面の摩擦（浮島の上をすべっている間、横の速さが 1秒ごとにこれだけ減る）
	float StopSpeed;		///< 浮島の上で横の速さがこれより遅くなったら止まる（吹っ飛ばされている状態が終わる）
	float Gravity;			///< 重力（1秒ごとに下向きの速さがこれだけ増える）
};

/// @brief ダメージ%にダメージを足す
/// @param percent 今のダメージ%
/// @param damage 攻撃のダメージ（負の値は 0 にする）
/// @return 足したあとのダメージ%（上限はない）
float KnockbackCalcAddDamage(float percent, float damage);

/// @brief 吹っ飛ぶ速さを求める
/// @param percent ダメージ%（攻撃のダメージを足したあとの値）
/// @param power 攻撃の強さ（1 でふつう。大きいほど速い）
/// @param params 調整用の値
/// @return 吹っ飛ぶ速さ =（BaseSpeed ＋ percent × Growth）× power
float KnockbackCalcSpeed(float percent, float power, const KNOCKBACK_PARAMS& params);

/// @brief 吹っ飛ぶ速度（向きと速さ）を求める
/// @details 攻撃の向き（横だけを使う）に、LaunchAngle だけ上向きの角度をつける。攻撃の向きが横に向いていないときは真上に飛ぶ
/// @param direction 攻撃の向き（攻撃した側から、された側へ。長さはいくつでもよい）
/// @param speed 吹っ飛ぶ速さ
/// @param params 調整用の値
/// @return 速度（長さが speed）
VEC3 KnockbackCalcVelocity(const VEC3& direction, float speed, const KNOCKBACK_PARAMS& params);

/// @brief 上下を含む指定方向の速度を取得する。方向の長さは無視し、ゼロ方向なら真上へ飛ばす
VEC3 KnockbackCalcDirectedVelocity(const VEC3& direction, float speed);

/// @brief 吹っ飛ばされている体を1フレーム分動かす
/// @details 自分では止まれない。空中では重力と空気の抵抗、浮島の上から落ちてきたら着地して、地面の摩擦ですべりながら遅くなる。
///          浮島の上で StopSpeed より遅くなったら止まり、吹っ飛ばされている状態が終わる（浮島の外では止まらずに落ちる）
/// @param[in,out] body 体（速度は吹っ飛ばされたときの速度から始める）
/// @param dt 1フレームの秒数
/// @param params 調整用の値
/// @param stage 浮島
/// @return まだ吹っ飛ばされているなら true、止まったら false
bool KnockbackCalcStep(PLAYER_BODY& body, float dt, const KNOCKBACK_PARAMS& params, const STAGE& stage);
