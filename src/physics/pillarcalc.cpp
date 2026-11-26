#include <math.h>
#include "physics/pillarcalc.h"

//------------------------------------------------------------------------------
//	調整用の値（仕様書の初期値。当たったときのダメージ・吹っ飛びは game/mapgimmick.cpp）
//------------------------------------------------------------------------------
static constexpr float DURABILITY = 65.0f;			// 柱の耐久
static constexpr float WARNING_SECONDS = 0.38f;		// 耐久が尽きてから倒れ始めるまでの予兆
static constexpr float FALL_SECONDS = 0.80f;		// 直立から水平まで倒れる時間
static constexpr float HIT_INTERVAL = 0.3f;			// 同じ元（人の片手）の攻撃が、同じ柱に続けて当たらない秒数（1振りで1回）
static constexpr float SWEEP_STEP = 0.24f;			// 倒れる柱の通り道を調べる間隔（柱の先が進む距離）
static constexpr float STUMP_WIDTH_RATE = 0.725f;	// 倒れている途中の根元の台の幅（柱の幅に対して）
static constexpr float STUMP_HEIGHT = 0.24f;		// 倒れている途中の根元の台の高さ
static constexpr float WOBBLE_ANGLE = 0.019f;		// 予兆の揺れの角度（ラジアン。見た目だけ）
static constexpr float WOBBLE_SPEED = 69.0f;		// 予兆の揺れの速さ（見た目だけ）
static constexpr float HALF_PI = 1.57079633f;
static constexpr float MIN_HORIZONTAL = 1.0e-4f;	// 攻撃の向きの水平の長さがこれより短ければ、向きなしとみなす


// 倒れ方（0〜1）から柱の傾き。重心が台から外れるほど速く倒れる（倒れ方の2乗 × 90°）。
static float FallAngle(float fall)
{
	return fall * fall * HALF_PI;
}

static float HalfWidth(const STAGE_REGION& region)
{
	return (region.MaxX - region.MinX) * 0.5f;
}

void PillarCalc::Setup(const STAGE& stage)
{
	m_Count = 0;
	m_Clock = 0.0f;
	m_BaseY = stage.TopY;
	for (int i = 0; i < stage.RegionCount && m_Count < PILLAR_MAX; ++i) {
		if (stage.Regions[i].Surface != STAGE_SURFACE_PILLAR) continue;
		PILLAR& pillar = m_Pillars[m_Count++];
		pillar = PILLAR{};
		pillar.Region = i;
		pillar.Home = stage.Regions[i];
		pillar.Durability = DURABILITY;
		for (float& time : pillar.LastHit) time = -HIT_INTERVAL;
	}
}

bool PillarCalc::GetBody(int index, CAPSULE& body) const
{
	if (index < 0 || index >= m_Count || m_Pillars[index].State != PILLAR_STANDING) return false;
	const PILLAR& pillar = m_Pillars[index];
	const float radius = HalfWidth(pillar.Home);
	const float x = (pillar.Home.MinX + pillar.Home.MaxX) * 0.5f, z = (pillar.Home.MinZ + pillar.Home.MaxZ) * 0.5f;
	body = { { x, m_BaseY + radius, z }, { x, pillar.Home.BaseY - radius, z }, radius };
	return true;
}

bool PillarCalc::Hit(int index, int source, int attacker, float damage, const VEC3& direction)
{
	if (index < 0 || index >= m_Count || source < 0 || source >= PILLAR_SOURCE_MAX || damage <= 0.0f) return false;
	PILLAR& pillar = m_Pillars[index];
	if (pillar.State != PILLAR_STANDING || m_Clock - pillar.LastHit[source] < HIT_INTERVAL) return false;
	pillar.LastHit[source] = m_Clock;
	pillar.Durability = fmaxf(0.0f, pillar.Durability - damage);
	if (pillar.Durability > 0.0f) return true;
	// 耐久が尽きたら、最後の攻撃の進む向き（水平）へ倒す。
	const float horizontal = sqrtf(direction.X * direction.X + direction.Z * direction.Z);
	pillar.Direction = horizontal < MIN_HORIZONTAL ? VEC3{ 0.0f, 0.0f, 1.0f }
		: VEC3{ direction.X / horizontal, 0.0f, direction.Z / horizontal };
	pillar.Attacker = attacker;
	pillar.State = PILLAR_WARNING;
	pillar.Timer = WARNING_SECONDS;
	pillar.HitMask = 0;
	return true;
}

// 傾きが angle のときの根元の中心（倒れるほど、横たわった柱の太さの半分まで浮く）。
VEC3 PillarCalc::GetBase(const PILLAR& pillar, float angle) const
{
	const float x = (pillar.Home.MinX + pillar.Home.MaxX) * 0.5f, z = (pillar.Home.MinZ + pillar.Home.MaxZ) * 0.5f;
	return { x, m_BaseY + HalfWidth(pillar.Home) * sinf(angle), z };
}

