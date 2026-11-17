#include "physics/spearcalc.h"
#include "physics/areacalc.h"
#include <cmath>

void SpearCalc::Reset()
{
	m_Flying = false;
	m_StuckTarget = -1;
	m_Traveled = 0.0f;
}

void SpearCalc::Throw(const VEC3& origin, const VEC3& direction)
{
	Reset();
	m_Flying = true;
	m_Tip = origin;
	m_Direction = direction;
}

SPEAR_THROW_EVENT SpearCalc::Step(float dt, const SPEAR_THROW_PARAMS& params, const TENTACLE_TARGET* targets, int count)
{
	if (!m_Flying || dt <= 0.0f) return SPEAR_THROW_NONE;
	// このフレームに穂先が通る線（届く距離を超える分は切る）の上で、最初に触れた相手に刺す。
	const float step = fminf(params.Speed * dt, params.Range - m_Traveled);
	float distance = step;
	const int hit = AreaCalcFirstOnRay(m_Tip, m_Direction, step, params.Radius, targets, count, &distance);
	if (hit >= 0) {
		m_Tip = Vec3Add(m_Tip, Vec3Scale(m_Direction, fmaxf(0.0f, distance)));
		m_Flying = false;
		m_StuckTarget = hit;
		return SPEAR_THROW_STUCK;
	}
	m_Tip = Vec3Add(m_Tip, Vec3Scale(m_Direction, step));
	m_Traveled += step;
	if (m_Traveled < params.Range) return SPEAR_THROW_NONE;
	m_Flying = false;
	return SPEAR_THROW_MISSED;
}

bool SpearCalcHasFallen(const STAGE& stage, const VEC3& feet)
{
	return feet.Y < stage.TopY || !StageCalcIsOnPlatform(stage, feet.X, feet.Z);
}
