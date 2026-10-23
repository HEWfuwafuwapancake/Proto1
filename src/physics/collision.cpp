#include "physics/collision.h"


static constexpr float EPSILON = 1.0e-6f;	// これより短い長さは 0 とみなす
static const VEC3 UP = { 0.0f, 1.0f, 0.0f };	// 押し出す向きが決まらないときに使う向き（真上）


// 値を lo〜hi の範囲に収める
static float Clamp(float v, float lo, float hi)
{
	return v < lo ? lo : (v > hi ? hi : v);
}

// 2つの点が「半径の合計」より近ければ重なっている。a を b から離す向きと深さを返す
static HIT_RESULT PointsWithRadius(const VEC3& a, const VEC3& b, float radiusSum)
{
	HIT_RESULT result = {};
	const VEC3 diff = Vec3Sub(a, b);
	const float distSq = Vec3LengthSq(diff);
	if (distSq >= radiusSum * radiusSum) {
		return result;	// 離れている（ちょうど触れているときも含む）
	}

	const float dist = sqrtf(distSq);
	result.Hit = true;
	result.Depth = radiusSum - dist;
	result.Normal = dist > EPSILON ? Vec3Scale(diff, 1.0f / dist) : UP;
	return result;
}


VEC3 CollisionClosestPointOnSegment(const VEC3& p, const VEC3& a, const VEC3& b)
{
	const VEC3 ab = Vec3Sub(b, a);
	const float lengthSq = Vec3LengthSq(ab);
	if (lengthSq < EPSILON) {
		return a;	// 線分の長さが 0（点）
	}
	// a から b へ向かって、どれだけの割合のところがいちばん近いか（0〜1 に収める）
	const float t = Clamp(Vec3Dot(Vec3Sub(p, a), ab) / lengthSq, 0.0f, 1.0f);
	return Vec3Add(a, Vec3Scale(ab, t));
}

void CollisionClosestPointsOfSegments(const VEC3& a0, const VEC3& a1, const VEC3& b0, const VEC3& b1, VEC3& onA, VEC3& onB)
{
	// 線分 A = a0 + d1·s、線分 B = b0 + d2·t（s・t は 0〜1）で、いちばん近い s・t を求める
	const VEC3 d1 = Vec3Sub(a1, a0);
	const VEC3 d2 = Vec3Sub(b1, b0);
	const VEC3 r = Vec3Sub(a0, b0);
	const float a = Vec3Dot(d1, d1);	// A の長さの2乗
	const float e = Vec3Dot(d2, d2);	// B の長さの2乗
	const float f = Vec3Dot(d2, r);

	float s = 0.0f;
	float t = 0.0f;
	if (a < EPSILON && e < EPSILON) {
		// どちらも点
	}
	else if (a < EPSILON) {
		// A が点：B の上でいちばん近い点
		t = Clamp(f / e, 0.0f, 1.0f);
	}
	else {
		const float c = Vec3Dot(d1, r);
		if (e < EPSILON) {
			// B が点：A の上でいちばん近い点
			s = Clamp(-c / a, 0.0f, 1.0f);
		}
		else {
			const float b = Vec3Dot(d1, d2);
			const float denom = a * e - b * b;	// 平行なら 0
			// 平行でなければ、2本の直線のいちばん近いところを A の範囲に収める。平行なら A の端（s = 0）を使う
			s = denom > EPSILON ? Clamp((b * f - c * e) / denom, 0.0f, 1.0f) : 0.0f;
			// その点に B の上でいちばん近い t。B の範囲の外になったら、B の端に収めて s を求め直す
			t = (b * s + f) / e;
			if (t < 0.0f) {
				t = 0.0f;
				s = Clamp(-c / a, 0.0f, 1.0f);
			}
			else if (t > 1.0f) {
				t = 1.0f;
				s = Clamp((b - c) / a, 0.0f, 1.0f);
			}
		}
	}

	onA = Vec3Add(a0, Vec3Scale(d1, s));
	onB = Vec3Add(b0, Vec3Scale(d2, t));
}

HIT_RESULT CollisionSphereSphere(const SPHERE& a, const SPHERE& b)
{
	return PointsWithRadius(a.Center, b.Center, a.Radius + b.Radius);
}

HIT_RESULT CollisionSphereCapsule(const SPHERE& a, const CAPSULE& b)
{
	// カプセルは「線分の上のいちばん近い点を中心にした球」とみなせる
	const VEC3 closest = CollisionClosestPointOnSegment(a.Center, b.Start, b.End);
	return PointsWithRadius(a.Center, closest, a.Radius + b.Radius);
}

HIT_RESULT CollisionCapsuleCapsule(const CAPSULE& a, const CAPSULE& b)
{
	VEC3 onA, onB;
	CollisionClosestPointsOfSegments(a.Start, a.End, b.Start, b.End, onA, onB);
	return PointsWithRadius(onA, onB, a.Radius + b.Radius);
}

HIT_RESULT CollisionSphereBox(const SPHERE& a, const BOX& b)
{
	HIT_RESULT result = {};
	const VEC3& c = a.Center;
	const bool inside = c.X >= b.Min.X && c.X <= b.Max.X
		&& c.Y >= b.Min.Y && c.Y <= b.Max.Y
		&& c.Z >= b.Min.Z && c.Z <= b.Max.Z;

	if (!inside) {
		// 箱の中で、球の中心にいちばん近い点
		const VEC3 closest = {
			Clamp(c.X, b.Min.X, b.Max.X),
			Clamp(c.Y, b.Min.Y, b.Max.Y),
			Clamp(c.Z, b.Min.Z, b.Max.Z),
		};
		return PointsWithRadius(c, closest, a.Radius);
	}

	// 中心が箱の中：いちばん近い面の外へ押し出す（6つの面までの距離のうち、いちばん短いもの）
	const float distances[6] = {
		c.X - b.Min.X, b.Max.X - c.X,
		c.Y - b.Min.Y, b.Max.Y - c.Y,
		c.Z - b.Min.Z, b.Max.Z - c.Z,
	};
	const VEC3 normals[6] = {
		{ -1.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f },
		{ 0.0f, -1.0f, 0.0f }, { 0.0f, 1.0f, 0.0f },
		{ 0.0f, 0.0f, -1.0f }, { 0.0f, 0.0f, 1.0f },
	};
	int nearest = 0;
	for (int i = 1; i < 6; i++) {
		if (distances[i] < distances[nearest]) {
			nearest = i;
		}
	}
	result.Hit = true;
	result.Normal = normals[nearest];
	result.Depth = distances[nearest] + a.Radius;
	return result;
}


void HitRecordClear(HIT_RECORD& record)
{
	record.Count = 0;
}

bool HitRecordContains(const HIT_RECORD& record, int id)
{
	for (int i = 0; i < record.Count; i++) {
		if (record.Ids[i] == id) {
			return true;
		}
	}
	return false;
}

bool HitRecordTryAdd(HIT_RECORD& record, int id)
{
	if (HitRecordContains(record, id) || record.Count >= HIT_RECORD_MAX) {
		return false;
	}
	record.Ids[record.Count] = id;
	record.Count++;
	return true;
}
