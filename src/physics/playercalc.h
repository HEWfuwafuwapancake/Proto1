#pragma once

#include "physics/stagecalc.h"

/// @file playercalc.h
/// @brief プレイヤーの動きの計算（移動・ジャンプ・重力・浮島への着地）
/// @details DirectX と Windows を使わないので、単体テスト（tests/Proto1Tests）からそのまま使える

/// @brief プレイヤーの体（位置・速度・立っているか）
struct PLAYER_BODY
{
	float X;			///< 足元の X
	float Y;			///< 足元の高さ
	float Z;			///< 足元の Z
	float VelocityX;	///< 速度の X（1秒に進む距離）
	float VelocityY;	///< 速度の Y（上がプラス）
	float VelocityZ;	///< 速度の Z
	bool OnGround;		///< 浮島の上に立っているか（立っているときだけジャンプできる）
};

/// @brief プレイヤーの動きの調整用の値
struct PLAYER_PARAMS
{
	float MoveSpeed;	///< 歩く速さ（1秒に進む距離）
	float JumpSpeed;	///< ジャンプした瞬間の上向きの速さ（1秒に進む距離）
	float Gravity;		///< 重力（1秒ごとに下向きの速さがこれだけ増える）
};

/// @brief プレイヤーの体を1フレーム分動かす
/// @details 次の順に計算する：
///          1. 横の速度 = 移動の向き × 歩く速さ（空中でも同じように動ける。向きの長さは 1 までに縮める）
///          2. 立っていてジャンプを押したら、上向きの速さを与える。浮島の外へ歩き出たら立っていない状態にする
///          3. 立っていなければ重力で下向きの速さを増やし、位置を動かす
///          4. 落ちてきて浮島の上の面を上から通り過ぎ、そこが浮島の範囲の中なら、上の面に止めて立っている状態にする
///             （浮島の外や、浮島より下から上がってきたときは止めない）
/// @param[in,out] body プレイヤーの体
/// @param moveX 移動の向きの X（ワールドの向き。-1〜1）
/// @param moveZ 移動の向きの Z（ワールドの向き。-1〜1）
/// @param jump ジャンプを押した瞬間か（true：押した瞬間、false：押していない）
/// @param dt 1フレームの秒数（ふつうは GameTimeGetDeltaTime() の値）
/// @param params 調整用の値
/// @param stage 浮島
void PlayerCalcStep(PLAYER_BODY& body, float moveX, float moveZ, bool jump, float dt,
	const PLAYER_PARAMS& params, const STAGE& stage);
