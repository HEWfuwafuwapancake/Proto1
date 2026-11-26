#include <math.h>
#include "physics/stagecalc.h"
#include <float.h>

// 調整用の値
static constexpr float GRIP_SPACING = 0.25f;			// ふちを調べる点の間隔
static constexpr float STANDING_TOLERANCE = 0.01f;		// この高さの差までなら「その足場に立っている」とみなす
static constexpr float GRIP_MIN_HORIZONTAL = 0.0001f;	// 水平の向きを決められる最小の長さ
static constexpr float GROUND_STEP = 0.4f;				// 地形のマップで、立っている体が坂として上り下りできる高さの差（1フレーム・体の半径の分）
static constexpr float AIR_STEP = 0.25f;				// 地形のマップで、空中の体が側面に止められずに越えられ、着地で拾われる高さの差
static constexpr float SUPPORT_TOLERANCE = 0.0005f;		// 地形のマップで「その高さに床がある」とみなす差
static constexpr float TERRAIN_GRIP_SPACING = 0.5f;		// 地形のふちを調べる点の間隔
static constexpr float TERRAIN_EDGE_PROBE = 0.3f;		// ふちの外側に床がないか調べる距離
static constexpr float TERRAIN_GRIP_INSET = 0.6f;		// 地形のふちをつかんだときの着地点（ふちから内側への距離）
static constexpr int BLOCK_SAMPLES = 24;				// 地形のマップで、線が地形をくぐるか調べる点の数
static constexpr float BRIDGE_THICKNESS = 0.5f;			// 片面の橋を、線をさえぎる板として見る厚さ
static constexpr int BODY_SAMPLES = 8;					// 体の側面を調べる、円周上の点の数


static constexpr float ABOVE_ALL = FLT_MAX;			// 足の高さを決めずに調べるとき（片面の橋も含めて、いちばん上の区画を使う）


bool StageCalcIsInsideRegion(const STAGE_REGION& region, float x, float z)
{
	const float centerX = (region.MinX + region.MaxX) * 0.5f;
	const float centerZ = (region.MinZ + region.MaxZ) * 0.5f;
	if (region.Yaw == 0.0f) {
		if (x < region.MinX || x > region.MaxX || z < region.MinZ || z > region.MaxZ) return false;
	}
	else {
		// 世界の点を区画の向きへ戻してから、回す前の範囲とくらべる。
		const float dx = x - centerX, dz = z - centerZ;
		const float c = cosf(region.Yaw), s = sinf(region.Yaw);
		if (fabsf(dx * c - dz * s) > (region.MaxX - region.MinX) * 0.5f
			|| fabsf(dx * s + dz * c) > (region.MaxZ - region.MinZ) * 0.5f) return false;
	}
	if (region.Radius > 0.0f) {
		const float dx = x - centerX, dz = z - centerZ;
		if (dx * dx + dz * dz > region.Radius * region.Radius) return false;
	}
	return region.DiagonalLimit <= 0.0f || fabsf(x) + fabsf(z) <= region.DiagonalLimit;
}

float StageCalcRegionHeight(const STAGE_REGION& region, float x, float z)
{
	return region.BaseY + region.SlopeX * x + region.SlopeZ * z;
}

// 足（reference）から見た床の区画を上から順に探す。片面の橋は、足が上面から reach より下にあればくぐるので飛ばす。
static int FindSurface(const STAGE& stage, float x, float z, float reference, float reach, float& height)
{
	if (!stage.HasTerrain) return -1;
	for (int i = 0; i < stage.RegionCount; ++i) {
		const STAGE_REGION& region = stage.Regions[i];
		if (!StageCalcIsInsideRegion(region, x, z)) continue;
		const float top = StageCalcRegionHeight(region, x, z);
		if (!region.Solid && reference < top - reach) continue;
		height = top;
		return i;
	}
	return -1;
}

int StageCalcFindRegion(const STAGE& stage, float x, float z)
{
	float height = 0.0f;
	return FindSurface(stage, x, z, ABOVE_ALL, 0.0f, height);
}

int StageCalcFindFloor(const STAGE& stage, float x, float y, float z)
{
	float height = 0.0f;
	return FindSurface(stage, x, z, y, GROUND_STEP, height);
}

// 地形のマップで、その場所のいちばん上の上面の高さを求める（床がなければ false）。
static bool TerrainHeight(const STAGE& stage, float x, float z, float& height, int* index = nullptr)
{
	const int region = FindSurface(stage, x, z, ABOVE_ALL, 0.0f, height);
	if (index) *index = region;
	return region >= 0;
}

