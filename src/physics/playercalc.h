#pragma once

#include "physics/stagecalc.h"
#include "physics/vec3.h"

/// @file playercalc.h
/// @brief プレイヤーの動きの計算（移動・ジャンプ・重力・足場への着地・左右の向き）
/// @details DirectX と Windows を使わないので、単体テスト（tests/Proto1Tests）からそのまま使える

/// @brief プレイヤーの動きの調整用の値
struct PLAYER_PARAMS
{
	float MoveSpeed;	///< 歩く速さ（1秒に進む距離）
	float JumpSpeed;	///< ジャンプした瞬間の上向きの速さ（1秒に進む距離）
	float Gravity;		///< 重力（1秒ごとに下向きの速さがこれだけ増える）
	float MaxFallSpeed = 0.0f;	///< 重力で落ちる速さの上限（1秒に進む距離。0 以下なら上限なし）
	int AirJumps = 0;	///< 空中でジャンプできる回数（着地で戻る）
};

/// @brief 重力で上下の速さを1フレーム分変える（落ちる速さの上限つき）
/// @details 上限より速く落ちているとき（下へ強く吹っ飛ばされたときなど）は、それ以上速くしないだけで遅くはしない
/// @param velocityY 今の上下の速さ（上がプラス）
/// @param gravity 重力（1秒ごとに下向きの速さがこれだけ増える）
/// @param maxFallSpeed 重力で落ちる速さの上限（0 以下なら上限なし）
/// @param dt 1フレームの秒数
/// @return 更新後の上下の速さ
float PlayerCalcFall(float velocityY, float gravity, float maxFallSpeed, float dt);

/// @brief ターゲット、なければ歩く方向へ速さの上限つきで向く（腕の操作中は画面の正面へ即座に向く）
/// @param current 今の左右の向き（ラジアン。0で+Z方向、プラスで右回り）
/// @param moveX 移動の向きの X（ワールド座標。速度でもよい）
/// @param moveZ 移動の向きの Z（ワールド座標。速度でもよい）
/// @param cameraYaw 画面の正面の左右の向き（ラジアン）
/// @param armActive 腕の向きを優先するか（true：ボタンを押した瞬間または腕が出ている、false：ターゲットまたは歩く方向へ向く）
/// @param speed 1秒に回る角度の上限（ラジアン。0以下なら通常の回転は止まる）
/// @param dt 1フレームの秒数（0以下なら通常の回転は止まる。腕の向きは即座に反映する）
/// @param hasTarget ターゲットがいるか（true：targetYawへ向く、false：歩く方向へ向く）
/// @param targetYaw ターゲットへの左右の向き（ラジアン）
/// @return 更新後の左右の向き（ターゲットも移動入力もなければcurrentのまま）
float PlayerCalcFacing(float current, float moveX, float moveZ, float cameraYaw,
	bool armActive, float speed, float dt, bool hasTarget = false, float targetYaw = 0.0f);

/// @brief フリーカメラのときの攻撃の向きを求める（左スティックを倒していればその向き、倒していなければ体の向き）
/// @param current 今の体の左右の向き（ラジアン。0で+Z方向、プラスで右回り）
/// @param stickX 左スティックの左右（画面から見た向き。右がプラス）
/// @param stickY 左スティックの前後（画面から見た向き。前がプラス）
/// @param cameraYaw 画面の正面の左右の向き（ラジアン）
/// @return 攻撃の左右の向き（ラジアン。-π〜π）
float PlayerCalcFreeAimYaw(float current, float stickX, float stickY, float cameraYaw);

/// @brief フリーカメラのときの体の向き（腕を使い始めた瞬間は攻撃の向きへ即座に向き、使っている間は回さない）
/// @param current 今の左右の向き（ラジアン。0で+Z方向、プラスで右回り）
/// @param moveX 移動の向きの X（ワールド座標。速度でもよい）
/// @param moveZ 移動の向きの Z（ワールド座標。速度でもよい）
/// @param aimYaw 攻撃の向き（ラジアン。PlayerCalcFreeAimYaw の結果）
/// @param armActive 腕を使っているかを設定する（true：使っている間は向きを変えない、false：歩く方向へ向く）
/// @param armStarted 腕を使い始めたフレームかを設定する（true：aimYaw へ即座に向く、false：armActive に従う）
/// @param speed 1秒に回る角度の上限（ラジアン。0以下なら歩く方向へは回らない）
/// @param dt 1フレームの秒数（0以下なら歩く方向へは回らない）
/// @return 更新後の左右の向き
float PlayerCalcFreeLookFacing(float current, float moveX, float moveZ, float aimYaw,
	bool armActive, bool armStarted, float speed, float dt);

