#include "physics/parrycalc.h"

// 調整用の値
static constexpr double PARRY_SECONDS = 0.2f; // 構え始めにパリィを受け付ける秒数
static constexpr double PARRY_REARM_SECONDS = 0.5f; // 解いてから再びパリィを受け付けるまでの秒数
static constexpr double TIME_EPSILON = 0.00001; // 1/60 秒刻みの丸め誤差を吸収する

ParryCalc::ParryCalc()
{
	Reset();
}

void ParryCalc::Reset()
{
	m_Guarding = false;
	m_Eligible = false;
	m_GuardSeconds = 0.0;
	m_ReleasedSeconds = PARRY_REARM_SECONDS;
}

void ParryCalc::Step(bool guarding, float dt)
{
	if (guarding) {
		if (!m_Guarding) {
			m_Eligible = m_ReleasedSeconds + TIME_EPSILON >= PARRY_REARM_SECONDS;
			m_GuardSeconds = 0.0;
		}
		m_GuardSeconds += dt;
	}
	else {
		if (m_Guarding) m_ReleasedSeconds = 0.0;
		m_ReleasedSeconds += dt;
		m_GuardSeconds = 0.0;
		m_Eligible = false;
	}
	m_Guarding = guarding;
}

bool ParryCalc::IsParryWindow() const
{
	return m_Guarding && m_Eligible && m_GuardSeconds <= PARRY_SECONDS + TIME_EPSILON;
}

PARRY_RESULT ParryCalc::OnHit(bool blocked) const
{
	if (!blocked || !m_Guarding) return PARRY_NONE;
	return IsParryWindow() ? PARRY_SUCCESS : PARRY_BREAK_ARM;
}
