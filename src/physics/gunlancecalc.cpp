#include "physics/gunlancecalc.h"

void GunlanceCalc::Reset()
{
	*this = {};
}

int GunlanceCalc::Step(bool tap, float dt, const GUNLANCE_PARAMS& params)
{
	for (float& rehit : m_Rehit) {
		rehit = rehit > dt ? rehit - dt : 0.0f;
	}
	if (tap) {
		++m_Level;
		m_Wait = 0.0f;
	}
	else if (m_Level > 0) {
		m_Wait += dt;
	}
	if (m_Level >= GUNLANCE_MAX_LEVEL || (m_Level > 0 && m_Wait >= params.WaitSeconds)) {
		const int fired = m_Level;
		m_Level = 0;
		m_Wait = 0.0f;
		return fired;
	}
	return 0;
}

bool GunlanceCalc::TryHit(int target, const GUNLANCE_PARAMS& params)
{
	if (target < 0 || target >= GUNLANCE_TARGET_MAX || m_Rehit[target] > 0.0f) return false;
	m_Rehit[target] = params.RehitSeconds;
	return true;
}
