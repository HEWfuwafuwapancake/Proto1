#include "physics/tentaclecalc.h"
#include <algorithm>

// 調整用の値
static constexpr float AIM_MIN_LENGTH = 0.00001f; // 水平の向きを決められる最小の長さ

static VEC3 MoveTowards(const VEC3& from, const VEC3& to, float distance)
{
	const VEC3 delta = Vec3Sub(to, from);
	const float length = Vec3Length(delta);
	return length <= distance || length < 0.00001f ? to : Vec3Add(from, Vec3Scale(delta, distance / length));
}

void TentacleCalcReset(TENTACLE& arm)
{
	arm = {};
	arm.Target = -1;
}

VEC3 TentacleCalcAim(const VEC3& origin, const VEC3& target, bool hasTarget, const VEC3& fallback)
{
	VEC3 direction = hasTarget ? VEC3{ target.X - origin.X, 0.0f, target.Z - origin.Z }
		: VEC3{ fallback.X, 0.0f, fallback.Z };
	float length = Vec3Length(direction);
	if (length < AIM_MIN_LENGTH) {
		direction = { fallback.X, 0.0f, fallback.Z };
		length = Vec3Length(direction);
	}
	return length < AIM_MIN_LENGTH ? VEC3{ 0.0f, 0.0f, 1.0f } : Vec3Scale(direction, 1.0f / length);
}

VEC3 TentacleCalcClip(const VEC3& from, const VEC3& to, const STAGE& stage, float radius, float halfHeight)
{
	// 物の大きさだけ箱を広げ、中心の線分が最初に箱へ入る位置で止める。
	const float lo[3] = { stage.CenterX - stage.HalfWidth - radius, stage.TopY - stage.Thickness - halfHeight,
		stage.CenterZ - stage.HalfDepth - radius };
	const float hi[3] = { stage.CenterX + stage.HalfWidth + radius, stage.TopY + halfHeight,
		stage.CenterZ + stage.HalfDepth + radius };
	const float a[3] = { from.X, from.Y, from.Z };
	const VEC3 delta = Vec3Sub(to, from);
	const float d[3] = { delta.X, delta.Y, delta.Z };
	float enter = 0.0f, leave = 1.0f;
	for (int axis = 0; axis < 3; ++axis) {
		if (fabsf(d[axis]) < 0.000001f) {
			if (a[axis] < lo[axis] || a[axis] > hi[axis]) return to;
		}
		else {
			float nearTime = (lo[axis] - a[axis]) / d[axis];
			float farTime = (hi[axis] - a[axis]) / d[axis];
			if (nearTime > farTime) std::swap(nearTime, farTime);
			enter = (std::max)(enter, nearTime);
			leave = (std::min)(leave, farTime);
			if (enter > leave) return to;
		}
	}
	// 境界から外向きに離れる移動は妨げない。
	if (leave <= 0.0f) return to;
	const float length = Vec3Length(delta);
	const float t = (std::max)(0.0f, enter - (length > 0.0f ? 0.001f / length : 0.0f));
	return Vec3Add(from, Vec3Scale(delta, t));
}

TENTACLE_EVENT TentacleCalcStep(TENTACLE& arm, const VEC3& origin, const VEC3& aim,
	const VEC3& holdPosition, bool trigger, bool cancel, float dt, const TENTACLE_PARAMS& params,
	const TENTACLE_TARGET* targets, int count, const STAGE& stage)
{
	TENTACLE_EVENT event{ -1, -1, false };
	if (cancel) {
		event.Released = arm.Target;
		TentacleCalcReset(arm);
		arm.Tip = origin;
		return event;
	}
	if (dt <= 0.0f) return event;
	if (arm.State == TENTACLE_IDLE) {
		arm.Tip = origin;
		if (!trigger) return event;
		const float length = Vec3Length(aim);
		if (length < 0.00001f) return event;
		arm.Direction = Vec3Scale(aim, 1.0f / length);
		arm.Length = 0.0f;
		HitRecordClear(arm.Hits);
		arm.State = TENTACLE_EXTENDING;
	}
	else if (arm.State == TENTACLE_HOLDING) {
		arm.HoldTime += dt;
		if (trigger || arm.HoldTime >= params.HoldSeconds) {
			event.Released = arm.Target;
			event.Throw = trigger;
			arm.Target = -1;
			arm.State = TENTACLE_RETURNING;
			return event;
		}
		const CAPSULE& body = targets[arm.Target].Body;
		const float halfHeight = Vec3Length(Vec3Sub(body.End, body.Start)) * 0.5f + body.Radius;
		arm.Tip = TentacleCalcClip(arm.Tip, MoveTowards(arm.Tip, holdPosition, params.ReturnSpeed * dt),
			stage, body.Radius, halfHeight);
		return event;
	}
	if (arm.State == TENTACLE_RETURNING) {
		arm.Tip = MoveTowards(arm.Tip, origin, params.ReturnSpeed * dt);
		if (Vec3LengthSq(Vec3Sub(arm.Tip, origin)) < 0.000001f) TentacleCalcReset(arm);
		return event;
	}

	arm.Length = (std::min)(params.MaxLength, arm.Length + params.ExtendSpeed * dt);
	const VEC3 desired = Vec3Add(origin, Vec3Scale(arm.Direction, arm.Length));
	const VEC3 end = TentacleCalcClip(origin, desired, stage, params.Radius, params.Radius);
	// 腕全体を調べるので、一度に大きく伸びても敵を飛び越えない。
	const CAPSULE reach{ origin, end, params.Radius };
	float firstTime = 2.0f;
	int first = -1;
	for (int i = 0; i < count; ++i) {
		if (!targets[i].Available || HitRecordContains(arm.Hits, i)
			|| !CollisionCapsuleCapsule(reach, targets[i].Body).Hit) continue;
		// 最初の接触位置を二分探索し、配列の順番ではなく手前を選ぶ。
		float low = 0.0f, high = 1.0f;
		for (int step = 0; step < 20; ++step) {
			const float mid = (low + high) * 0.5f;
			const CAPSULE partial{ origin, Vec3Add(origin, Vec3Scale(Vec3Sub(end, origin), mid)), params.Radius };
			if (CollisionCapsuleCapsule(partial, targets[i].Body).Hit) high = mid;
			else low = mid;
		}
		if (high < firstTime) { firstTime = high; first = i; }
	}
	arm.Tip = end;
	if (first >= 0 && HitRecordTryAdd(arm.Hits, first)) {
		arm.Target = first;
		arm.HoldTime = 0.0f;
		arm.State = TENTACLE_HOLDING;
		arm.Tip = Vec3Scale(Vec3Add(targets[first].Body.Start, targets[first].Body.End), 0.5f);
		event.Grabbed = first;
	}
	else if (arm.Length >= params.MaxLength || Vec3LengthSq(Vec3Sub(end, desired)) > 0.000001f) {
		arm.State = TENTACLE_RETURNING;
	}
	return event;
}
