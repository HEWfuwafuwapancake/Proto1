#include "physics/risecalc.h"
#include <math.h>

static constexpr float MIN_FORWARD = 1.0e-4f; // 向きの長さがこれより短ければ、向きなしとみなす

// 足場の上で pos にいちばん近い点（ふちから inset だけ内側）を返す。
static VEC3 NearestOnSurface(const STAGE_PLATFORM& surface, const VEC3& position, float inset)
{
	const float halfWidth = fmaxf(0.0f, surface.HalfWidth - inset);
	const float halfDepth = fmaxf(0.0f, surface.HalfDepth - inset);
	const float x = fminf(fmaxf(position.X, surface.CenterX - halfWidth), surface.CenterX + halfWidth);
	const float z = fminf(fmaxf(position.Z, surface.CenterZ - halfDepth), surface.CenterZ + halfDepth);
	return { x, surface.TopY, z };
}

bool RiseCalcNearestLanding(const STAGE& stage, const VEC3& position, float inset, float maxDistance, VEC3& landing)
{
	bool found = false;
	float best = maxDistance;
	for (int i = -1; i < stage.PlatformCount; ++i) {
		const STAGE_PLATFORM surface = i < 0 ? StageCalcIslandSurface(stage) : stage.Platforms[i];
		const VEC3 point = NearestOnSurface(surface, position, inset);
		const float distance = Vec3Length(Vec3Sub(point, position));
		if (distance > best) continue;
		best = distance;
		landing = point;
		found = true;
	}
	return found;
}

VEC3 RiseCalcSlopeOffset(const VEC3& forward, float pitch, float distance)
{
	const float length = sqrtf(forward.X * forward.X + forward.Z * forward.Z);
	if (length < MIN_FORWARD) return { 0.0f, distance, 0.0f };
	const float side = cosf(pitch) * distance / length;
	return { forward.X * side, sinf(pitch) * distance, forward.Z * side };
}

void RiseCalc::Reset()
{
	*this = RiseCalc{};
}

void RiseCalc::Start(const VEC3& from, const VEC3& offset, float seconds, bool slowDown)
{
	m_From = from;
	m_Offset = offset;
	m_Seconds = seconds;
	m_Time = 0.0f;
	m_SlowDown = slowDown;
	m_Active = true;
}

VEC3 RiseCalc::Step(float dt)
{
	if (!m_Active) return Vec3Add(m_From, m_Offset);
	m_Time += dt;
	float rate = m_Seconds > 0.0f ? fminf(1.0f, m_Time / m_Seconds) : 1.0f;
	if (rate >= 1.0f) m_Active = false;
	// だんだん遅くなる進み方：はじめが速く、終わりで速さ 0 になる。
	if (m_SlowDown) rate = 1.0f - (1.0f - rate) * (1.0f - rate);
	return Vec3Add(m_From, Vec3Scale(m_Offset, rate));
}
