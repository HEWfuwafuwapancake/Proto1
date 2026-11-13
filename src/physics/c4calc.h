#pragma once

#include "physics/projectilecalc.h"

/// @file c4calc.h
/// @brief C4 の弾を進め、着弾した位置と爆弾の大きさを返す（DirectX と Windows 不使用）

/// @brief このフレームに着弾して爆発する C4
struct C4_IMPACT
{
	VEC3 Position; ///< 相手または足場に当たった位置
	bool Large; ///< 大きさを設定する（true：大型、false：通常）
};

/// @brief 通常と大型の C4 を同じ弾の上限で管理し、着弾時に取り出す
class C4Calc
{
public:
	/// @brief 飛んでいる C4 をすべて消す
	void Reset();
	/// @brief 空いている所に C4 を投げ、出せたら true を返す
	/// @param origin 出す位置
	/// @param velocity 出す速度
	/// @param params 弾の調整値（非貫通の弾を使う）
	/// @param large 大きさを設定する（true：大型、false：通常）
	bool Throw(const VEC3& origin, const VEC3& velocity, const PROJECTILE_PARAMS& params, bool large);
	/// @brief 弾を進め、相手か足場に当たった C4 を返す（場外へ落ちた弾は返さず消す）
	/// @param dt 進める秒数
	/// @param targets 相手の並び
	/// @param count 相手の数
	/// @param stage 浮島・高台・場外の線
	/// @param params 弾の調整値
	/// @param impacts 着弾した位置と大きさを書く並び
	/// @param maxImpacts impacts の要素数（PROJECTILE_MAX 個なら全件受け取れる）
	/// @param expired 爆発せずに消えた弾の数を書く先（不要ならnullptr）
	/// @return 書いた着弾の数
	int Step(float dt, const TENTACLE_TARGET* targets, int count, const STAGE& stage,
		const PROJECTILE_PARAMS& params, C4_IMPACT* impacts, int maxImpacts, int* expired = nullptr);
	/// @brief 飛んでいる弾を取得する（描画用）
	const ProjectileCalc& GetThrown() const { return m_Thrown; }
	/// @brief 指定の弾が大型かを取得する（index は 0〜PROJECTILE_MAX-1、描画用）
	bool IsLarge(int index) const { return m_Large[index]; }

private:
	ProjectileCalc m_Thrown; ///< 飛んでいる弾（通常と大型を合わせて上限16発）
	bool m_Large[PROJECTILE_MAX] = {}; ///< 弾の番号ごとの大きさ
};