// 足の高さが、実体の区画の中（地盤の下面より上で、上面より下）にあるか調べる。
static bool IsInsideSolid(const STAGE& stage, int region, float height, float y)
{
	return region >= 0 && stage.Regions[region].Solid && y < height && y > stage.GroundBottomY;
}

bool StageCalcGroundHeight(const STAGE& stage, float x, float z, float& height)
{
	if (stage.HasTerrain) return TerrainHeight(stage, x, z, height);
	bool found = StageCalcIsOnPlatform(stage, x, z);
	if (found) height = stage.TopY;
	for (int i = 0; i < stage.PlatformCount; ++i) {
		const STAGE_PLATFORM& platform = stage.Platforms[i];
		if (fabsf(x - platform.CenterX) > platform.HalfWidth || fabsf(z - platform.CenterZ) > platform.HalfDepth) continue;
		if (!found || platform.TopY > height) height = platform.TopY;
		found = true;
	}
	return found;
}

bool StageCalcFollowGround(const STAGE& stage, float x, float y, float z, float& groundY)
{
	if (!stage.HasTerrain) {
		groundY = y;
		return StageCalcHasSupport(stage, x, y, z);
	}
	float height = 0.0f;
	const int region = FindSurface(stage, x, z, y, GROUND_STEP, height);
	if (region < 0) return false;
	// 段差の上限の中なら坂として合わせる。押し合い・引っぱりなどで実体の区画の中（上面より下）に入っていたら、
	// すり抜けて落ちないように上面へ押し上げる（歩いては側面で止まるので、ふつうは入らない）。
	if (fabsf(height - y) > GROUND_STEP && !IsInsideSolid(stage, region, height, y)) return false;
	groundY = height;
	return true;
}

// 足元がその場所にあるとき、体が地形の側面に食い込むか調べる（足の高さより step 以上高い実体の区画が、体の円の中にある）。
static bool IsBodyBlocked(const STAGE& stage, float x, float y, float z, float step)
{
	if (y + STAGE_BODY_HEIGHT <= stage.GroundBottomY) return false; // 地盤の下面より下を通っている
	for (int i = 0; i <= BODY_SAMPLES; ++i) {
		float px = x, pz = z;
		if (i < BODY_SAMPLES) {
			const float angle = 6.2831853f * i / BODY_SAMPLES;
			px += cosf(angle) * STAGE_BODY_RADIUS;
			pz += sinf(angle) * STAGE_BODY_RADIUS;
		}
		// 足より上の片面の橋はくぐるので、その下の区画で調べる（橋そのものは壁にならない）。
		float height = 0.0f;
		const int region = FindSurface(stage, px, pz, y, step, height);
		if (region < 0 || !stage.Regions[region].Solid) continue;
		if (height > y + step) return true;
	}
	return false;
}

int StageCalcMoveBody(const STAGE& stage, float x, float y, float z, float dx, float dz, bool onGround,
	float& outX, float& outZ)
{
	outX = x + dx;
	outZ = z + dz;
	if (!stage.HasTerrain || (dx == 0.0f && dz == 0.0f)) return 0;
	const float step = onGround ? GROUND_STEP : AIR_STEP;
	// 押し込まれてすでに壁の中にいるときは、抜け出せるようにそのまま動かす。
	if (IsBodyBlocked(stage, x, y, z, step) || !IsBodyBlocked(stage, outX, y, outZ, step)) return 0;
	// 斜めに当たったら、止まらない軸の分だけ壁に沿って動かす。
	if (dx != 0.0f && !IsBodyBlocked(stage, x + dx, y, z, step)) { outZ = z; return STAGE_STOP_Z; }
	if (dz != 0.0f && !IsBodyBlocked(stage, x, y, z + dz, step)) { outX = x; return STAGE_STOP_X; }
	outX = x;
	outZ = z;
	return STAGE_STOP_X | STAGE_STOP_Z;
}

