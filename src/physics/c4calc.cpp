#include "physics/c4calc.h"

void C4Calc::Reset()
{
	m_Thrown.Reset();
	for (bool& large : m_Large) large = false;
}

bool C4Calc::Throw(const VEC3& origin, const VEC3& velocity, const PROJECTILE_PARAMS& params, bool large)
{
	const int index = m_Thrown.Fire(origin, velocity, -1, params);
	if (index < 0) return false;
	m_Large[index] = large;
	return true;
}

int C4Calc::Step(float dt, const TENTACLE_TARGET* targets, int count, const STAGE& stage,
	const PROJECTILE_PARAMS& params, C4_IMPACT* impacts, int maxImpacts, int* expired)
{
	if (expired) *expired = 0;
	PROJECTILE_EVENT events[PROJECTILE_MAX];
	const int eventCount = m_Thrown.Step(dt, targets, count, stage, params, events, PROJECTILE_MAX);
	int written = 0;
	for (int i = 0; i < eventCount; ++i) {
		if (events[i].Type == PROJECTILE_EXPIRED) {
			if (expired) ++*expired;
			continue;
		}
		if (written >= maxImpacts) continue;
		impacts[written++] = { events[i].Position, m_Large[events[i].Index] };
	}
	return written;
}
