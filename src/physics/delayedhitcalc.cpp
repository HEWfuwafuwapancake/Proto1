#include "physics/delayedhitcalc.h"

void DelayedHitCalc::Reset()
{
	m_Count = 0;
}

bool DelayedHitCalc::Add(const DELAYED_HIT& hit)
{
	if (m_Count >= DELAYED_HIT_MAX) return false;
	m_Hits[m_Count++] = hit;
	return true;
}

int DelayedHitCalc::Step(float dt, DELAYED_HIT* out, int maxCount)
{
	int written = 0;
	int kept = 0;
	for (int i = 0; i < m_Count; ++i) {
		DELAYED_HIT hit = m_Hits[i];
		hit.Seconds -= dt;
		if (hit.Seconds <= 0.0f && written < maxCount) out[written++] = hit;
		else m_Hits[kept++] = hit;
	}
	m_Count = kept;
	return written;
}
