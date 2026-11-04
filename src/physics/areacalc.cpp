#include "physics/areacalc.h"
#include <math.h>

// 調整用の値
static constexpr float MIN_LENGTH = 0.0001f; // これより短い向きは「向きなし」とみなす。

bool AreaCalcInSector(const VEC3& origin, const VEC3& forward, float minRadius, float radius,
	float halfAngle, float halfHeight, const CAPSULE& body)
{
	const VEC3 nearest = CollisionClosestPointOnSegment(origin, body.Start, body.End);
	const VEC3 offset = Vec3Sub(nearest, origin);
	if (fabsf(offset.Y) > halfHeight + body.Radius) return false;
	const VEC3 flat{ offset.X, 0.0f, offset.Z };
	const float distance = Vec3Length(flat);
	if (distance > radius + body.Radius || distance + body.Radius < minRadius) return false;
	if (distance < MIN_LENGTH) return true;
	const VEC3 axis = TentacleCalcAim({}, {}, false, forward);
	const float cosine = Vec3Dot(axis, Vec3Scale(flat, 1.0f / distance));
	const float angle = acosf(cosine < -1.0f ? -1.0f : (cosine > 1.0f ? 1.0f : cosine));
	// 体の半径の分だけ角度も広げる（近い相手ほど広く見える）。
	const float extra = body.Radius < distance ? asinf(body.Radius / distance) : 3.14159265f;
	return angle <= halfAngle + extra;
}

VEC3 AreaCalcLaunchDirection(const VEC3& direction, float up)
{
	VEC3 flat{ direction.X, 0.0f, direction.Z };
	const float length = Vec3Length(flat);
	flat = length < MIN_LENGTH ? VEC3{} : Vec3Scale(flat, 1.0f / length);
	const VEC3 result = Vec3Add(flat, { 0.0f, up, 0.0f });
	const float total = Vec3Length(result);
	return total < MIN_LENGTH ? VEC3{ 0.0f, 1.0f, 0.0f } : Vec3Scale(result, 1.0f / total);
}

VEC3 AreaCalcRadialDirection(const VEC3& center, const CAPSULE& body, float up, const VEC3& fallback)
{
	const VEC3 bodyCenter = Vec3Scale(Vec3Add(body.Start, body.End), 0.5f);
	const VEC3 flat{ bodyCenter.X - center.X, 0.0f, bodyCenter.Z - center.Z };
	return AreaCalcLaunchDirection(Vec3Length(flat) < MIN_LENGTH ? fallback : flat, up);
}

VEC3 AreaCalcRotateYaw(const VEC3& direction, float angle)
{
	// yaw 0 が +Z、yaw が増えると +X へ回る（sin(yaw), cos(yaw)）向きと同じ。
	const float c = cosf(angle), s = sinf(angle);
	return { direction.X * c + direction.Z * s, direction.Y, direction.Z * c - direction.X * s };
}

int AreaCalcFirstOnRay(const VEC3& origin, const VEC3& direction, float length, float radius,
	const TENTACLE_TARGET* targets, int count, float* distance)
{
	const CAPSULE ray{ origin, Vec3Add(origin, Vec3Scale(direction, length)), radius };
	int best = -1;
	float bestDistance = 0.0f;
	for (int i = 0; i < count; ++i) {
		if (!targets[i].Available || !CollisionCapsuleCapsule(ray, targets[i].Body).Hit) continue;
		VEC3 onRay, onBody;
		CollisionClosestPointsOfSegments(ray.Start, ray.End, targets[i].Body.Start, targets[i].Body.End, onRay, onBody);
		const float d = Vec3Length(Vec3Sub(onRay, origin));
		if (best < 0 || d < bestDistance) {
			best = i;
			bestDistance = d;
		}
	}
	if (best >= 0 && distance != nullptr) *distance = bestDistance;
	return best;
}

int AreaCalcCountRays(const VEC3& origin, const VEC3& direction, float length, float radius,
	int rayCount, float spreadAngle, const CAPSULE& body)
{
	int hits = 0;
	for (int i = 0; i < rayCount; ++i) {
		const float t = rayCount > 1 ? static_cast<float>(i) / (rayCount - 1) * 2.0f - 1.0f : 0.0f;
		const VEC3 ray = AreaCalcRotateYaw(direction, spreadAngle * t);
		const CAPSULE capsule{ origin, Vec3Add(origin, Vec3Scale(ray, length)), radius };
		if (CollisionCapsuleCapsule(capsule, body).Hit) ++hits;
	}
	return hits;
}
