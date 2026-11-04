#include "physics/warpcalc.h"

// 調整用の値
static constexpr int SEARCH_STEPS = 16; // 浮島のふちを探す二分探索の回数。

VEC3 WarpCalcNearTarget(const VEC3& start, const VEC3& target, float distance)
{
	const VEC3 flat{ target.X - start.X, 0.0f, target.Z - start.Z };
	const float length = Vec3Length(flat);
	if (length <= distance) return start;
	const VEC3 move = Vec3Scale(flat, (length - distance) / length);
	return { start.X + move.X, start.Y, start.Z + move.Z };
}

VEC3 WarpCalcDestination(const VEC3& start, const VEC3& desired, const STAGE& stage)
{
	if (StageCalcIsOnPlatform(stage, desired.X, desired.Z) || !StageCalcIsOnPlatform(stage, start.X, start.Z)) return desired;
	// start は浮島の上、desired は外。ふちを二分探索する。
	float inside = 0.0f, outside = 1.0f;
	for (int i = 0; i < SEARCH_STEPS; ++i) {
		const float middle = (inside + outside) * 0.5f;
		const float x = start.X + (desired.X - start.X) * middle;
		const float z = start.Z + (desired.Z - start.Z) * middle;
		if (StageCalcIsOnPlatform(stage, x, z)) inside = middle;
		else outside = middle;
	}
	return { start.X + (desired.X - start.X) * inside, desired.Y, start.Z + (desired.Z - start.Z) * inside };
}
