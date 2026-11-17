#include "physics/combocalc.h"
#include <cmath>

// 調整用の値
static constexpr float MIN_FLAT_LENGTH = 0.00001f; // 向きの水平の長さがこれより短ければ進まない。

void ComboCalc::Reset()
{
	*this = {};
}

int ComboCalc::Start(int count)
{
	const int next = (m_Window > 0.0f && m_Step >= 0 && count > 0) ? (m_Step + 1) % count : 0;
	m_Step = next;
	m_Window = 0.0f;
	return next;
}

void ComboCalc::Open(float seconds)
{
	m_Window = seconds > 0.0f ? seconds : 0.0f;
}

void ComboCalc::Step(float dt)
{
	if (dt <= 0.0f || m_Window <= 0.0f) return;
	m_Window -= dt;
	if (m_Window <= 0.0f) {
		m_Window = 0.0f;
		m_Step = -1;
	}
}

// 1つの軸で、浮島のふち（margin だけ内側）に着くまでに進める割合を求める。
static float LimitOnAxis(float start, float delta, float center, float half, float margin, float limit)
{
	if (delta > 0.0f) return fminf(limit, fmaxf(0.0f, (center + half - margin - start) / delta));
	if (delta < 0.0f) return fminf(limit, fmaxf(0.0f, (center - half + margin - start) / delta));
	return limit;
}

VEC3 ComboCalcPierceEnd(const STAGE& stage, const VEC3& start, const VEC3& direction, float distance, float margin)
{
	const float flat = sqrtf(direction.X * direction.X + direction.Z * direction.Z);
	if (flat < MIN_FLAT_LENGTH || distance <= 0.0f) return start;
	const float dx = direction.X / flat * distance;
	const float dz = direction.Z / flat * distance;
	float t = 1.0f;
	if (StageCalcIsOnPlatform(stage, start.X, start.Z)) {
		t = LimitOnAxis(start.X, dx, stage.CenterX, stage.HalfWidth, margin, t);
		t = LimitOnAxis(start.Z, dz, stage.CenterZ, stage.HalfDepth, margin, t);
	}
	return { start.X + dx * t, start.Y, start.Z + dz * t };
}
