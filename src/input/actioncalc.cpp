#include <math.h>
#include "input/actioncalc.h"

// 調整用の値（送る側と受け取る側で共通）
static constexpr float MOVE_INPUT_SCALE = 32767.0f;
static constexpr float LOOK_INPUT_SCALE = 4096.0f;
static constexpr float INPUT_INTEGER_LIMIT = 32767.0f;

static int16_t Quantize(float value, float scale)
{
	return static_cast<int16_t>(roundf(fmaxf(-INPUT_INTEGER_LIMIT, fminf(INPUT_INTEGER_LIMIT, value * scale))));
}

PLAYER_INPUT ActionCalcPlayerInput(ACTION_AXIS move, ACTION_AXIS look, uint16_t buttons)
{
	move = ActionCalcClampLength(move);
	PLAYER_INPUT input;
	input.MoveX = Quantize(move.X, MOVE_INPUT_SCALE);
	input.MoveY = Quantize(move.Y, MOVE_INPUT_SCALE);
	input.LookX = Quantize(look.X, LOOK_INPUT_SCALE);
	input.LookY = Quantize(look.Y, LOOK_INPUT_SCALE);
	input.Buttons = buttons;
	return input;
}

ACTION_AXIS ActionCalcPlayerMove(const PLAYER_INPUT& input)
{
	return { input.MoveX / MOVE_INPUT_SCALE, input.MoveY / MOVE_INPUT_SCALE };
}

ACTION_AXIS ActionCalcPlayerLook(const PLAYER_INPUT& input)
{
	return { input.LookX / LOOK_INPUT_SCALE, input.LookY / LOOK_INPUT_SCALE };
}

bool ActionCalcPlayerPress(const PLAYER_INPUT& input, ACTION action)
{
	if (action < 0 || action >= ACTION_COUNT) return false;
	const uint16_t bit = static_cast<uint16_t>(1u << action);
	return (input.Buttons & bit) != 0;
}

bool ActionCalcPlayerTrigger(const PLAYER_INPUT& current, const PLAYER_INPUT& previous, ACTION action)
{
	const bool event = action == ACTION_TARGET_LEFT || action == ACTION_TARGET_RIGHT;
	return ActionCalcPlayerPress(current, action) && (event || !ActionCalcPlayerPress(previous, action));
}


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
	for (int hand = 0; hand < 2; ++hand) {
		if (!(hand == 0 ? leftMode : rightMode)) continue;
		// 同じ手のモード中なら次の段へ、最後の段の次で抜ける。別の手やモードなしからは1段目。
		const int page = ActionCalcEquipModeHand(mode) == hand ? ActionCalcEquipModePage(mode) + 1 : 0;
		mode = page < ACTION_EQUIP_PAGE_COUNT ? page * 2 + hand : -1;
	}
	if (number < 0) return { -1, -1 };
	const int hand = mode >= 0 ? ActionCalcEquipModeHand(mode) : (shift ? 1 : 0);
	const int page = mode >= 0 ? ActionCalcEquipModePage(mode) : 0;
	mode = -1;
	return { hand, page * ACTION_EQUIP_PAGE_SIZE + number };
}
