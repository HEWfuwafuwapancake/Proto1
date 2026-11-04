#include "physics/bladecalc.h"

// 調整用の値
static constexpr float TIME_EPSILON = 0.00001f; // 1/60 秒を何度も足したときの丸め誤差だけを吸収する。

void BladeCalc::Reset()
{
	*this = {};
}

void BladeCalc::Enter(BLADE_STATE state)
{
	m_State = state;
	m_Time = 0.0f;
}

int BladeCalc::Step(bool trigger, bool cancel, float dt, const BLADE_PARAMS& params)
{
	if (cancel) {
		Reset();
		return 0;
	}
	if (dt <= 0.0f) return 0;
	if (m_State == BLADE_IDLE) {
		if (trigger) {
			m_Slash = 1;
			m_Queued = false;
			Enter(BLADE_WINDUP);
		}
		return 0;
	}
	if (trigger && m_Slash < params.MaxSlash) {
		if (m_State == BLADE_WINDOW) {
			++m_Slash;
			Enter(BLADE_WINDUP);
			return 0;
		}
		if (m_State == BLADE_WINDUP || m_State == BLADE_SWING) m_Queued = true;
	}
	m_Time += dt;
	switch (m_State) {
	case BLADE_WINDUP:
		if (m_Time + TIME_EPSILON < params.WindupSeconds) return 0;
		Enter(BLADE_SWING);
		return m_Slash;
	case BLADE_SWING:
		if (m_Time + TIME_EPSILON < params.SwingSeconds) return 0;
		if (m_Slash >= params.MaxSlash) Enter(BLADE_COOLDOWN);
		else if (m_Queued) {
			m_Queued = false;
			++m_Slash;
			Enter(BLADE_WINDUP);
		}
		else Enter(BLADE_WINDOW);
		return 0;
	case BLADE_WINDOW:
		if (m_Time + TIME_EPSILON >= params.WindowSeconds) Enter(BLADE_COOLDOWN);
		return 0;
	case BLADE_COOLDOWN:
		if (m_Time + TIME_EPSILON >= params.CooldownSeconds) Reset();
		return 0;
	default:
		return 0;
	}
}
