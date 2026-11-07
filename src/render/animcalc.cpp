#include "render/animcalc.h"
#include <algorithm>
#include <cmath>

static ANIM_VEC3 Interpolate(const ANIM_VEC3& a, const ANIM_VEC3& b, float t)
{
	return { a.X + (b.X - a.X) * t, a.Y + (b.Y - a.Y) * t, a.Z + (b.Z - a.Z) * t };
}

static ANIM_QUAT Interpolate(const ANIM_QUAT& a, ANIM_QUAT b, float t)
{
	float dot = a.X * b.X + a.Y * b.Y + a.Z * b.Z + a.W * b.W;
	if (dot < 0.0f) {
		b = { -b.X, -b.Y, -b.Z, -b.W };
		dot = -dot;
	}
	float from = 1.0f - t, to = t;
	if (dot < 0.9995f) {
		const float angle = acosf(dot);
		const float sine = sinf(angle);
		from = sinf(from * angle) / sine;
		to = sinf(to * angle) / sine;
	}
	const ANIM_QUAT result{ a.X * from + b.X * to, a.Y * from + b.Y * to,
		a.Z * from + b.Z * to, a.W * from + b.W * to };
	const float length = sqrtf(result.X * result.X + result.Y * result.Y
		+ result.Z * result.Z + result.W * result.W);
	return { result.X / length, result.Y / length, result.Z / length, result.W / length };
}

template<class T>
static T Sample(const std::vector<ANIM_KEY<T>>& keys, float time, const T& fallback)
{
	if (keys.empty()) return fallback;
	const auto next = std::upper_bound(keys.begin(), keys.end(), time,
		[](float value, const ANIM_KEY<T>& key) { return value < key.Time; });
	if (next == keys.begin()) return next->Value;
	const auto& previous = *(next - 1);
	if (next == keys.end()) return previous.Value;
	return Interpolate(previous.Value, next->Value, (time - previous.Time) / (next->Time - previous.Time));
}

float AnimCalcLoopTime(float seconds, float lastFrameSeconds)
{
	return fmodf(seconds, lastFrameSeconds);
}

ANIM_TRANSFORM AnimCalcSample(const ANIM_TRACK& track, float time, const ANIM_TRANSFORM& bindPose)
{
	return { Sample(track.Positions, time, bindPose.Position),
		Sample(track.Rotations, time, bindPose.Rotation), Sample(track.Scales, time, bindPose.Scale) };
}

float AnimCalcBlendWeight(float elapsed, float duration)
{
	return fminf(fmaxf(elapsed / duration, 0.0f), 1.0f);
}

ANIM_TRANSFORM AnimCalcBlend(const ANIM_TRANSFORM& from, const ANIM_TRANSFORM& to, float weight)
{
	return { Interpolate(from.Position, to.Position, weight), Interpolate(from.Rotation, to.Rotation, weight),
		Interpolate(from.Scale, to.Scale, weight) };
}

ANIM_MATRIX AnimCalcMatrix(const ANIM_TRANSFORM& pose)
{
	const ANIM_QUAT& q = pose.Rotation;
	const float x = q.X, y = q.Y, z = q.Z, w = q.W;
	const ANIM_VEC3& s = pose.Scale;
	const ANIM_VEC3& p = pose.Position;
	return {{
		{{ s.X * (1 - 2 * (y * y + z * z)), s.X * 2 * (x * y + z * w), s.X * 2 * (x * z - y * w), 0 }},
		{{ s.Y * 2 * (x * y - z * w), s.Y * (1 - 2 * (x * x + z * z)), s.Y * 2 * (y * z + x * w), 0 }},
		{{ s.Z * 2 * (x * z + y * w), s.Z * 2 * (y * z - x * w), s.Z * (1 - 2 * (x * x + y * y)), 0 }},
		{{ p.X, p.Y, p.Z, 1 }}
	}};
}

ANIM_MATRIX AnimCalcCompose(const ANIM_MATRIX& local, const ANIM_MATRIX& parent)
{
	ANIM_MATRIX result{};
	for (int row = 0; row < 4; ++row) {
		for (int column = 0; column < 4; ++column) {
			for (int k = 0; k < 4; ++k) result[row][column] += local[row][k] * parent[k][column];
		}
	}
	return result;
}

ANIM_MATRIX AnimCalcRotateY(const ANIM_MATRIX& global, float angle)
{
	// 原点へ戻し、Y まわりに回してから元の位置へ置く（行ベクトルなので右から掛ける）。
	const float c = cosf(angle), s = sinf(angle);
	const ANIM_MATRIX rotation = {{
		{{ c, 0, -s, 0 }}, {{ 0, 1, 0, 0 }}, {{ s, 0, c, 0 }}, {{ 0, 0, 0, 1 }}
	}};
	const float x = global[3][0], y = global[3][1], z = global[3][2];
	ANIM_MATRIX moved = global;
	moved[3][0] = moved[3][1] = moved[3][2] = 0.0f;
	ANIM_MATRIX result = AnimCalcCompose(moved, rotation);
	result[3][0] = x;
	result[3][1] = y;
	result[3][2] = z;
	return result;
}
