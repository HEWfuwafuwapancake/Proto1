#include <math.h>
#include "physics/breachcalc.h"

// 調整用の値
static constexpr float BACKTRACK_STEP = 0.5f; // 線の先から手前へ戻って足場を探す間隔
static constexpr float MIN_AXIS = 0.0001f; // 線がその軸の面を横切ると言える最小の向き
static constexpr float BACK_OFF = 0.3f; // 天井・壁に当たった所から手前へ戻す距離（その下へ下ろす）
static constexpr float SIGHT_RAISE = 0.3f; // 見えるか調べるとき、着地点から上げる高さ（面すれすれを避ける）
static constexpr float EDGE_STEP = 0.5f; // 崖のふちを調べる点の間隔
static constexpr float EDGE_SIGHT_OFFSET = 0.05f; // 見えるか調べるとき、ふちから外・上へずらす距離
static constexpr float EDGE_MIN_RISE = 0.5f; // 足元よりこれだけ高い足場のふちだけを選ぶ
static constexpr float EDGE_HALF_ANGLE_COS = 0.5f; // 見ている水平の向きから左右 60° までのふちを選ぶ
static constexpr float EDGE_NEAR_HORIZONTAL = 1.0f; // 水平にこれより近いふちは向きを問わない
static constexpr float UP_VIEW_MIN = 0.25f; // 線の上向きがこれより大きければ「上を見ている」とする

namespace
{
	enum HIT_FACE
	{
		HIT_TOP, // 上の面（立てる）
		HIT_BOTTOM, // 下の面（天井）
		HIT_SIDE, // 横の面（壁）
	};

	// 線と足場の箱（上の面から厚さの分だけ下まで）が交わる所を調べる。始まりが箱の中なら交わらないとする。
	bool RayBox(const STAGE_PLATFORM& surface, const VEC3& origin, const VEC3& direction, float range,
		float& distance, HIT_FACE& face)
	{
		const float minimum[3] = { surface.CenterX - surface.HalfWidth, surface.TopY - surface.Thickness, surface.CenterZ - surface.HalfDepth };
		const float maximum[3] = { surface.CenterX + surface.HalfWidth, surface.TopY, surface.CenterZ + surface.HalfDepth };
		const float start[3] = { origin.X, origin.Y, origin.Z };
		const float step[3] = { direction.X, direction.Y, direction.Z };
		float enter = -1.0e30f;
		float exit = 1.0e30f;
		int enterAxis = -1;
		for (int axis = 0; axis < 3; ++axis) {
			if (fabsf(step[axis]) < MIN_AXIS) {
				if (start[axis] < minimum[axis] || start[axis] > maximum[axis]) return false;
				continue;
			}
			float low = (minimum[axis] - start[axis]) / step[axis];
			float high = (maximum[axis] - start[axis]) / step[axis];
			if (low > high) {
				const float swap = low;
				low = high;
				high = swap;
			}
			if (low > enter) {
				enter = low;
				enterAxis = axis;
			}
			exit = fminf(exit, high);
		}
		if (enterAxis < 0 || enter > exit || enter <= 0.0f || enter > range) return false;
		distance = enter;
		face = enterAxis != 1 ? HIT_SIDE : (direction.Y < 0.0f ? HIT_TOP : HIT_BOTTOM);
		return true;
	}

	// 浮島と高台のうち、線がいちばん手前で当たる面を探す（top には当たった足場の上の面の高さを書く）。
	bool FirstHit(const STAGE& stage, const VEC3& origin, const VEC3& direction, float range,
		float& distance, HIT_FACE& face, float& top)
	{
		bool found = false;
		const auto check = [&](const STAGE_PLATFORM& surface) {
			float d = 0.0f;
			HIT_FACE f = HIT_TOP;
			if (!RayBox(surface, origin, direction, found ? distance : range, d, f)) return;
			found = true;
			distance = d;
			face = f;
			top = surface.TopY;
		};
		check(StageCalcIslandSurface(stage));
		for (int i = 0; i < stage.PlatformCount; ++i) check(stage.Platforms[i]);
		return found;
	}

	// from から to が、浮島・高台にさえぎられずに見えるか。
	bool IsVisible(const STAGE& stage, const VEC3& from, const VEC3& to)
	{
		const VEC3 line = Vec3Sub(to, from);
		const float length = Vec3Length(line);
		if (length <= 0.0f) return true;
		float distance = 0.0f;
		HIT_FACE face = HIT_TOP;
		float top = 0.0f;
		return !FirstHit(stage, from, Vec3Scale(line, 1.0f / length), length, distance, face, top);
	}

	// その点の真下の足場へ下ろし、始まりから見えれば着地点にする。
	bool DropVisible(const STAGE& stage, const VEC3& origin, const VEC3& point, VEC3& landing)
	{
		float top = 0.0f;
		if (!StageCalcFindLanding(stage, point.X, point.Z, point.Y, stage.OutY, top)) return false;
		const VEC3 candidate{ point.X, top, point.Z };
		if (!IsVisible(stage, origin, { candidate.X, candidate.Y + SIGHT_RAISE, candidate.Z })) return false;
		landing = candidate;
		return true;
	}

