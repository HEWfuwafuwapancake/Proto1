#pragma once

#include "physics/projectilecalc.h"

/// @file c4calc.h
/// @brief C4 の投げた弾・落ちる弾・設置済みの管理（DirectX と Windows は使わない）

/// @brief 片手で置いておける C4 の数
static constexpr int C4_MAX = 3;

/// @brief 片手の C4 を、投げた・落ちている・設置済みの3つに分けて管理する
class C4Calc
{
public:
	/// @brief 弾なしで初期化する
	C4Calc() = default;
	/// @brief 投げた弾・落ちている弾・設置済みをすべて消す
	void Reset();
	/// @brief C4 を投げる
	/// @param origin 出す位置
	/// @param velocity 出す速度
	/// @param params 弾の調整値
	void Throw(const VEC3& origin, const VEC3& velocity, const PROJECTILE_PARAMS& params);
	/// @brief 弾を1フレーム進める。足場に当たったら設置し、相手に当たったらその場に落とす
	/// @param dt 進める秒数
	/// @param targets 相手の並び
	/// @param count 相手の数
	/// @param stage 浮島・高台・場外の線
	/// @param params 弾の調整値
	void Step(float dt, const TENTACLE_TARGET* targets, int count, const STAGE& stage, const PROJECTILE_PARAMS& params);
	/// @brief 設置済みを全部取り出して消す（起爆）
	/// @param positions 設置されていた位置を書く並び（C4_MAX 個分）
	/// @return 取り出した数
	int Detonate(VEC3* positions);
	/// @brief 設置済みの数を取得する
	int GetPlacedCount() const { return m_PlacedCount; }
	/// @brief 設置済みの位置を取得する（0 がいちばん古い）
	/// @param index 番号（0〜GetPlacedCount()-1）
	const VEC3& GetPlaced(int index) const { return m_Placed[index]; }
	/// @brief 投げた弾を取得する（描画用）
	const ProjectileCalc& GetThrown() const { return m_Thrown; }
	/// @brief 相手に当たって落ちている弾を取得する（描画用）
	const ProjectileCalc& GetFalling() const { return m_Falling; }

private:
	void Place(const VEC3& position);
	ProjectileCalc m_Thrown; ///< 投げて飛んでいる弾
	ProjectileCalc m_Falling; ///< 相手に当たって真下へ落ちている弾（相手には当たらない）
	VEC3 m_Placed[C4_MAX] = {}; ///< 設置済みの位置（古い順）
	int m_PlacedCount = 0; ///< 設置済みの数
};
