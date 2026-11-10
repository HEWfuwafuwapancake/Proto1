#include "physics/spawncalc.h"
#include <limits>

int SpawnCalc::ChoosePoint(const VEC3* points, int pointCount, const VEC3* opponents, int opponentCount, float safeDistance)
{
	const float safeDistanceSq = safeDistance * safeDistance;
	float bestDistanceSq = -1.0f;
	int farthestPoint = -1;
	int safePoint = -1;
	int safeCount = 0;
	for (int i = 0; i < pointCount; ++i) {
		float nearestDistanceSq = std::numeric_limits<float>::infinity();
		for (int j = 0; j < opponentCount; ++j) {
			const float distanceSq = Vec3LengthSq(Vec3Sub(points[i], opponents[j]));
			if (distanceSq < nearestDistanceSq) nearestDistanceSq = distanceSq;
		}
		if (nearestDistanceSq > bestDistanceSq) {
			bestDistanceSq = nearestDistanceSq;
			farthestPoint = i;
		}
		if (nearestDistanceSq >= safeDistanceSq) {
			// 安全な候補が増えるたびに、候補数を使って選び直す。
			++safeCount;
			if (m_Random.NextInt(safeCount) == 0) safePoint = i;
		}
	}
	return safePoint >= 0 ? safePoint : farthestPoint;
}
