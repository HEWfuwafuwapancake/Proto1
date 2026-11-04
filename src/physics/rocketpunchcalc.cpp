#include "physics/rocketpunchcalc.h"

void RocketPunchCalc::Reset()
{
	*this = {};
}

PROJECTILE_PARAMS RocketPunchCalc::ShotParams(const ROCKET_PARAMS& params) const
{
	return { params.Radius, 0.0f, params.Range / params.Speed, 0.0f, false };
}

bool RocketPunchCalc::Launch(const VEC3& origin, const VEC3& direction, const ROCKET_PARAMS& params)
{
	if (m_State != ROCKET_IDLE) return false;
	m_Shot.Reset();
	m_Index = m_Shot.Fire(origin, Vec3Scale(direction, params.Speed), -1, ShotParams(params));
	m_Direction = direction;
	m_Position = origin;
	m_State = ROCKET_FLYING;
	return true;
}

VEC3 RocketPunchCalc::GetFist() const
{
	return m_State == ROCKET_FLYING && m_Index >= 0 ? m_Shot.Get(m_Index).Position : m_Position;
}

int RocketPunchCalc::Step(bool recall, float dt, const VEC3& shoulder, const TENTACLE_TARGET* targets, int count,
	const STAGE& stage, const ROCKET_PARAMS& params)
{
	if (dt <= 0.0f) return -1;
	int hit = -1;
	if (m_State == ROCKET_FLYING) {
		if (recall) {
			m_Position = GetFist();
			m_Shot.Reset();
			m_State = ROCKET_RETURNING;
		}
		else {
			PROJECTILE_EVENT events[2];
			const int n = m_Shot.Step(dt, targets, count, stage, ShotParams(params), events, 2);
			m_Position = m_Shot.Get(m_Index).Position;
			for (int i = 0; i < n; ++i) {
				if (events[i].Type == PROJECTILE_HIT_FIGHTER) hit = events[i].Target;
				m_Position = events[i].Position;
				m_State = ROCKET_RETURNING;
			}
			return hit;
		}
	}
	if (m_State == ROCKET_RETURNING) {
		m_Position = ProjectileCalcMoveToward(m_Position, shoulder, params.ReturnSpeed, dt);
		const VEC3 offset = Vec3Sub(m_Position, shoulder);
		if (Vec3Dot(offset, offset) <= 0.0f) m_State = ROCKET_IDLE;
	}
	return hit;
}