int StageCalcRegionOutline(const STAGE_REGION& region, float corners[][2])
{
	const float centerX = (region.MinX + region.MaxX) * 0.5f;
	const float centerZ = (region.MinZ + region.MaxZ) * 0.5f;
	if (region.Radius > 0.0f) {
		// 円は、上から見て反時計回り（X から Z の向き）の多角形にする。
		for (int i = 0; i < STAGE_OUTLINE_MAX; ++i) {
			const float angle = 6.2831853f * i / STAGE_OUTLINE_MAX;
			corners[i][0] = centerX + cosf(angle) * region.Radius;
			corners[i][1] = centerZ + sinf(angle) * region.Radius;
		}
		return STAGE_OUTLINE_MAX;
	}
	if (region.DiagonalLimit <= 0.0f) {
		const float rect[4][2] = { { region.MinX, region.MinZ }, { region.MaxX, region.MinZ },
			{ region.MaxX, region.MaxZ }, { region.MinX, region.MaxZ } };
		const float c = cosf(region.Yaw), s = sinf(region.Yaw);
		for (int i = 0; i < 4; ++i) {
			// 回した区画は、範囲の中心のまわりに回す（回しても並びの向きは変わらない）。
			const float lx = rect[i][0] - centerX, lz = rect[i][1] - centerZ;
			corners[i][0] = centerX + lx * c + lz * s;
			corners[i][1] = centerZ - lx * s + lz * c;
		}
		return 4;
	}
	// 原点を中心にした左右・前後対称の区画を、|x|＋|z| ≤ DiagonalLimit で切り落とした8角形。
	const float hx = region.MaxX, hz = region.MaxZ, c = region.DiagonalLimit;
	const float oct[8][2] = { { -(c - hz), hz }, { -hx, c - hx }, { -hx, -(c - hx) }, { -(c - hz), -hz },
		{ c - hz, -hz }, { hx, -(c - hx) }, { hx, c - hx }, { c - hz, hz } };
	for (int i = 0; i < 8; ++i) { corners[i][0] = oct[i][0]; corners[i][1] = oct[i][1]; }
	return 8;
}

// 地形のマップで、外周・穴に面した床のふち（外側に床がないふち）を探す。
static void SearchTerrainRim(const STAGE& stage, const VEC3& origin, const VEC3& forward, float cosine,
	float maxDistance, bool lookingUp, bool& found, float& best, STAGE_GRIP& grip)
{
	for (int index = 0; index < stage.RegionCount; ++index) {
		const STAGE_REGION& region = stage.Regions[index];
		float corners[STAGE_OUTLINE_MAX][2];
		const int count = StageCalcRegionOutline(region, corners);
		for (int side = 0; side < count; ++side) {
			const float* a = corners[side];
			const float* b = corners[(side + 1) % count];
			const float edgeX = b[0] - a[0], edgeZ = b[1] - a[1];
			const float length = sqrtf(edgeX * edgeX + edgeZ * edgeZ);
			if (length < GRIP_MIN_HORIZONTAL) continue;
			// 角は反時計回り（上から見て）に並ぶので、辺の右が外側。
			const float outX = edgeZ / length, outZ = -edgeX / length;
			const int steps = (int)ceilf(length / TERRAIN_GRIP_SPACING);
			for (int i = 0; i <= steps; ++i) {
				const float t = (float)i / steps;
				const float x = a[0] + edgeX * t, z = a[1] + edgeZ * t;
				const float innerX = x - outX * TERRAIN_GRIP_INSET, innerZ = z - outZ * TERRAIN_GRIP_INSET;
				float outside = 0.0f, inner = 0.0f;
				int innerRegion = -1;
				// 外側にも床がある継ぎ目と、ほかの区画が上に重なっているふちは使わない。
				if (TerrainHeight(stage, x + outX * TERRAIN_EDGE_PROBE, z + outZ * TERRAIN_EDGE_PROBE, outside)) continue;
				if (!TerrainHeight(stage, innerX, innerZ, inner, &innerRegion) || innerRegion != index) continue;
				const VEC3 point{ x, StageCalcRegionHeight(region, x, z), z };
				const float flatX = point.X - origin.X, flatZ = point.Z - origin.Z;
				const float flat = sqrtf(flatX * flatX + flatZ * flatZ);
				const float distance = Vec3Length(Vec3Sub(point, origin));
				if (distance > maxDistance || flat < GRIP_MIN_HORIZONTAL) continue;
				const float alignment = lookingUp ? Vec3Dot(Vec3Sub(point, origin), forward) / distance
					: (flatX * forward.X + flatZ * forward.Z) / flat;
				const float score = lookingUp ? alignment : -distance;
				if (alignment < cosine || (found && score <= best)) continue;
				found = true;
				best = score;
				// 着地点はふちの少し内側。大きさ 0 の足場にして、着地の計算がそのまま中心を使うようにする。
				grip = { point, { innerX, innerZ, 0.0f, 0.0f, inner, 0.5f } };
			}
		}
	}
}


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
	if (stage.HasTerrain) return StageCalcFindRegion(stage, x, z) >= 0;
	return fabsf(x - stage.CenterX) <= stage.HalfWidth
		&& fabsf(z - stage.CenterZ) <= stage.HalfDepth;
}

