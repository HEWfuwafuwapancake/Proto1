#pragma once

#include "physics/collision.h"
#include "physics/stagecalc.h"
#include "physics/tentaclecalc.h"

/// @file projectilecalc.h
/// @brief 飛び道具の弾（まっすぐ・山なり・追いかける・貫通）の入れ物と動き（DirectX と Windows は使わない）

/// @brief 1本の腕が同時に出せる弾の数
static constexpr int PROJECTILE_MAX = 16;

/// @brief 弾の調整値（1本の腕のすべての弾で同じ）
struct PROJECTILE_PARAMS
{
	float Radius; ///< 弾の半径
	float Gravity; ///< 下向きの加速度（0 ならまっすぐ飛ぶ）
	float LifeSeconds; ///< 弾が消えるまでの秒数
	float TurnRate; ///< 追いかけるときに1秒に曲がれる角度（ラジアン。0 なら追いかけない）
	bool Pierce; ///< 貫通するか（true：当たっても進み、同じ相手には1回だけ、false：最初の相手で消える）
};

/// @brief 1発の弾
struct PROJECTILE
{
	VEC3 Position; ///< 今の位置
	VEC3 Velocity; ///< 今の速度
	float Life; ///< 消えるまでの残りの秒数
	int Target; ///< 追いかける相手の番号（なしは -1）
	bool Active; ///< 飛んでいるか
	HitRecord Hits; ///< 貫通する弾が当てた相手
};

/// @brief 弾に起きたことの種類
enum PROJECTILE_EVENT_TYPE
{
	PROJECTILE_HIT_FIGHTER, ///< 相手に当たった
	PROJECTILE_HIT_STAGE, ///< 浮島や高台に当たって消えた
	PROJECTILE_EXPIRED ///< 時間切れか場外の線より下で消えた
};

/// @brief 1フレームに弾に起きたこと
struct PROJECTILE_EVENT
{
	PROJECTILE_EVENT_TYPE Type; ///< 起きたことの種類
	int Index; ///< 弾の番号
	int Target; ///< 当たった相手の番号（相手に当たったとき以外は -1）
	VEC3 Position; ///< 起きた位置（足場なら上の面の高さ）
	VEC3 Direction; ///< そのときの進む単位方向
};

/// @brief 1本の腕が持つ弾の入れ物
class ProjectileCalc
{
public:
	/// @brief 弾なしで初期化する
	ProjectileCalc() = default;
	/// @brief 弾をすべて消す
	void Reset();
	/// @brief 空いている所に弾を出す
	/// @param origin 出す位置
	/// @param velocity 出す速度
	/// @param target 追いかける相手の番号（なしは -1）
	/// @param params 調整値（寿命を使う）
	/// @return 弾の番号。いっぱいなら -1
	int Fire(const VEC3& origin, const VEC3& velocity, int target, const PROJECTILE_PARAMS& params);
	/// @brief すべての弾を1フレーム進め、起きたことを返す
	/// @details 相手との当たりは「前の位置 → 今の位置」のカプセルで調べる（速い弾がすり抜けない）。相手が先、足場があと
	/// @param dt 進める秒数（0 以下なら何もしない）
	/// @param targets 相手の並び（Available が false の相手には当たらず、追いかけもしない）
	/// @param count 相手の数
	/// @param stage 浮島・高台・場外の線
	/// @param params 調整値
	/// @param events 起きたことを書く並び
	/// @param maxEvents events の数
	/// @return 書いた数
	int Step(float dt, const TENTACLE_TARGET* targets, int count, const STAGE& stage, const PROJECTILE_PARAMS& params,
		PROJECTILE_EVENT* events, int maxEvents);
	/// @brief 飛んでいる弾の数を取得する
	int GetCount() const;
	/// @brief 弾を取得する（描画用。index は 0〜PROJECTILE_MAX-1）
	const PROJECTILE& Get(int index) const { return m_Shots[index]; }

private:
	PROJECTILE m_Shots[PROJECTILE_MAX] = {};
};

/// @brief 山なりで to に落ちる速度を求める
/// @param from 出す位置
/// @param to 落としたい位置
/// @param horizontalSpeed 水平の速さ
/// @param gravity 下向きの加速度（正）
/// @param maxRange 水平にこれより遠いときは、同じ向きのこの距離に落とす
/// @return 速度
VEC3 ProjectileCalcLobVelocity(const VEC3& from, const VEC3& to, float horizontalSpeed, float gravity, float maxRange);

/// @brief 一定の速さで to へ近づける（行き過ぎない）
/// @param from 今の位置
/// @param to 目指す位置
/// @param speed 速さ
/// @param dt 進める秒数
/// @return 進めた位置
VEC3 ProjectileCalcMoveToward(const VEC3& from, const VEC3& to, float speed, float dt);
