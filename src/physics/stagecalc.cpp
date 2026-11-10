#include <math.h>
#include "physics/stagecalc.h"

// 調整用の値
static constexpr float GRIP_SPACING = 0.25f;			// ふちを調べる点の間隔
static constexpr float STANDING_TOLERANCE = 0.01f;		// この高さの差までなら「その足場に立っている」とみなす
static constexpr float GRIP_MIN_HORIZONTAL = 0.0001f;	// 水平の向きを決められる最小の長さ


static bool IsInsideSurface(const STAGE_PLATFORM& surface, float x, float z)
{
	return fabsf(x - surface.CenterX) <= surface.HalfWidth && fabsf(z - surface.CenterZ) <= surface.HalfDepth;
}

static void SearchRim(const STAGE_PLATFORM& surface, const VEC3& origin, const VEC3& forward, float cosine,
	float maxDistance, bool lookingUp, bool& found, float& best, STAGE_GRIP& grip)
{
	const float corners[4][2] = {
		{ surface.CenterX - surface.HalfWidth, surface.CenterZ - surface.HalfDepth },
		{ surface.CenterX + surface.HalfWidth, surface.CenterZ - surface.HalfDepth },
		{ surface.CenterX + surface.HalfWidth, surface.CenterZ + surface.HalfDepth },
		{ surface.CenterX - surface.HalfWidth, surface.CenterZ + surface.HalfDepth },
	};
	for (int side = 0; side < 4; ++side) {
		const float* a = corners[side];
		const float* b = corners[(side + 1) % 4];
		const float length = fabsf(b[0] - a[0]) + fabsf(b[1] - a[1]);
		const int count = length > GRIP_SPACING ? (int)ceilf(length / GRIP_SPACING) : 1;
		for (int i = 0; i < count; ++i) {
			const float t = (float)i / count;
			const VEC3 point{ a[0] + (b[0] - a[0]) * t, surface.TopY, a[1] + (b[1] - a[1]) * t };
			const float flatX = point.X - origin.X;
			const float flatZ = point.Z - origin.Z;
			const float flat = sqrtf(flatX * flatX + flatZ * flatZ);
			const float distance = Vec3Length(Vec3Sub(point, origin));
			if (distance > maxDistance || flat < GRIP_MIN_HORIZONTAL) continue;
			// 見上げているときは上下も含めて比べ、画面の中央にいちばん近いふちを選ぶ。それ以外は水平に比べ、いちばん近いふちを選ぶ。
			const float alignment = lookingUp ? Vec3Dot(Vec3Sub(point, origin), forward) / distance
				: (flatX * forward.X + flatZ * forward.Z) / flat;
			const float score = lookingUp ? alignment : -distance;
			if (alignment < cosine || (found && score <= best)) continue;
			found = true;
			best = score;
			grip = { point, surface };
		}
	}
}

bool StageCalcIsOnPlatform(const STAGE& stage, float x, float z)
{
	return fabsf(x - stage.CenterX) <= stage.HalfWidth
		&& fabsf(z - stage.CenterZ) <= stage.HalfDepth;
}

bool StageCalcHasSupport(const STAGE& stage, float x, float y, float z)
{
	float landingY = 0.0f;
	return StageCalcFindLanding(stage, x, z, y, y, landingY);
}

bool StageCalcFindLanding(const STAGE& stage, float x, float z, float previousY, float y, float& landingY)
{
	if (y > previousY) return false;

	bool found = y <= stage.TopY && stage.TopY <= previousY && StageCalcIsOnPlatform(stage, x, z);
	if (found) landingY = stage.TopY;
	for (int i = 0; i < stage.PlatformCount; ++i) {
		const STAGE_PLATFORM& platform = stage.Platforms[i];
		if (platform.TopY < y || platform.TopY > previousY || (found && platform.TopY <= landingY)) continue;
		if (fabsf(x - platform.CenterX) <= platform.HalfWidth
			&& fabsf(z - platform.CenterZ) <= platform.HalfDepth) {
			landingY = platform.TopY;
			found = true;
		}
	}
	return found;
}

STAGE_PLATFORM StageCalcIslandSurface(const STAGE& stage)
{
	return { stage.CenterX, stage.CenterZ, stage.HalfWidth, stage.HalfDepth, stage.TopY, stage.Thickness };
}

bool StageCalcFindGrip(const STAGE& stage, const VEC3& origin, const VEC3& feet, const VEC3& forward,
	float maxDistance, float halfAngle, STAGE_GRIP& grip)
{
	const float flat = sqrtf(forward.X * forward.X + forward.Z * forward.Z);
	if (flat < GRIP_MIN_HORIZONTAL) return false;
	const bool lookingUp = forward.Y > 0.0f;
	const VEC3 unit = lookingUp ? Vec3Scale(forward, 1.0f / Vec3Length(forward)) : VEC3{ forward.X / flat, 0.0f, forward.Z / flat };
	const float cosine = cosf(halfAngle);
	bool found = false;
	float best = 0.0f;
	for (int i = 0; i < stage.PlatformCount; ++i) {
		const STAGE_PLATFORM& platform = stage.Platforms[i];
		if (fabsf(feet.Y - platform.TopY) <= STANDING_TOLERANCE && IsInsideSurface(platform, feet.X, feet.Z)) continue;
		SearchRim(platform, origin, unit, cosine, maxDistance, lookingUp, found, best, grip);
	}
	// 浮島の上にいるときは、浮島のふちへ伸ばしても意味がない。
	if (!StageCalcIsOnPlatform(stage, feet.X, feet.Z) || feet.Y < stage.TopY - STANDING_TOLERANCE) {
		SearchRim(StageCalcIslandSurface(stage), origin, unit, cosine, maxDistance, lookingUp, found, best, grip);
	}
	return found;
}

bool StageCalcIsOut(const STAGE& stage, float y)
{
	return y < stage.OutY;
}
