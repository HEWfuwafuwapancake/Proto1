#pragma once

#include <cstdint>

/// @brief 同じ種から同じ乱数の並びを作る（xorshift32）
class RandomCalc
{
public:
	/// @brief 乱数の種を設定する（0 は既定の種に置き換える）
	void SetSeed(uint32_t seed);
	/// @brief 0 以上 n 未満の整数を取得する（n は正の数）
	int NextInt(int n);
	/// @brief 指定した最小値と最大値の間の実数を取得する
	float NextFloat(float minValue, float maxValue);

private:
	uint32_t Next();
	static constexpr uint32_t DEFAULT_SEED = 1u;
	uint32_t m_State = DEFAULT_SEED;
};
