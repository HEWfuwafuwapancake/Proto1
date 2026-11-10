#include "physics/randomcalc.h"

void RandomCalc::SetSeed(uint32_t seed)
{
	m_State = seed == 0 ? DEFAULT_SEED : seed;
}

uint32_t RandomCalc::Next()
{
	m_State ^= m_State << 13;
	m_State ^= m_State >> 17;
	m_State ^= m_State << 5;
	return m_State;
}

int RandomCalc::NextInt(int n)
{
	return static_cast<int>(Next() % static_cast<uint32_t>(n));
}

float RandomCalc::NextFloat(float minValue, float maxValue)
{
	// 上位24ビットを、float で正確に表せる 0 以上 1 未満の値にする。
	const float fraction = static_cast<float>(Next() >> 8) * (1.0f / 16777216.0f);
	return minValue + (maxValue - minValue) * fraction;
}
