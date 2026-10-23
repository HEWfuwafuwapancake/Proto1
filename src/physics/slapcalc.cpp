#include "physics/slapcalc.h"
#include <algorithm>

void SlapCalcReset(SLAP& slap)
{
	slap = {};
}

bool SlapCalcCanStart(const SLAP& slap)
{
	return !slap.Active && slap.Wait <= 0.0f;
}

bool SlapCalcStart(SLAP& slap)
{
	if (!SlapCalcCanStart(slap)) return false;
	slap.Active = true;
	slap.Time = 0.0f;
	HitRecordClear(slap.Hits);
	return true;
}

void SlapCalcStep(SLAP& slap, float dt, const SLAP_PARAMS& params)
{
	if (dt <= 0.0f) return;
	if (slap.Active) {
		const float remaining = params.Duration - slap.Time;
		if (dt < remaining - 0.000001f) {
			slap.Time += dt;
			return;
		}
		slap.Time = params.Duration;
		slap.Active = false;
		slap.Wait = params.Cooldown;
		dt = (std::max)(0.0f, dt - remaining);
	}
	slap.Wait = (std::max)(0.0f, slap.Wait - dt);
	if (slap.Wait < 0.000001f) slap.Wait = 0.0f;
}

VEC3 SlapCalcDirection(const SLAP& slap, const VEC3& forward, float side, const SLAP_PARAMS& params)
{
	VEC3 horizontal{ forward.X, 0.0f, forward.Z };
	const float length = Vec3Length(horizontal);
	horizontal = length > 0.00001f ? Vec3Scale(horizontal, 1.0f / length) : VEC3{ 0, 0, 1 };
	const float progress = (std::min)(1.0f, (std::max)(0.0f, slap.Time / params.Duration));
	const float angle = side * params.Angle * (1.0f - 2.0f * progress);
	return { horizontal.X * cosf(angle) + horizontal.Z * sinf(angle), 0.0f,
		-horizontal.X * sinf(angle) + horizontal.Z * cosf(angle) };
}

int SlapCalcHit(SLAP& slap, const CAPSULE& reach, const TENTACLE_TARGET* targets, int count)
{
	if (!slap.Active) return -1;
	for (int i = 0; i < count; ++i) {
		if (targets[i].Available && !HitRecordContains(slap.Hits, i)
			&& CollisionCapsuleCapsule(reach, targets[i].Body).Hit
			&& HitRecordTryAdd(slap.Hits, i)) return i;
	}
	return -1;
}
