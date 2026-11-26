#include "physics/capsulecalc.h"
#include <algorithm>
#include <limits>

// 調整用の値
static constexpr int PLATFORM_CHANCE_DIVISOR = 4; // 高台を選ぶのは4回に1回の確率
static constexpr float CAPSULE_EDGE_MARGIN = 1.25f;
static constexpr float CAPSULE_SPACING = 2.0f;
static constexpr int POSITION_ATTEMPTS = 8;
static constexpr int TERRAIN_ATTEMPTS = 32; // 対戦マップで平らな床の点を引き直す回数（穴の多い市場でも見つかる回数）
static constexpr float PICKUP_RADIUS = 0.65f;
static constexpr float PICKUP_CENTER_HEIGHT = 0.7f;

void ArmCapsuleCalc::SetAmount(int initialCount, double spawnSeconds)
{
	m_InitialCount = (std::max)(0, (std::min)(CAPACITY, initialCount));
	if (spawnSeconds > 0.0) m_SpawnSeconds = spawnSeconds;
}

void ArmCapsuleCalc::Start(const STAGE& stage, uint32_t seed)
{
	m_Items = {};
	m_Random.SetSeed(seed);
	m_NextType = TYPE_COUNT;
	m_SpawnElapsed = 0.0;
	for (int i = 0; i < m_InitialCount; ++i) Spawn(stage);
}

void ArmCapsuleCalc::Step(const STAGE& stage, float dt)
{
	m_SpawnElapsed += dt;
	while (m_SpawnElapsed >= m_SpawnSeconds) {
		m_SpawnElapsed -= m_SpawnSeconds;
		Spawn(stage);
	}
}

int ArmCapsuleCalc::FindTouching(const CAPSULE& body) const
{
	for (int i = 0; i < CAPACITY; ++i) {
		const ARM_CAPSULE_ITEM& item = m_Items[i];
		if (!item.Active) continue;
		VEC3 center = item.Position;
		center.Y += PICKUP_CENTER_HEIGHT;
		if (CollisionSphereCapsule({ center, PICKUP_RADIUS }, body).Hit) return i;
	}
	return -1;
}

void ArmCapsuleCalc::Remove(int index)
{
	m_Items[index].Active = false;
}

void ArmCapsuleCalc::Spawn(const STAGE& stage)
{
	for (auto& item : m_Items) {
		if (item.Active) continue;
		item = { ChoosePosition(stage), DrawType(), true };
		return;
	}
}

// 地形のマップで、その点が平らな床の上で、ふちから CAPSULE_EDGE_MARGIN 以上内側か調べる
// （坂・ふちのそば・障害物（ビル・車・柱など）とジャンプ台の上には置かない）。
static bool IsFlatFloor(const STAGE& stage, float x, float z, float& height)
{
	const int region = StageCalcFindRegion(stage, x, z);
	if (region < 0 || stage.Regions[region].SlopeX != 0.0f || stage.Regions[region].SlopeZ != 0.0f) return false;
	const STAGE_SURFACE surface = stage.Regions[region].Surface;
	if (surface == STAGE_SURFACE_BLOCK || surface == STAGE_SURFACE_PILLAR || surface == STAGE_SURFACE_PAD) return false;
	height = StageCalcRegionHeight(stage.Regions[region], x, z);
	const float probes[4][2] = { { CAPSULE_EDGE_MARGIN, 0 }, { -CAPSULE_EDGE_MARGIN, 0 }, { 0, CAPSULE_EDGE_MARGIN }, { 0, -CAPSULE_EDGE_MARGIN } };
	for (const auto& probe : probes) {
		float around = 0.0f;
		if (!StageCalcGroundHeight(stage, x + probe[0], z + probe[1], around) || fabsf(around - height) > 0.01f) return false;
	}
	return true;
}

VEC3 ArmCapsuleCalc::ChoosePosition(const STAGE& stage)
{
	if (stage.HasTerrain) {
		// 対戦マップは、全体を囲む四角の中から平らな床の点を引き、ほかのカプセルとの間隔が取れる点を選ぶ。
		VEC3 best{ stage.CenterX, stage.TopY, stage.CenterZ };
		float bestDistanceSq = -1.0f;
		for (int i = 0; i < TERRAIN_ATTEMPTS; ++i) {
			const float x = stage.CenterX + m_Random.NextFloat(-stage.HalfWidth, stage.HalfWidth);
			const float z = stage.CenterZ + m_Random.NextFloat(-stage.HalfDepth, stage.HalfDepth);
			float height = 0.0f;
			if (!IsFlatFloor(stage, x, z, height)) continue;
			const VEC3 position{ x, height, z };
			const float distanceSq = NearestDistanceSq(position);
			if (distanceSq >= CAPSULE_SPACING * CAPSULE_SPACING) return position;
			if (distanceSq > bestDistanceSq) {
				best = position;
				bestDistanceSq = distanceSq;
			}
		}
		return best;
	}
	const bool onPlatform = stage.PlatformCount > 0 && m_Random.NextInt(PLATFORM_CHANCE_DIVISOR) == 0;
	const STAGE_PLATFORM surface = onPlatform
		? stage.Platforms[m_Random.NextInt(stage.PlatformCount)] : StageCalcIslandSurface(stage);
	const float halfWidth = fmaxf(0.0f, surface.HalfWidth - CAPSULE_EDGE_MARGIN);
	const float halfDepth = fmaxf(0.0f, surface.HalfDepth - CAPSULE_EDGE_MARGIN);
	VEC3 best{};
	float bestDistanceSq = -1.0f;
	for (int i = 0; i < POSITION_ATTEMPTS; ++i) {
		const VEC3 position = {
			surface.CenterX + m_Random.NextFloat(-halfWidth, halfWidth), surface.TopY,
			surface.CenterZ + m_Random.NextFloat(-halfDepth, halfDepth),
		};
		const float distanceSq = NearestDistanceSq(position);
		if (distanceSq >= CAPSULE_SPACING * CAPSULE_SPACING) return position;
		if (distanceSq > bestDistanceSq) {
			best = position;
			bestDistanceSq = distanceSq;
		}
	}
	// 引き直しても混み合っているときは、候補の中で最も間隔が取れる点に置く。
	return best;
}

float ArmCapsuleCalc::NearestDistanceSq(const VEC3& position) const
{
	float distanceSq = std::numeric_limits<float>::max();
	for (const auto& item : m_Items) {
		if (item.Active) distanceSq = fminf(distanceSq, Vec3LengthSq(Vec3Sub(position, item.Position)));
	}
	return distanceSq;
}

int ArmCapsuleCalc::DrawType()
{
	if (m_NextType == TYPE_COUNT) {
		for (int i = 0; i < TYPE_COUNT; ++i) m_Bag[i] = i;
		for (int i = TYPE_COUNT - 1; i > 0; --i) std::swap(m_Bag[i], m_Bag[m_Random.NextInt(i + 1)]);
		m_NextType = 0;
	}
	return m_Bag[m_NextType++];
}