	// 線の上を distance から手前へ戻りながら、真下に見える足場がある最初の点へ下ろす。
	bool Backtrack(const STAGE& stage, const VEC3& origin, const VEC3& direction, float distance, VEC3& landing)
	{
		for (float d = distance; d >= 0.0f; d -= BACKTRACK_STEP) {
			if (DropVisible(stage, origin, Vec3Add(origin, Vec3Scale(direction, d)), landing)) return true;
		}
		return false;
	}

	// 足元より高い足場のふちのうち、見ている向きにあり、見えていて、いちばん近い所の少し内側を探す。
	bool NearestVisibleEdge(const STAGE& stage, const VEC3& origin, const VEC3& direction, float range,
		float feetY, float inset, VEC3& landing)
	{
		const float flatLength = sqrtf(direction.X * direction.X + direction.Z * direction.Z);
		const bool hasFlat = flatLength > MIN_AXIS;
		bool found = false;
		float best = range;
		const auto checkPoint = [&](const STAGE_PLATFORM& surface, float x, float z, float outX, float outZ) {
			const float dx = x - origin.X;
			const float dz = z - origin.Z;
			const float flat = sqrtf(dx * dx + dz * dz);
			if (hasFlat && flat > EDGE_NEAR_HORIZONTAL
				&& (dx * direction.X + dz * direction.Z) / (flat * flatLength) < EDGE_HALF_ANGLE_COS) return;
			const float dy = surface.TopY - origin.Y;
			const float distance = sqrtf(flat * flat + dy * dy);
			if (distance > best) return;
			const VEC3 sight{ x + outX * EDGE_SIGHT_OFFSET, surface.TopY + EDGE_SIGHT_OFFSET, z + outZ * EDGE_SIGHT_OFFSET };
			if (!IsVisible(stage, origin, sight)) return;
			// ふちから内側へ入れる（足場が小さければ中心まで）。
			const float limitX = surface.HalfWidth - fminf(inset, surface.HalfWidth);
			const float limitZ = surface.HalfDepth - fminf(inset, surface.HalfDepth);
			found = true;
			best = distance;
			landing = { fmaxf(surface.CenterX - limitX, fminf(surface.CenterX + limitX, x)), surface.TopY,
				fmaxf(surface.CenterZ - limitZ, fminf(surface.CenterZ + limitZ, z)) };
		};
		const auto checkSurface = [&](const STAGE_PLATFORM& surface) {
			if (surface.TopY < feetY + EDGE_MIN_RISE) return;
			const float x0 = surface.CenterX - surface.HalfWidth;
			const float x1 = surface.CenterX + surface.HalfWidth;
			const float z0 = surface.CenterZ - surface.HalfDepth;
			const float z1 = surface.CenterZ + surface.HalfDepth;
			for (float x = x0; x <= x1; x += EDGE_STEP) {
				checkPoint(surface, x, z0, 0.0f, -1.0f);
				checkPoint(surface, x, z1, 0.0f, 1.0f);
			}
			for (float z = z0; z <= z1; z += EDGE_STEP) {
				checkPoint(surface, x0, z, -1.0f, 0.0f);
				checkPoint(surface, x1, z, 1.0f, 0.0f);
			}
		};
		checkSurface(StageCalcIslandSurface(stage));
		for (int i = 0; i < stage.PlatformCount; ++i) checkSurface(stage.Platforms[i]);
		return found;
	}
}

bool BreachCalcFindLanding(const STAGE& stage, const VEC3& origin, const VEC3& direction, float range,
	float feetY, float inset, VEC3& landing)
{
	float distance = 0.0f;
	HIT_FACE face = HIT_TOP;
	float top = 0.0f;
	if (FirstHit(stage, origin, direction, range, distance, face, top)) {
		if (face == HIT_TOP) {
			const VEC3 point = Vec3Add(origin, Vec3Scale(direction, distance));
			landing = { point.X, top, point.Z };
			return true;
		}
		// 天井・壁は通り抜けず、当たった所の手前の下へ下ろす。
		if (Backtrack(stage, origin, direction, fmaxf(0.0f, distance - BACK_OFF), landing)) return true;
		return NearestVisibleEdge(stage, origin, direction, range, feetY, inset, landing);
	}
	// 何にも当たらず上を見ているときは、見えているいちばん近い崖のふちへ。
	if (direction.Y > UP_VIEW_MIN) {
		return NearestVisibleEdge(stage, origin, direction, range, feetY, inset, landing)
			|| Backtrack(stage, origin, direction, range, landing);
	}
	return Backtrack(stage, origin, direction, range, landing)
		|| NearestVisibleEdge(stage, origin, direction, range, feetY, inset, landing);
}
