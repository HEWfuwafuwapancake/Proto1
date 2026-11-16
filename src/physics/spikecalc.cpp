#include "physics/spikecalc.h"
#include <cmath>

float SpikeCalcLineStart(const SPIKE_LINE_PARAMS& params, int index)
{
	if (params.MaxCount <= 1) return 0.0f;
	return params.SpreadSeconds * static_cast<float>(index) / static_cast<float>(params.MaxCount - 1);
}

float SpikeCalcLineGrowth(const SPIKE_LINE_PARAMS& params, int index, float elapsed)
{
	float t = elapsed - SpikeCalcLineStart(params, index);
	if (t <= 0.0f) return 0.0f;
	if (t < params.RiseSeconds) return t / params.RiseSeconds;
	t -= params.RiseSeconds;
	if (t < params.StaySeconds) return 1.0f;
	t -= params.StaySeconds;
	if (t < params.SinkSeconds) return 1.0f - t / params.SinkSeconds;
	return 0.0f;
}

bool SpikeCalcLineFinished(const SPIKE_LINE_PARAMS& params, int count, float elapsed)
{
	if (count <= 0) return true;
	return elapsed >= SpikeCalcLineStart(params, count - 1) + params.RiseSeconds + params.StaySeconds + params.SinkSeconds;
}

VEC3 SpikeCalcLinePosition(const VEC3& feet, const VEC3& direction, const SPIKE_LINE_PARAMS& params, int index)
{
	return Vec3Add(feet, Vec3Scale(direction, params.StartDistance + params.Spacing * static_cast<float>(index)));
}

int SpikeCalcLineCount(const STAGE& stage, const VEC3& feet, const VEC3& direction, const SPIKE_LINE_PARAMS& params)
{
	const int maxCount = params.MaxCount < SPIKE_LINE_MAX ? params.MaxCount : SPIKE_LINE_MAX;
	for (int i = 0; i < maxCount; ++i) {
		const VEC3 p = SpikeCalcLinePosition(feet, direction, params, i);
		if (!StageCalcHasSupport(stage, p.X, feet.Y, p.Z)) return i;
	}
	return maxCount;
}

void ThornFieldCalc::Reset()
{
	*this = {};
}

void ThornFieldCalc::Place(const VEC3& center, float seconds)
{
	int slot = 0;
	for (int i = 0; i < THORN_FIELD_MAX; ++i) {
		if (!m_Fields[i].Active) {
			slot = i;
			break;
		}
		if (m_Fields[i].Remaining < m_Fields[slot].Remaining) slot = i;
	}
	m_Fields[slot] = { center, seconds, 0.0f, seconds > 0.0f };
}

void ThornFieldCalc::Step(float dt)
{
	if (dt <= 0.0f) return;
	for (auto& field : m_Fields) {
		if (!field.Active) continue;
		field.Age += dt;
		field.Remaining -= dt;
		if (field.Remaining <= 0.0f) field.Active = false;
	}
}

bool ThornFieldCalc::IsInside(const VEC3& feet, float radius, float halfHeight) const
{
	for (const auto& field : m_Fields) {
		if (!field.Active || fabsf(feet.Y - field.Center.Y) > halfHeight) continue;
		const float dx = feet.X - field.Center.X;
		const float dz = feet.Z - field.Center.Z;
		if (dx * dx + dz * dz <= radius * radius) return true;
	}
	return false;
}

float ThornFieldCalc::TrackMove(int id, const VEC3& feet, bool inside)
{
	if (id < 0 || id >= FIGHTER_COUNT) return 0.0f;
	float moved = 0.0f;
	if (inside && m_WasInside[id]) {
		const float dx = feet.X - m_Last[id].X;
		const float dz = feet.Z - m_Last[id].Z;
		moved = sqrtf(dx * dx + dz * dz);
	}
	m_Last[id] = feet;
	m_WasInside[id] = inside;
	return moved;
}

int ThornFieldCalc::GetCount() const
{
	int count = 0;
	for (const auto& field : m_Fields) {
		if (field.Active) ++count;
	}
	return count;
}
