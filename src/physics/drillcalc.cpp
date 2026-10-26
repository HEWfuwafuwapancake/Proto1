#include "physics/drillcalc.h"

// 調整用の値
static constexpr float TICK_TIME_EPSILON = 0.000001f; // 0.1 秒の足し算の丸め誤差だけを吸収する。

static void Cooldown(DRILL& drill)
{
	drill.State = DRILL_COOLDOWN;
	drill.Time = 0.0f;
	drill.Target = -1;
}

void DrillCalcReset(DRILL& drill)
{
	drill = {};
	drill.State = DRILL_IDLE;
	drill.Direction = { 0, 0, 1 };
	drill.Target = -1;
}

CAPSULE DrillCalcBit(const VEC3& origin, const VEC3& direction, const DRILL_PARAMS& params)
{
	const VEC3 horizontal = TentacleCalcAim({}, {}, false, direction);
	return { Vec3Add(origin, Vec3Scale(horizontal, params.BitStart)),
		Vec3Add(origin, Vec3Scale(horizontal, params.Reach)), params.Radius };
}

DRILL_EVENT DrillCalcStep(DRILL& drill, const VEC3& origin, const VEC3& aim,
	bool trigger, bool cancel, float dt, const DRILL_PARAMS& params,
	const TENTACLE_TARGET* targets, int count)
{
	DRILL_EVENT event{ -1, 0, false };
	if (cancel) {
		if (drill.State != DRILL_IDLE) Cooldown(drill);
		return event;
	}
	if (dt <= 0.0f) return event;
	if (drill.State == DRILL_IDLE) {
		if (!trigger) return event;
		drill.State = DRILL_STRIKING;
		drill.Direction = TentacleCalcAim({}, {}, false, aim);
		drill.Time = 0.0f;
		drill.Target = -1;
		drill.Ticks = 0;
	}
	const CAPSULE bit = DrillCalcBit(origin, drill.Direction, params);
	if (drill.State == DRILL_STRIKING) {
		int nearest = -1;
		float nearestDistance = 0.0f;
		for (int i = 0; targets != nullptr && i < count; ++i) {
			if (!targets[i].Available || !CollisionCapsuleCapsule(bit, targets[i].Body).Hit) continue;
			const VEC3 center = Vec3Scale(Vec3Add(targets[i].Body.Start, targets[i].Body.End), 0.5f);
			const float distance = Vec3LengthSq(Vec3Sub(center, origin));
			if (nearest < 0 || distance < nearestDistance) {
				nearest = i;
				nearestDistance = distance;
			}
		}
		if (nearest >= 0) {
			drill.State = DRILL_BORING;
			drill.Target = nearest;
			drill.Time = 0.0f;
		}
		else {
			drill.Time += dt;
			if (drill.Time + TICK_TIME_EPSILON >= params.StrikeSeconds) Cooldown(drill);
		}
	}
	else if (drill.State == DRILL_BORING) {
		// 先に有効性を確認する。離れたフレームに加算や押し出しを残さない。
		const int target = drill.Target;
		if (targets == nullptr || target < 0 || target >= count || !targets[target].Available
			|| !CollisionCapsuleCapsule(bit, targets[target].Body).Hit) {
			Cooldown(drill);
			return event;
		}
		drill.Time = fminf(drill.Time + dt, params.BoreSeconds);
		event.Target = target;
		while (drill.Ticks < params.TickCount
			&& drill.Time + TICK_TIME_EPSILON >= params.TickSeconds * (drill.Ticks + 1)) {
			++drill.Ticks;
			++event.DamageTicks;
		}
		if (drill.Ticks >= params.TickCount) {
			event.Push = true;
			Cooldown(drill);
		}
		else if (drill.Time + TICK_TIME_EPSILON >= params.BoreSeconds) {
			Cooldown(drill);
		}
	}
	else if (drill.State == DRILL_COOLDOWN) {
		drill.Time += dt;
		if (drill.Time + TICK_TIME_EPSILON >= params.CooldownSeconds) DrillCalcReset(drill);
	}
	return event;
}

VEC3 DrillCalcPushDirection(const VEC3& direction, float up)
{
	VEC3 push = TentacleCalcAim({}, {}, false, direction);
	push.Y = up;
	return Vec3Scale(push, 1.0f / Vec3Length(push));
}
