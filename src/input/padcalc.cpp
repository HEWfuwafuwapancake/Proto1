#include <math.h>
#include "input/padcalc.h"
#include "core/mathutil.h"

// 調整用の値
static constexpr unsigned char PAD_TRIGGER_THRESHOLD = 30; // これを超えるとトリガーを押した扱い
static constexpr float SWITCH_STICK_RANGE = 1600.0f; // フル入力の中心から端までの幅

static constexpr unsigned char DS4_USB_REPORT_ID = 0x01;	// USB でつないだときの入力レポートの番号
static constexpr int DS4_USB_REPORT_MIN_SIZE = 8;			// ボタンまで（[0]〜[7]）読むのに要るバイト数
static constexpr int DS4_USB_OUTPUT_MIN_SIZE = 9;          // ライトバーの青まで（[0]〜[8]）
static constexpr int DUALSENSE_USB_REPORT_SIZE = 64;       // Bluetooth の簡易レポート（番号は同じ0x01）と区別する
static constexpr int DUALSENSE_USB_OUTPUT_MIN_SIZE = 48;   // ライトバーの青まで（[0]〜[47]）

PAD_MODEL PadCalcIdentifyModel(unsigned int vendorId, unsigned int productId)
{
	if (vendorId == 0x057E && productId == 0x2009) return PAD_MODEL_SWITCH_PRO;
	if (vendorId != 0x054C) return PAD_MODEL_NONE;
	if (productId == 0x05C4 || productId == 0x09CC) return PAD_MODEL_DS4;
	if (productId == 0x0CE6) return PAD_MODEL_DUALSENSE;
	return PAD_MODEL_NONE;
}


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

