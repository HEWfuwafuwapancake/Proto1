#include <math.h>
#include "input/padcalc.h"
#include "core/mathutil.h"


static constexpr unsigned char DS4_USB_REPORT_ID = 0x01;	// USB でつないだときの入力レポートの番号
static constexpr int DS4_USB_REPORT_MIN_SIZE = 8;			// ボタンまで（[0]〜[7]）読むのに要るバイト数
static constexpr int DS4_USB_OUTPUT_MIN_SIZE = 9;          // ライトバーの青まで（[0]〜[8]）


bool PadCalcBuildDs4UsbOutput(unsigned char* data, int size,
	float smallMotor, float largeMotor, float red, float green, float blue)
{
	if (data == nullptr || size < DS4_USB_OUTPUT_MIN_SIZE) return false;
	for (int i = 0; i < size; ++i) data[i] = 0;
	data[0] = 0x05;
	data[1] = 0x07;
	data[4] = (unsigned char)(Saturate(smallMotor) * 255.0f + 0.5f);
	data[5] = (unsigned char)(Saturate(largeMotor) * 255.0f + 0.5f);
	data[6] = (unsigned char)(Saturate(red) * 255.0f + 0.5f);
	data[7] = (unsigned char)(Saturate(green) * 255.0f + 0.5f);
	data[8] = (unsigned char)(Saturate(blue) * 255.0f + 0.5f);
	return true;
}

void PadCalcRumble(PAD_RUMBLE& rumble, float smallMotor, float largeMotor, float seconds)
{
	if (seconds <= 0.0f) {
		rumble = {};
		return;
	}
	if (rumble.RemainingSeconds <= 0.0f) rumble = {};
	rumble.SmallMotor = fmaxf(rumble.SmallMotor, Saturate(smallMotor));
	rumble.LargeMotor = fmaxf(rumble.LargeMotor, Saturate(largeMotor));
	rumble.RemainingSeconds = fmaxf(rumble.RemainingSeconds, seconds);
}

void PadCalcUpdateRumble(PAD_RUMBLE& rumble, float dt)
{
	rumble.RemainingSeconds = fmaxf(0.0f, rumble.RemainingSeconds - fmaxf(0.0f, dt));
	if (rumble.RemainingSeconds <= 0.0f) rumble = {};
}


float PadCalcStickValue(unsigned char raw)
{
	// 0〜255 の真ん中は 127.5（ちょうど真ん中の値はないので、手を離すと 127 か 128 が来る）
	return ((float)raw - 127.5f) / 127.5f;
}

ACTION_AXIS PadCalcApplyDeadZone(ACTION_AXIS stick, float deadZone)
{
	const float length = sqrtf(stick.X * stick.X + stick.Y * stick.Y);
	if (length <= deadZone) {
		return { 0.0f, 0.0f };
	}

	// 遊びの外側（deadZone〜1）を 0〜1 に広げ直す。向きはそのまま
	float scaled = (length - deadZone) / (1.0f - deadZone);
	if (scaled > 1.0f) {
		scaled = 1.0f;	// 斜めに倒すと長さが 1 を超えることがある
	}
	return { stick.X / length * scaled, stick.Y / length * scaled };
}

bool PadCalcParseDs4UsbReport(const unsigned char* data, int size, PAD_STATE& state)
{
	if (data == nullptr || size < DS4_USB_REPORT_MIN_SIZE || data[0] != DS4_USB_REPORT_ID) {
		return false;
	}

	// スティック（上下は、レポートでは下がプラスなので、上がプラスに直す）
	state.LeftStick = { PadCalcStickValue(data[1]), -PadCalcStickValue(data[2]) };
	state.RightStick = { PadCalcStickValue(data[3]), -PadCalcStickValue(data[4]) };

	// ボタン
	unsigned int buttons = 0;
	const unsigned char dpad = data[5] & 0x0F;
	if (dpad == 0 || dpad == 1 || dpad == 7) buttons |= PAD_BUTTON_DPAD_UP;
	if (dpad == 1 || dpad == 2 || dpad == 3) buttons |= PAD_BUTTON_DPAD_RIGHT;
	if (dpad == 3 || dpad == 4 || dpad == 5) buttons |= PAD_BUTTON_DPAD_DOWN;
	if (dpad == 5 || dpad == 6 || dpad == 7) buttons |= PAD_BUTTON_DPAD_LEFT;
	if (data[5] & 0x10) buttons |= PAD_BUTTON_SQUARE;
	if (data[5] & 0x20) buttons |= PAD_BUTTON_CROSS;
	if (data[5] & 0x40) buttons |= PAD_BUTTON_CIRCLE;
	if (data[5] & 0x80) buttons |= PAD_BUTTON_TRIANGLE;
	if (data[6] & 0x01) buttons |= PAD_BUTTON_L1;
	if (data[6] & 0x02) buttons |= PAD_BUTTON_R1;
	if (data[6] & 0x04) buttons |= PAD_BUTTON_L2;
	if (data[6] & 0x08) buttons |= PAD_BUTTON_R2;
	if (data[6] & 0x10) buttons |= PAD_BUTTON_SHARE;
	if (data[6] & 0x20) buttons |= PAD_BUTTON_OPTIONS;
	if (data[6] & 0x40) buttons |= PAD_BUTTON_L3;
	if (data[6] & 0x80) buttons |= PAD_BUTTON_R3;
	if (data[7] & 0x01) buttons |= PAD_BUTTON_PS;
	state.Buttons = buttons;

	return true;
}
