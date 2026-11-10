#pragma once

#include <cstdint>
#include "physics/randomcalc.h"
#include "physics/vec3.h"

/// @brief 復活先を相手の位置と試合ごとの乱数で選ぶ
class SpawnCalc
{
public:
	/// @brief 復活先を選ぶ乱数の種を設定する
	void SetSeed(uint32_t seed) { m_Random.SetSeed(seed); }
	/// @brief 安全な候補から無作為に選び、どれも近ければ最も近い相手までの距離が最大の候補を選ぶ
	/// @param points 復活先の候補
	/// @param pointCount 候補の数
	/// @param opponents 生存している相手の足元の位置
	/// @param opponentCount 相手の数
	/// @param safeDistance 各相手から離れているべき最小の距離（高さも含む）
	/// @return 選んだ候補の添字（候補がなければ -1）
	int ChoosePoint(const VEC3* points, int pointCount, const VEC3* opponents, int opponentCount, float safeDistance);

private:
	RandomCalc m_Random;
};