/// @brief プレイヤーの体（位置・速度・立っているか）
class PlayerBodyCalc
{
public:
	/// @brief プレイヤーの体を1フレーム分動かす
	/// @details 次の順に計算する：
	///          1. 横の速度は、突進速度または移動の向き × 歩く速さ（向きの長さは 1 までに縮める）
	///          2. 足元に足場があるときにジャンプを押したら、上向きの速さを与える。空中では params.AirJumps 回まで出せる
	///          3. 立っていなければ重力で下向きの速さを増やし、位置を動かす
	///          4. 足場から出たら立っていない状態にする。上から足場の上面を通り過ぎたら、その上面に着地する
	/// @param moveX 移動の向きの X（ワールドの向き。-1〜1）
	/// @param moveZ 移動の向きの Z（ワールドの向き。-1〜1）
	/// @param jump ジャンプ入力を設定する（true：押した瞬間、false：押していない）
	/// @param dt 1フレームの秒数（ふつうは GameTimeGetDeltaTime() の値）
	/// @param params 調整用の値
	/// @param stage 浮島と高台
	/// @param rushVelocity 突進のワールド速度（X・Z をそのまま使う。両方 0 なら通常の移動）
	void Step(float moveX, float moveZ, bool jump, float dt,
		const PLAYER_PARAMS& params, const STAGE& stage, const VEC3& rushVelocity = {});

	/// @brief 足元の Xを取得する
	float GetX() const { return m_X; }

	/// @brief 足元の高さを取得する
	float GetY() const { return m_Y; }

	/// @brief 足元の Zを取得する
	float GetZ() const { return m_Z; }

	/// @brief 速度の X（1秒に進む距離）を取得する
	float GetVelocityX() const { return m_VelocityX; }

	/// @brief 速度の Y（上がプラス）を取得する
	float GetVelocityY() const { return m_VelocityY; }

	/// @brief 速度の Zを取得する
	float GetVelocityZ() const { return m_VelocityZ; }

	/// @brief 足場の上に立っているか（立っているときだけジャンプできる）を取得する
	bool GetOnGround() const { return m_OnGround; }

	/// @brief 足元の位置を設定する
	/// @param position 足元の座標
	void SetPosition(const VEC3& position) { m_X = position.X; m_Y = position.Y; m_Z = position.Z; }

	/// @brief 速度を設定する
	/// @param velocity 1秒あたりの移動量
	void SetVelocity(const VEC3& velocity) { m_VelocityX = velocity.X; m_VelocityY = velocity.Y; m_VelocityZ = velocity.Z; }

	/// @brief 着地状態を設定する（true：足場の上。空中ジャンプの回数も戻す、false：空中）
	void SetOnGround(bool onGround) { m_OnGround = onGround; if (onGround) m_AirJumpsUsed = 0; }

	/// @brief ジャンプする（足場の上ならふつうのジャンプ、空中なら残りがあれば空中ジャンプ。横の速さは変えない）
	/// @param jumpSpeed ジャンプした瞬間の上向きの速さ
	/// @param airJumps 空中でジャンプできる回数
	/// @return ジャンプできたら true
	bool Jump(float jumpSpeed, int airJumps);

	/// @brief 空中ジャンプの回数を戻す（掴まれたときなど）
	void ResetAirJumps() { m_AirJumpsUsed = 0; }

	/// @brief 空中ジャンプを使った回数を取得する
	int GetAirJumpsUsed() const { return m_AirJumpsUsed; }

private:
	float m_X{}; ///< 足元の X
	float m_Y{}; ///< 足元の高さ
	float m_Z{}; ///< 足元の Z
	float m_VelocityX{}; ///< 速度の X（1秒に進む距離）
	float m_VelocityY{}; ///< 速度の Y（上がプラス）
	float m_VelocityZ{}; ///< 速度の Z
	bool m_OnGround{}; ///< 足場の上に立っているか
	int m_AirJumpsUsed{}; ///< 着地してから空中ジャンプを使った回数
};
