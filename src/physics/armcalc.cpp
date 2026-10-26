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

TENTACLE_PIECE_AXES ArmCalcRollAxes(const TENTACLE_PIECE_AXES& axes, float roll)
{
	if (roll == 0.0f) return axes;
	const float c = cosf(roll), s = sinf(roll);
	return { Vec3Add(Vec3Scale(axes.X, c), Vec3Scale(axes.Z, s)), axes.Y,
		Vec3Sub(Vec3Scale(axes.Z, c), Vec3Scale(axes.X, s)) };
}
