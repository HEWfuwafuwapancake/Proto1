#include <math.h>
#include "input/actioncalc.h"


// 向きの長さの2乗（比べるだけなら平方根はいらない）
static float LengthSq(ACTION_AXIS axis)
{
	return axis.X * axis.X + axis.Y * axis.Y;
}

ACTION_AXIS ActionCalcMoveFromKeys(bool forward, bool back, bool left, bool right)
{
	ACTION_AXIS axis = { 0.0f, 0.0f };
	if (forward) axis.Y += 1.0f;
	if (back)    axis.Y -= 1.0f;
	if (right)   axis.X += 1.0f;
	if (left)    axis.X -= 1.0f;

	// 斜めは長さが √2 になるので 1 に直す
	return ActionCalcClampLength(axis);
}

ACTION_AXIS ActionCalcClampLength(ACTION_AXIS axis)
{
	const float lengthSq = LengthSq(axis);
	if (lengthSq > 1.0f) {
		const float length = sqrtf(lengthSq);
		axis.X /= length;
		axis.Y /= length;
	}
	return axis;
}

ACTION_AXIS ActionCalcCombineAxis(ACTION_AXIS a, ACTION_AXIS b)
{
	return LengthSq(b) > LengthSq(a) ? b : a;
}

ACTION_EQUIP_KEY ActionCalcEquipKey(int& mode, bool leftMode, bool rightMode, int number, bool shift)
{
	if (leftMode) mode = mode == 0 ? -1 : 0;
	if (rightMode) mode = mode == 1 ? -1 : 1;
	if (number < 0) return { -1, -1 };
	const int hand = mode >= 0 ? mode : (shift ? 1 : 0);
	mode = -1;
	return { hand, number };
}