bool PadCalcBuildDualSenseUsbOutput(unsigned char* data, int size,
	float smallMotor, float largeMotor, float red, float green, float blue)
{
	if (data == nullptr || size < DUALSENSE_USB_OUTPUT_MIN_SIZE) return false;
	for (int i = 0; i < size; ++i) data[i] = 0;
	data[0] = 0x02;
	data[1] = 0x03; // 互換振動を有効にし、音声のハプティクスから切り替える（停止時も0を送る）
	data[2] = 0x04; // ライトバーの色を更新する
	data[3] = (unsigned char)(Saturate(smallMotor) * 255.0f + 0.5f);
	data[4] = (unsigned char)(Saturate(largeMotor) * 255.0f + 0.5f);
	data[45] = (unsigned char)(Saturate(red) * 255.0f + 0.5f);
	data[46] = (unsigned char)(Saturate(green) * 255.0f + 0.5f);
	data[47] = (unsigned char)(Saturate(blue) * 255.0f + 0.5f);
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

// 機種によってボタンの開始位置だけが違う。スティックと各ボタンのビットは共通。
static void ReadUsbState(const unsigned char* data, int buttonOffset, PAD_STATE& state)
{
	// スティック（上下は、レポートでは下がプラスなので、上がプラスに直す）
	state.LeftStick = { PadCalcStickValue(data[1]), -PadCalcStickValue(data[2]) };
	state.RightStick = { PadCalcStickValue(data[3]), -PadCalcStickValue(data[4]) };

	// ボタン
	const unsigned char* buttonData = data + buttonOffset;
	unsigned int buttons = 0;
	const unsigned char dpad = buttonData[0] & 0x0F;
	if (dpad == 0 || dpad == 1 || dpad == 7) buttons |= PAD_BUTTON_DPAD_UP;
	if (dpad == 1 || dpad == 2 || dpad == 3) buttons |= PAD_BUTTON_DPAD_RIGHT;
	if (dpad == 3 || dpad == 4 || dpad == 5) buttons |= PAD_BUTTON_DPAD_DOWN;
	if (dpad == 5 || dpad == 6 || dpad == 7) buttons |= PAD_BUTTON_DPAD_LEFT;
	if (buttonData[0] & 0x10) buttons |= PAD_BUTTON_SQUARE;
	if (buttonData[0] & 0x20) buttons |= PAD_BUTTON_CROSS;
	if (buttonData[0] & 0x40) buttons |= PAD_BUTTON_CIRCLE;
	if (buttonData[0] & 0x80) buttons |= PAD_BUTTON_TRIANGLE;
	if (buttonData[1] & 0x01) buttons |= PAD_BUTTON_L1;
	if (buttonData[1] & 0x02) buttons |= PAD_BUTTON_R1;
	if (buttonData[1] & 0x04) buttons |= PAD_BUTTON_L2;
	if (buttonData[1] & 0x08) buttons |= PAD_BUTTON_R2;
	if (buttonData[1] & 0x10) buttons |= PAD_BUTTON_SHARE;
	if (buttonData[1] & 0x20) buttons |= PAD_BUTTON_OPTIONS;
	if (buttonData[1] & 0x40) buttons |= PAD_BUTTON_L3;
	if (buttonData[1] & 0x80) buttons |= PAD_BUTTON_R3;
	if (buttonData[2] & 0x01) buttons |= PAD_BUTTON_PS;
	state.Buttons = buttons;
}

bool PadCalcParseDs4UsbReport(const unsigned char* data, int size, PAD_STATE& state)
{
	if (data == nullptr || size < DS4_USB_REPORT_MIN_SIZE || data[0] != DS4_USB_REPORT_ID) return false;
	ReadUsbState(data, 5, state);
	return true;
}

bool PadCalcParseDualSenseUsbReport(const unsigned char* data, int size, PAD_STATE& state)
{
	if (data == nullptr || size != DUALSENSE_USB_REPORT_SIZE || data[0] != 0x01) return false;
	ReadUsbState(data, 8, state);
	return true;
}

static float SignedStick(int value)
{
	return (float)value / (value < 0 ? 32768.0f : 32767.0f);
}

void PadCalcParseXInput(unsigned int buttons, unsigned char leftTrigger, unsigned char rightTrigger,
	short lx, short ly, short rx, short ry, PAD_STATE& state)
{
	// Windows のヘッダーに依存しない XInput のビット値。
	static constexpr unsigned int bits[] = {
		0x1000, 0x2000, 0x4000, 0x8000, 0x0100, 0x0200,
		0x0020, 0x0010, 0x0040, 0x0080, 0x0001, 0x0008, 0x0002, 0x0004
	};
	static constexpr unsigned int mapped[] = {
		PAD_BUTTON_CROSS, PAD_BUTTON_CIRCLE, PAD_BUTTON_SQUARE, PAD_BUTTON_TRIANGLE,
		PAD_BUTTON_L1, PAD_BUTTON_R1, PAD_BUTTON_SHARE, PAD_BUTTON_OPTIONS,
		PAD_BUTTON_L3, PAD_BUTTON_R3, PAD_BUTTON_DPAD_UP, PAD_BUTTON_DPAD_RIGHT,
		PAD_BUTTON_DPAD_DOWN, PAD_BUTTON_DPAD_LEFT
	};
	state = {};
	for (int i = 0; i < (int)(sizeof(bits) / sizeof(bits[0])); ++i) {
		if (buttons & bits[i]) state.Buttons |= mapped[i];
	}
	if (leftTrigger > PAD_TRIGGER_THRESHOLD) state.Buttons |= PAD_BUTTON_L2;
	if (rightTrigger > PAD_TRIGGER_THRESHOLD) state.Buttons |= PAD_BUTTON_R2;
	state.LeftStick = { SignedStick(lx), SignedStick(ly) };
	state.RightStick = { SignedStick(rx), SignedStick(ry) };
}

static float SwitchStick(int value)
{
	return fmaxf(-1.0f, fminf(1.0f, ((float)value - 2048.0f) / SWITCH_STICK_RANGE));
}

bool PadCalcParseSwitchProReport(const unsigned char* data, int size, PAD_STATE& state)
{
	if (data == nullptr || size < 12 || (data[0] != 0x30 && data[0] != 0x3F)) return false;
	state = {};
	if (data[0] == 0x30) {
		const unsigned char right = data[3], middle = data[4], left = data[5];
		if (right & 0x01) state.Buttons |= PAD_BUTTON_SQUARE;
		if (right & 0x02) state.Buttons |= PAD_BUTTON_TRIANGLE;
		if (right & 0x04) state.Buttons |= PAD_BUTTON_CROSS;
		if (right & 0x08) state.Buttons |= PAD_BUTTON_CIRCLE;
		if (right & 0x40) state.Buttons |= PAD_BUTTON_R1;
		if (right & 0x80) state.Buttons |= PAD_BUTTON_R2;
		if (middle & 0x01) state.Buttons |= PAD_BUTTON_SHARE;
		if (middle & 0x02) state.Buttons |= PAD_BUTTON_OPTIONS;
		if (middle & 0x04) state.Buttons |= PAD_BUTTON_R3;
		if (middle & 0x08) state.Buttons |= PAD_BUTTON_L3;
		if (middle & 0x10) state.Buttons |= PAD_BUTTON_PS;
		if (left & 0x01) state.Buttons |= PAD_BUTTON_DPAD_DOWN;
		if (left & 0x02) state.Buttons |= PAD_BUTTON_DPAD_UP;
		if (left & 0x04) state.Buttons |= PAD_BUTTON_DPAD_RIGHT;
		if (left & 0x08) state.Buttons |= PAD_BUTTON_DPAD_LEFT;
		if (left & 0x40) state.Buttons |= PAD_BUTTON_L1;
		if (left & 0x80) state.Buttons |= PAD_BUTTON_L2;
		state.LeftStick = { SwitchStick(data[6] | ((data[7] & 0x0F) << 8)),
			SwitchStick((data[7] >> 4) | (data[8] << 4)) };
		state.RightStick = { SwitchStick(data[9] | ((data[10] & 0x0F) << 8)),
			SwitchStick((data[10] >> 4) | (data[11] << 4)) };
	}
	else {
		static constexpr unsigned int mapped[] = { PAD_BUTTON_CROSS, PAD_BUTTON_CIRCLE,
			PAD_BUTTON_SQUARE, PAD_BUTTON_TRIANGLE, PAD_BUTTON_L1, PAD_BUTTON_R1, PAD_BUTTON_L2, PAD_BUTTON_R2 };
		for (int i = 0; i < 8; ++i) if (data[1] & (1u << i)) state.Buttons |= mapped[i];
		if (data[2] & 0x01) state.Buttons |= PAD_BUTTON_SHARE;
		if (data[2] & 0x02) state.Buttons |= PAD_BUTTON_OPTIONS;
		if (data[2] & 0x04) state.Buttons |= PAD_BUTTON_L3;
		if (data[2] & 0x08) state.Buttons |= PAD_BUTTON_R3;
		if (data[2] & 0x10) state.Buttons |= PAD_BUTTON_PS;
		const int hat = data[3] & 0x0F;
		if (hat == 0 || hat == 1 || hat == 7) state.Buttons |= PAD_BUTTON_DPAD_UP;
		if (hat == 1 || hat == 2 || hat == 3) state.Buttons |= PAD_BUTTON_DPAD_RIGHT;
		if (hat == 3 || hat == 4 || hat == 5) state.Buttons |= PAD_BUTTON_DPAD_DOWN;
		if (hat == 5 || hat == 6 || hat == 7) state.Buttons |= PAD_BUTTON_DPAD_LEFT;
		state.LeftStick = { SignedStick((data[4] | (data[5] << 8)) - 32768),
			-SignedStick((data[6] | (data[7] << 8)) - 32768) };
		state.RightStick = { SignedStick((data[8] | (data[9] << 8)) - 32768),
			-SignedStick((data[10] | (data[11] << 8)) - 32768) };
	}
	return true;
}

bool PadCalcBuildSwitchUsbCommand(unsigned char* data, int size, unsigned char command)
{
	if (data == nullptr || size < 2) return false;
	for (int i = 0; i < size; ++i) data[i] = 0;
	data[0] = 0x80;
	data[1] = command;
	return true;
}

bool PadCalcBuildSwitchInputMode(unsigned char* data, int size, unsigned char counter)
{
	if (data == nullptr || size < 12) return false;
	for (int i = 0; i < size; ++i) data[i] = 0;
	data[0] = 0x01;
	data[1] = counter & 0x0F;
	static constexpr unsigned char silent[] = { 0x00, 0x01, 0x40, 0x40, 0x00, 0x01, 0x40, 0x40 };
	for (int i = 0; i < 8; ++i) data[i + 2] = silent[i];
	data[10] = 0x03;
	data[11] = 0x30;
	return true;
}
