#include "physics/guardcalc.h"
#include <math.h>

// 調整用の値
static constexpr float MIN_LENGTH = 0.0001f; // これより短い向きは「向きなし」とみなす。

bool GuardCalcBlocks(const GUARD& guard, const VEC3& center, const VEC3& from)
{
	if (guard.Kind == GUARD_ALL) return true;
	if (guard.Kind != GUARD_FRONT) return false;
	const VEC3 flat{ from.X - center.X, 0.0f, from.Z - center.Z };
	const float distance = Vec3Length(flat);
	if (distance < MIN_LENGTH) return true;
	const VEC3 forward{ guard.Forward.X, 0.0f, guard.Forward.Z };
	const float forwardLength = Vec3Length(forward);
	if (forwardLength < MIN_LENGTH) return false;
	const float cosine = Vec3Dot(flat, forward) / (distance * forwardLength);
	return cosine >= cosf(guard.HalfAngle);
}

GUARD GuardCalcStronger(const GUARD& a, const GUARD& b)
{
	return b.Kind > a.Kind ? b : a;
}
