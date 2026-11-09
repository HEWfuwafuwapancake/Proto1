#include "physics/c4calc.h"

void C4Calc::Reset()
{
	m_Thrown.Reset();
	m_Falling.Reset();
	m_PlacedCount = 0;
}

void C4Calc::Throw(const VEC3& origin, const VEC3& velocity, const PROJECTILE_PARAMS& params)
{
	m_Thrown.Fire(origin, velocity, -1, params);
}

void C4Calc::Place(const VEC3& position)
{
	// いっぱいなら、いちばん古いものを消して詰める。
	if (m_PlacedCount == C4_MAX) {
		for (int i = 1; i < C4_MAX; ++i) m_Placed[i - 1] = m_Placed[i];
		--m_PlacedCount;
	}
	m_Placed[m_PlacedCount++] = position;
}

void C4Calc::Step(float dt, const TENTACLE_TARGET* targets, int count, const STAGE& stage, const PROJECTILE_PARAMS& params)
{
	PROJECTILE_EVENT events[PROJECTILE_MAX];
	int written = m_Thrown.Step(dt, targets, count, stage, params, events, PROJECTILE_MAX);
	for (int i = 0; i < written; ++i) {
		if (events[i].Type == PROJECTILE_HIT_STAGE) Place(events[i].Position);
		else if (events[i].Type == PROJECTILE_HIT_FIGHTER) m_Falling.Fire(events[i].Position, {}, -1, params);
	}
	written = m_Falling.Step(dt, nullptr, 0, stage, params, events, PROJECTILE_MAX);
	for (int i = 0; i < written; ++i) {
		if (events[i].Type == PROJECTILE_HIT_STAGE) Place(events[i].Position);
	}
}

int C4Calc::Detonate(VEC3* positions)
{
	const int count = m_PlacedCount;
	for (int i = 0; i < count; ++i) positions[i] = m_Placed[i];
	m_PlacedCount = 0;
	return count;
}
