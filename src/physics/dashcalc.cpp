#include "physics/dashcalc.h"
#include <math.h>

static constexpr float MIN_DIRECTION = 1.0e-4f; // 向きの長さがこれより短ければ、向きなしとみなす

void DashCalc::Reset()
{
	*this = DashCalc{};
}

bool DashCalc::TryStart(const VEC3& direction, bool onGround, const DASH_PARAMS& params, bool sustain)
{
	const float length = sqrtf(direction.X * direction.X + direction.Z * direction.Z);
	if (length < MIN_DIRECTION || m_Cooldown > 0.0f || (!onGround && m_AirUsed)) return false;
	m_Direction = { direction.X / length, 0.0f, direction.Z / length };
	m_Time = sustain ? params.Seconds : 0.0f;
	// 休みは、進み終わってから数える。
	m_Cooldown = m_Time + params.Cooldown;
	if (!onGround) m_AirUsed = true;
	return true;
}

VEC3 DashCalc::Step(float dt, bool onGround, const DASH_PARAMS& params)
{
	if (onGround && !IsDashing()) m_AirUsed = false;
	const bool dashing = IsDashing();
	m_Time = fmaxf(0.0f, m_Time - dt);
	m_Cooldown = fmaxf(0.0f, m_Cooldown - dt);
	return dashing ? Vec3Scale(m_Direction, params.Speed) : VEC3{};
}
