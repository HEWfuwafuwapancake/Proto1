#include <math.h>
#include "physics/breachcalc.h"

// 調整用の値
static constexpr float BACKTRACK_STEP = 0.5f; // 線の先から手前へ戻って足場を探す間隔
static constexpr float MIN_VERTICAL = 0.0001f; // 線が面を横切ると言える最小の上下の向き

static void CheckSurface(const STAGE_PLATFORM& surface, const VEC3& origin, const VEC3& direction, float range,
	bool& found, float& best, VEC3& landing)
{
	if (fabsf(direction.Y) < MIN_VERTICAL) return;
	const float distance = (surface.TopY - origin.Y) / direction.Y;
	if (distance <= 0.0f || distance > range || (found && distance >= best)) return;
	const float x = origin.X + direction.X * distance;
	const float z = origin.Z + direction.Z * distance;
	if (fabsf(x - surface.CenterX) > surface.HalfWidth || fabsf(z - surface.CenterZ) > surface.HalfDepth) return;
	found = true;
	best = distance;
	landing = { x, surface.TopY, z };
}

bool BreachCalcFindLanding(const STAGE& stage, const VEC3& origin, const VEC3& direction, float range, VEC3& landing)
{
	bool found = false;
	float best = 0.0f;
	CheckSurface(StageCalcIslandSurface(stage), origin, direction, range, found, best, landing);
	for (int i = 0; i < stage.PlatformCount; ++i) {
		CheckSurface(stage.Platforms[i], origin, direction, range, found, best, landing);
	}
	if (found) return true;
	for (float distance = range; distance >= 0.0f; distance -= BACKTRACK_STEP) {
		const VEC3 point = Vec3Add(origin, Vec3Scale(direction, distance));
		float top = 0.0f;
		if (StageCalcFindLanding(stage, point.X, point.Z, point.Y, stage.OutY, top)) {
			landing = { point.X, top, point.Z };
			return true;
		}
	}
	return false;
}
