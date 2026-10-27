#include "physics/armcalc.h"

int ArmCalcCycle(int current, int direction, const bool* available, int count)
{
	if (available == nullptr || count <= 0 || current < 0 || current >= count
		|| (direction != -1 && direction != 1)) return -1;
	int next = current;
	for (int i = 0; i < count; ++i) {
		next += direction;
		if (next >= count) next = 0;
		if (next < 0) next = count - 1;
		if (available[next]) return next;
	}
	return -1;
}

int ArmCalcSelect(int number, const bool* available, int count)
{
	if (available == nullptr || number < 0 || number >= count) return -1;
	return available[number] ? number : -1;
}

VEC3 ArmCalcShoulder(const VEC3& position, float yaw, int hand, float sideOffset, float height, float forwardOffset)
{
	const float side = (hand == 0 ? -1.0f : 1.0f) * sideOffset;
	const float c = cosf(yaw), s = sinf(yaw);
	return { position.X + c * side + s * forwardOffset, position.Y + height,
		position.Z - s * side + c * forwardOffset };
}

VEC3 ArmCalcRestDirection(float yaw, int hand, float pitch, float spread)
{
	const float angle = yaw + (hand == 0 ? -spread : spread);
	return ArmCalcPitchDirection({ sinf(angle), 0.0f, cosf(angle) }, pitch);
}

VEC3 ArmCalcPitchDirection(const VEC3& forward, float pitch)
{
	const VEC3 horizontal{ forward.X, 0.0f, forward.Z };
	const float length = Vec3Length(horizontal);
	const VEC3 direction = length > 0.00001f ? Vec3Scale(horizontal, 1.0f / length) : VEC3{ 0, 0, 1 };
	const float c = cosf(pitch);
	return { direction.X * c, sinf(pitch), direction.Z * c };
}

VEC3 ArmCalcBlendDirection(const VEC3& a, const VEC3& b, float ratio)
{
	const float t = fmaxf(0.0f, fminf(1.0f, ratio));
	const VEC3 mixed = Vec3Add(Vec3Scale(a, 1.0f - t), Vec3Scale(b, t));
	const float length = Vec3Length(mixed);
	if (length > 0.00001f) return Vec3Scale(mixed, 1.0f / length);
	const float bLength = Vec3Length(b);
	return bLength > 0.00001f ? Vec3Scale(b, 1.0f / bLength) : VEC3{ 0, 0, 1 };
}

TENTACLE_PIECE_AXES ArmCalcRollAxes(const TENTACLE_PIECE_AXES& axes, float roll)
{
	if (roll == 0.0f) return axes;
	const float c = cosf(roll), s = sinf(roll);
	return { Vec3Add(Vec3Scale(axes.X, c), Vec3Scale(axes.Z, s)), axes.Y,
		Vec3Sub(Vec3Scale(axes.Z, c), Vec3Scale(axes.X, s)) };
}
