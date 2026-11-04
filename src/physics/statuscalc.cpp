#include "physics/statuscalc.h"

void StatusCalc::Reset()
{
	*this = {};
}

void StatusCalc::Apply(STATUS_TYPE type, float seconds, float value, int attacker)
{
	if (type < 0 || type >= STATUS_COUNT || seconds <= 0.0f) return;
	if (seconds > m_Time[type]) m_Time[type] = seconds;
	if (type == STATUS_SLOW) m_SlowScale = value;
	if (type == STATUS_BURN) {
		m_BurnPerSecond = value;
		m_BurnAttacker = attacker;
	}
}

STATUS_STEP StatusCalc::Step(float dt)
{
	STATUS_STEP step{ 0.0f, -1 };
	if (dt <= 0.0f) return step;
	if (m_Time[STATUS_BURN] > 0.0f) {
		// 残り時間より長くは燃やさない（2秒なら 1秒あたりの % × 2 だけ増える）。
		const float burned = dt < m_Time[STATUS_BURN] ? dt : m_Time[STATUS_BURN];
		step.Damage = m_BurnPerSecond * burned;
		step.Attacker = m_BurnAttacker;
	}
	for (int i = 0; i < STATUS_COUNT; ++i) {
		m_Time[i] -= dt;
		if (m_Time[i] < 0.0f) m_Time[i] = 0.0f;
	}
	if (m_Time[STATUS_BURN] <= 0.0f) m_BurnAttacker = -1;
	return step;
}

bool StatusCalc::IsActive(STATUS_TYPE type) const
{
	return type >= 0 && type < STATUS_COUNT && m_Time[type] > 0.0f;
}

float StatusCalc::GetMoveScale() const
{
	if (IsActive(STATUS_STUN)) return 0.0f;
	return IsActive(STATUS_SLOW) ? m_SlowScale : 1.0f;
}
