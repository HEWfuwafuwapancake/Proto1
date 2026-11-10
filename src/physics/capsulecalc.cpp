#include "physics/capsulecalc.h"
#include <algorithm>
#include <limits>

// 調整用の値
static constexpr int INITIAL_CAPSULE_COUNT = 8;
static constexpr double CAPSULE_SPAWN_SECONDS = 1.5;
static constexpr int PLATFORM_CHANCE_DIVISOR = 4; // 高台を選ぶのは4回に1回の確率
static constexpr float CAPSULE_EDGE_MARGIN = 1.25f;
static constexpr float CAPSULE_SPACING = 2.0f;
static constexpr int POSITION_ATTEMPTS = 8;
static constexpr float PICKUP_RADIUS = 0.65f;
static constexpr float PICKUP_CENTER_HEIGHT = 0.7f;

void ArmCapsuleCalc::Start(const STAGE& stage, uint32_t seed)
{
	m_Items = {};
	m_Random.SetSeed(seed);
	m_NextType = TYPE_COUNT;
	m_SpawnElapsed = 0.0;
	for (int i = 0; i < INITIAL_CAPSULE_COUNT; ++i) Spawn(stage);
}

void ArmCapsuleCalc::Step(const STAGE& stage, float dt)
{
	m_SpawnElapsed += dt;
	while (m_SpawnElapsed >= CAPSULE_SPAWN_SECONDS) {
		m_SpawnElapsed -= CAPSULE_SPAWN_SECONDS;
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

VEC3 ArmCapsuleCalc::ChoosePosition(const STAGE& stage)
{
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
