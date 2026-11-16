#include "physics/flailcalc.h"
#include <cmath>

float FlailCalcCharge(float spinSeconds, float fullSeconds)
{
	if (fullSeconds <= 0.0f) return 1.0f;
	const float charge = spinSeconds / fullSeconds;
	return charge < 0.0f ? 0.0f : (charge > 1.0f ? 1.0f : charge);
}

float FlailCalcMix(float minValue, float maxValue, float charge)
{
	const float t = charge < 0.0f ? 0.0f : (charge > 1.0f ? 1.0f : charge);
	return minValue + (maxValue - minValue) * t;
}

VEC3 FlailCalcSpinPosition(const VEC3& center, float angle, float radius)
{
	return { center.X + sinf(angle) * radius, center.Y, center.Z + cosf(angle) * radius };
}

void FlailCalc::Reset()
{
	*this = {};
}

void FlailCalc::Throw(const VEC3& origin, const VEC3& direction, float range)
{
	m_State = FLAIL_OUT;
	m_Position = origin;
	m_Previous = origin;
	m_MoveDirection = direction;
	m_Remaining = range;
}

FLAIL_STATE FlailCalc::Step(float dt, const VEC3& hand, const FLAIL_THROW_PARAMS& params)
{
	m_Previous = m_Position;
	if (dt <= 0.0f || m_State == FLAIL_IDLE) return m_State;
	if (m_State == FLAIL_OUT) {
		const float move = params.OutSpeed * dt;
		if (move >= m_Remaining) {
			// 決めた距離で止めて、次から戻る。
			m_Position = Vec3Add(m_Position, Vec3Scale(m_MoveDirection, m_Remaining));
			m_Remaining = 0.0f;
			m_State = FLAIL_BACK;
		}
		else {
			m_Position = Vec3Add(m_Position, Vec3Scale(m_MoveDirection, move));
			m_Remaining -= move;
		}
		return m_State;
	}
	// 帰り：動いている手元を追いかける。
	const VEC3 toHand = Vec3Sub(hand, m_Position);
	const float distance = Vec3Length(toHand);
	const float move = params.BackSpeed * dt;
	if (distance > 0.00001f) m_MoveDirection = Vec3Scale(toHand, 1.0f / distance);
	if (distance <= params.CatchDistance + move) {
		m_Position = hand;
		m_State = FLAIL_IDLE;
	}
	else m_Position = Vec3Add(m_Position, Vec3Scale(m_MoveDirection, move));
	return m_State;
}