bool StageCalcHasSupport(const STAGE& stage, float x, float y, float z)
{
	if (stage.HasTerrain) {
		float height = 0.0f;
		return FindSurface(stage, x, z, y, GROUND_STEP, height) >= 0 && fabsf(height - y) <= SUPPORT_TOLERANCE;
	}
	float landingY = 0.0f;
	return StageCalcFindLanding(stage, x, z, y, y, landingY);
}

bool StageCalcFindLanding(const STAGE& stage, float x, float z, float previousY, float y, float& landingY)
{
	if (y > previousY) return false;
	if (stage.HasTerrain) {
		// 片面の橋は、上から通った分だけ（移動前に上面より下にいたら、くぐって下の区画へ落ちる）。
		float height = 0.0f;
		const int region = FindSurface(stage, x, z, previousY, 0.0f, height);
		if (region < 0) return false;
		// 実体の区画は、少し下から斜めに入ったとき（坂へ跳び込んだときなど）も上面に拾う。片面の橋は上から通った分だけ。
		// 押し合い・引っぱりなどで実体の区画の中に入っていた体も、すり抜けて落ちないように上面に拾う。
		const float reach = stage.Regions[region].Solid ? AIR_STEP : 0.0f;
		if (y > height) return false;
		if (previousY < height - reach && !IsInsideSolid(stage, region, height, previousY)) return false;
		landingY = height;
		return true;
	}

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
	if (stage.HasTerrain) {
		// 床の上に立っている（その場所の上面より下にいない）ときは、ふちへ伸ばしても意味がない。
		float height = 0.0f;
		if (FindSurface(stage, feet.X, feet.Z, feet.Y, GROUND_STEP, height) >= 0 && feet.Y >= height - STANDING_TOLERANCE) return false;
		SearchTerrainRim(stage, origin, unit, cosine, maxDistance, lookingUp, found, best, grip);
		return found;
	}
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

// 線が1つの足場の上の面を上下に通り抜けるか調べる。
static bool CrossesSurface(const STAGE_PLATFORM& surface, const VEC3& from, const VEC3& to)
{
	const float low = fminf(from.Y, to.Y);
	const float high = fmaxf(from.Y, to.Y);
	if (!(low < surface.TopY && surface.TopY < high)) return false;
	const float t = (surface.TopY - from.Y) / (to.Y - from.Y);
	const float x = from.X + (to.X - from.X) * t;
	const float z = from.Z + (to.Z - from.Z) * t;
	return fabsf(x - surface.CenterX) <= surface.HalfWidth && fabsf(z - surface.CenterZ) <= surface.HalfDepth;
}

bool StageCalcIsBlocked(const STAGE& stage, const VEC3& from, const VEC3& to)
{
	if (stage.HasTerrain) {
		// 線の途中の点が、実体の区画の中（地盤の下面から上面）か、片面の橋の板の中にあればさえぎられる。
		// 片面の橋より下の点は、その下の区画も調べる（橋の下をくぐる線）。
		for (int i = 1; i < BLOCK_SAMPLES; ++i) {
			const VEC3 point = Vec3Add(from, Vec3Scale(Vec3Sub(to, from), (float)i / BLOCK_SAMPLES));
			for (int r = 0; r < stage.RegionCount; ++r) {
				const STAGE_REGION& region = stage.Regions[r];
				if (!StageCalcIsInsideRegion(region, point.X, point.Z)) continue;
				const float height = StageCalcRegionHeight(region, point.X, point.Z);
				const float bottom = region.Solid ? stage.GroundBottomY : height - BRIDGE_THICKNESS;
				if (point.Y < height && point.Y > bottom) return true;
				if (region.Solid || point.Y >= height) break;
			}
		}
		return false;
	}
	if (CrossesSurface(StageCalcIslandSurface(stage), from, to)) return true;
	for (int i = 0; i < stage.PlatformCount; ++i) {
		if (CrossesSurface(stage.Platforms[i], from, to)) return true;
	}
	return false;
}

bool StageCalcIsOut(const STAGE& stage, float y)
{
	return y < stage.OutY;
}

bool StageCalcIsSideOut(const STAGE& stage, float x, float z)
{
	if (stage.SideOutDistance < 0.0f) return false;
	const float dx = x - stage.CenterX;
	const float dz = z - stage.CenterZ;
	if (stage.RoundOutline) {
		const float radius = stage.HalfWidth + stage.SideOutDistance;
		return dx * dx + dz * dz > radius * radius;
	}
	return fabsf(dx) > stage.HalfWidth + stage.SideOutDistance || fabsf(dz) > stage.HalfDepth + stage.SideOutDistance;
}

bool StageCalcIsRingOut(const STAGE& stage, const VEC3& feet)
{
	return StageCalcIsOut(stage, feet.Y) || StageCalcIsSideOut(stage, feet.X, feet.Z);
}