STAGE_REGION PillarCalc::GetRegion(const PILLAR& pillar) const
{
	if (pillar.State == PILLAR_STANDING || pillar.State == PILLAR_WARNING) return pillar.Home;
	const float halfWidth = HalfWidth(pillar.Home);
	const VEC3 base = GetBase(pillar, HALF_PI);
	STAGE_REGION region = pillar.Home;
	if (pillar.State == PILLAR_FALLING) {
		// 倒れている途中は、折れた根元だけが低い台として残る。
		const float half = halfWidth * STUMP_WIDTH_RATE;
		region.MinX = base.X - half;
		region.MaxX = base.X + half;
		region.MinZ = base.Z - half;
		region.MaxZ = base.Z + half;
		region.BaseY = m_BaseY + STUMP_HEIGHT;
		return region;
	}
	// 倒れ終わったら、倒れた向きに長い箱（幅と高さは柱の太さ、長さは柱の高さ）。
	const float length = pillar.Home.BaseY - m_BaseY;
	const float centerX = base.X + pillar.Direction.X * length * 0.5f;
	const float centerZ = base.Z + pillar.Direction.Z * length * 0.5f;
	region.MinX = centerX - halfWidth;
	region.MaxX = centerX + halfWidth;
	region.MinZ = centerZ - length * 0.5f;
	region.MaxZ = centerZ + length * 0.5f;
	region.BaseY = m_BaseY + halfWidth * 2.0f;
	region.Yaw = atan2f(pillar.Direction.X, pillar.Direction.Z);
	return region;
}

int PillarCalc::Step(float dt, const CAPSULE (&bodies)[FIGHTER_COUNT], const bool (&hittable)[FIGHTER_COUNT], STAGE& stage,
	PILLAR_IMPACT impacts[], int maxImpacts)
{
	if (dt <= 0.0f) return 0;
	m_Clock += dt;
	int count = 0;
	for (int index = 0; index < m_Count; ++index) {
		PILLAR& pillar = m_Pillars[index];
		const PILLAR_STATE before = pillar.State;
		float remaining = dt;
		if (pillar.State == PILLAR_WARNING) {
			const float wait = fminf(remaining, pillar.Timer);
			pillar.Timer -= wait;
			remaining -= wait;
			if (pillar.Timer <= 0.0f) pillar.State = PILLAR_FALLING;
		}
		if (pillar.State == PILLAR_FALLING && remaining > 0.0f) {
			const float from = FallAngle(pillar.Fall);
			pillar.Fall = fminf(1.0f, pillar.Fall + remaining / FALL_SECONDS);
			const float to = FallAngle(pillar.Fall);
			// 長いフレームでも見逃さないよう、柱の先が SWEEP_STEP 進むごとに通り道を調べる。
			const float height = pillar.Home.BaseY - m_BaseY;
			const float radius = HalfWidth(pillar.Home);
			const int steps = (int)fmaxf(1.0f, ceilf((to - from) * height / SWEEP_STEP));
			for (int k = 1; k <= steps; ++k) {
				const float angle = from + (to - from) * k / steps;
				const VEC3 base = GetBase(pillar, angle);
				const VEC3 axis{ pillar.Direction.X * sinf(angle), cosf(angle), pillar.Direction.Z * sinf(angle) };
				const CAPSULE shaft{ base, Vec3Add(base, Vec3Scale(axis, height)), radius };
				for (int victim = 0; victim < FIGHTER_COUNT; ++victim) {
					if (!hittable[victim] || (pillar.HitMask & (1u << victim)) || !CollisionCapsuleCapsule(shaft, bodies[victim]).Hit) continue;
					pillar.HitMask |= 1u << victim;
					if (count < maxImpacts) impacts[count++] = { index, victim, pillar.Attacker, pillar.Direction };
				}
			}
			if (pillar.Fall >= 1.0f) pillar.State = PILLAR_FALLEN;
		}
		if (pillar.State != before && pillar.Region < stage.RegionCount) stage.Regions[pillar.Region] = GetRegion(pillar);
	}
	return count;
}

PILLAR_POSE PillarCalc::GetPose(int index) const
{
	const PILLAR& pillar = m_Pillars[index];
	float angle = FallAngle(pillar.Fall);
	if (pillar.State == PILLAR_WARNING) angle += WOBBLE_ANGLE * sinf((WARNING_SECONDS - pillar.Timer) * WOBBLE_SPEED);
	const VEC3& d = pillar.Direction;
	const float s = sinf(angle), c = cosf(angle);
	PILLAR_POSE pose;
	pose.Base = GetBase(pillar, angle);
	pose.Axis = { d.X * s, c, d.Z * s };
	pose.Front = { d.X * c, -s, d.Z * c };
	pose.Side = { d.Z, 0.0f, -d.X };
	pose.Damage = 1.0f - pillar.Durability / DURABILITY;
	return pose;
}
