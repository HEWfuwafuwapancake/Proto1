#include "input/pad.h"

#include "core/gametime.h"
#include <vector>
extern "C" {
#include <hidsdi.h>
}

#pragma comment(lib, "hid.lib")

#ifdef _DEBUG
#include <iostream>
#include "core/debug_ostream.h"
#endif


//------------------------------------------------------------------------------
//	調整用の値
//------------------------------------------------------------------------------
static constexpr float STICK_DEAD_ZONE = 0.12f;	// スティックの遊び（手を離したときのずれを 0 にする大きさ。0〜1）
static constexpr bool RUMBLE_ENABLED_AT_START = false;	// 起動したときの振動（true：振動する、false：振動しない）。ゲーム中は SHARE／V で切り替える

static constexpr UINT RAW_INPUT_BUFFER_SIZE = 1024;	// WM_INPUT のデータを受け取る大きさ（DS4 の USB は 64 バイト＋ヘッダー）
static constexpr int DS4_USB_OUTPUT_DEFAULT_SIZE = 32; // HID から出力の長さを取得できなかったとき
static constexpr int DUALSENSE_USB_OUTPUT_DEFAULT_SIZE = 63; // DualSense の USB 出力レポート全体


/// @brief 1台の入力・出力・振動の状態（切断した番号だけを空ける）
struct PAD_DEVICE
{
	HANDLE Device = NULL;                         ///< Raw Input の機器
	PAD_MODEL Model = PAD_MODEL_NONE;             ///< 入出力レポートの種類
	PAD_STATE Received = {};                      ///< 最後に届いた入力
	PAD_STATE State = {};                         ///< このフレームの入力
	HANDLE OutputDevice = INVALID_HANDLE_VALUE;   ///< 出力専用のハンドル
	bool OutputAttempted = false;                 ///< 接続ごとに一度だけ開く
	bool OutputWarningShown = false;              ///< 同じ警告を繰り返さない
	PAD_RUMBLE Rumble = {};                       ///< 振動の強さと残り時間
	float LightColor[3] = {};                     ///< 未接続でも色を覚える
	std::vector<unsigned char> OutputReport;      ///< 送信するデータ
	std::vector<unsigned char> LastOutput;        ///< 最後に送ったデータ
};
static PAD_DEVICE g_Pads[PAD_COUNT];
static HWND g_Window = NULL;
static bool g_RumbleEnabled = RUMBLE_ENABLED_AT_START;

static bool IsValidPad(int pad)
{
	return pad >= 0 && pad < PAD_COUNT;
}


// 出力できなくても、入力には影響させない。接続中は同じ警告を繰り返さない。
static void WarnOutputUnavailable(PAD_DEVICE& pad)
{
	if (pad.OutputWarningShown) return;
	pad.OutputWarningShown = true;
#ifdef _DEBUG
	hal::dout << "パッド：振動・ライトバーを出力できません（入力は続けます）" << std::endl;
#endif
}

// 送信するバイト列が変わったときだけ書く。非選択中は必ず振動 0 にする。
static void SendOutput(PAD_DEVICE& pad)
{
	if (pad.OutputDevice == INVALID_HANDLE_VALUE) return;
	const bool active = g_Window != NULL && GetForegroundWindow() == g_Window;
	const auto buildOutput = pad.Model == PAD_MODEL_DUALSENSE
		? PadCalcBuildDualSenseUsbOutput : PadCalcBuildDs4UsbOutput;
	if (!buildOutput(pad.OutputReport.data(), (int)pad.OutputReport.size(),
		active ? pad.Rumble.SmallMotor : 0.0f, active ? pad.Rumble.LargeMotor : 0.0f,
		pad.LightColor[0], pad.LightColor[1], pad.LightColor[2])) {
		WarnOutputUnavailable(pad);
		return;
	}
	if (pad.OutputReport == pad.LastOutput) return;
	DWORD written = 0;
	const DWORD size = (DWORD)pad.OutputReport.size();
	if (!WriteFile(pad.OutputDevice, pad.OutputReport.data(), size, &written, NULL) || written != size) {
		WarnOutputUnavailable(pad);
	}
	// 失敗時も同じ値で毎フレーム再送しない。値が変われば、停止も含め再び送る。
	pad.LastOutput = pad.OutputReport;
}

static void CloseOutputDevice(PAD_DEVICE& pad)
{
	pad.Rumble = {};
	if (pad.OutputDevice != INVALID_HANDLE_VALUE) {
		SendOutput(pad); // 外れたときも、閉じる前に停止を送る（外れていれば失敗してよい）
		CloseHandle(pad.OutputDevice);
		pad.OutputDevice = INVALID_HANDLE_VALUE;
	}
	pad.OutputReport.clear();
	pad.LastOutput.clear();
}

// USB の入力レポートを読めた最初の一度だけ、同じ機器を出力用に開く。
static void OpenOutputDevice(PAD_DEVICE& pad)
{
	pad.OutputAttempted = true;
	UINT nameSize = 0;
	if (GetRawInputDeviceInfoA(pad.Device, RIDI_DEVICENAME, NULL, &nameSize) == (UINT)-1 || nameSize == 0) {
		WarnOutputUnavailable(pad);
		return;
	}
	std::vector<char> name((size_t)nameSize + 1, 0);
	if (GetRawInputDeviceInfoA(pad.Device, RIDI_DEVICENAME, name.data(), &nameSize) == (UINT)-1) {
		WarnOutputUnavailable(pad);
		return;
	}
	pad.OutputDevice = CreateFileA(name.data(), GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL, OPEN_EXISTING, 0, NULL);
	if (pad.OutputDevice == INVALID_HANDLE_VALUE) {
		WarnOutputUnavailable(pad);
		return;
	}

	HIDP_CAPS caps{};
	PHIDP_PREPARSED_DATA preparsed = NULL;
	if (HidD_GetPreparsedData(pad.OutputDevice, &preparsed)) {
		if (HidP_GetCaps(preparsed, &caps) != HIDP_STATUS_SUCCESS) caps.OutputReportByteLength = 0;
		HidD_FreePreparsedData(preparsed);
	}
	const int defaultSize = pad.Model == PAD_MODEL_DUALSENSE
		? DUALSENSE_USB_OUTPUT_DEFAULT_SIZE : DS4_USB_OUTPUT_DEFAULT_SIZE;
	const int size = caps.OutputReportByteLength != 0
		? (int)caps.OutputReportByteLength : defaultSize;
	pad.OutputReport.resize(size);
	SendOutput(pad); // つなぎ直したときも、現在のライトバーの色と停止状態を反映する
}


// ゲームパッドの VID/PID から、対応する入出力形式を調べる。
static PAD_MODEL GetDeviceModel(HANDLE device)
{
	RID_DEVICE_INFO info{};
	info.cbSize = sizeof(info);
	UINT size = sizeof(info);
	if (GetRawInputDeviceInfoA(device, RIDI_DEVICEINFO, &info, &size) == (UINT)-1 || info.dwType != RIM_TYPEHID) {
		return PAD_MODEL_NONE;
	}
	if (info.hid.usUsagePage != 0x01 || info.hid.usUsage != 0x05) return PAD_MODEL_NONE;
	return PadCalcIdentifyModel(info.hid.dwVendorId, info.hid.dwProductId);
}

static int FindDevice(HANDLE device)
{
	if (device == NULL) return -1;
	for (int i = 0; i < PAD_COUNT; ++i) {
		if (g_Pads[i].Device == device) return i;
	}
	return -1;
}

static int AddDevice(HANDLE device)
{
	const int found = FindDevice(device);
	if (found >= 0) return found;
	if (device == NULL) return -1;
	const PAD_MODEL model = GetDeviceModel(device);
	if (model == PAD_MODEL_NONE) return -1;
	for (int i = 0; i < PAD_COUNT; ++i) {
		if (g_Pads[i].Device != NULL) continue;
		g_Pads[i].Device = device;
		g_Pads[i].Model = model;
#ifdef _DEBUG
		hal::dout << "パッド" << i << "："
			<< (model == PAD_MODEL_DUALSENSE ? "DualSense" : "DUALSHOCK 4") << " をつなぎました" << std::endl;
#endif
		return i;
	}
	return -1;
}

// 起動前からつながっている機器にも番号を付ける（起動時だけは列挙順）。
static void FindConnectedDevices(void)
{
	UINT count = 0;
	if (GetRawInputDeviceList(NULL, &count, sizeof(RAWINPUTDEVICELIST)) == (UINT)-1 || count == 0) return;
	std::vector<RAWINPUTDEVICELIST> devices(count);
	const UINT read = GetRawInputDeviceList(devices.data(), &count, sizeof(RAWINPUTDEVICELIST));
	if (read == (UINT)-1) return;
	for (UINT i = 0; i < read; ++i) {
		if (devices[i].dwType == RIM_TYPEHID) AddDevice(devices[i].hDevice);
	}
}

void PadInitialize(HWND hWnd)
{
	PadFinalize();
	g_Window = hWnd;
	g_RumbleEnabled = RUMBLE_ENABLED_AT_START;

	// Raw Input にゲームパッドを登録する。RIDEV_DEVNOTIFY で、つないだ・外したときに WM_INPUT_DEVICE_CHANGE が届く
	RAWINPUTDEVICE rid{};
	rid.usUsagePage = 0x01;	// 汎用デスクトップ
	rid.usUsage = 0x05;		// ゲームパッド
	rid.dwFlags = RIDEV_DEVNOTIFY;
	rid.hwndTarget = hWnd;
	RegisterRawInputDevices(&rid, 1, sizeof(rid));
	FindConnectedDevices();
}

void PadFinalize(void)
{
	for (PAD_DEVICE& pad : g_Pads) {
		CloseOutputDevice(pad);
		pad = {};
	}
	g_Window = NULL;
}

void PadUpdate(void)
{
	const bool active = g_Window != NULL && GetForegroundWindow() == g_Window;
	for (PAD_DEVICE& pad : g_Pads) {
		if (!active) pad.Rumble = {}; // 戻ってきたときに古い振動を再開しない
		else PadCalcUpdateRumble(pad.Rumble, GameTimeGetDeltaTime());
		SendOutput(pad);
		if (pad.Device == NULL || !active) {
			pad.State = {};
			if (!active) pad.Received = {}; // 再選択後に古い入力を使わない
			continue;
		}
		pad.State = pad.Received;
		pad.State.LeftStick = PadCalcApplyDeadZone(pad.Received.LeftStick, STICK_DEAD_ZONE);
		pad.State.RightStick = PadCalcApplyDeadZone(pad.Received.RightStick, STICK_DEAD_ZONE);
	}
}

void PadOnRawInput(LPARAM lParam)
{
	// HID のデータは RAWINPUT 構造体より大きいので、大きめのバッファで受け取る
	alignas(8) BYTE buffer[RAW_INPUT_BUFFER_SIZE];
	UINT size = sizeof(buffer);
	if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, buffer, &size, sizeof(RAWINPUTHEADER)) == (UINT)-1) {
		return;	// マウスのデータは小さいので読めるが、大きすぎるデータはここで捨てる
	}

	const RAWINPUT* raw = (const RAWINPUT*)buffer;
	if (raw->header.dwType != RIM_TYPEHID) {
		return;	// マウスなど
	}

	const int index = AddDevice(raw->header.hDevice);
	if (index < 0) return; // 対応外・5台目以降
	PAD_DEVICE& pad = g_Pads[index];

	// 1回の WM_INPUT にレポートが何個か入っていることがあるので、最後の1個を使う
	const DWORD reportSize = raw->data.hid.dwSizeHid;
	const DWORD count = raw->data.hid.dwCount;
	if (reportSize == 0 || count == 0) {
		return;
	}
	const BYTE* last = raw->data.hid.bRawData + reportSize * (count - 1);
	const auto parseReport = pad.Model == PAD_MODEL_DUALSENSE
		? PadCalcParseDualSenseUsbReport : PadCalcParseDs4UsbReport;
	if (parseReport(last, (int)reportSize, pad.Received) && !pad.OutputAttempted) {
		OpenOutputDevice(pad); // USB 入力を読めたときだけ出力を開く
	}
}

void PadOnDeviceChange(WPARAM wParam, LPARAM lParam)
{
	if (wParam == GIDC_ARRIVAL) {
		AddDevice((HANDLE)lParam);
		return;
	}
	const int index = FindDevice((HANDLE)lParam);
	if (wParam == GIDC_REMOVAL && index >= 0) {
		PAD_DEVICE& pad = g_Pads[index];
		CloseOutputDevice(pad);
		pad.Device = NULL;
		pad.Model = PAD_MODEL_NONE;
		pad.OutputAttempted = false;
		pad.OutputWarningShown = false;
		pad.Received = {};
		pad.State = {};
#ifdef _DEBUG
		hal::dout << "パッド" << index << "：コントローラーが外れました" << std::endl;
#endif
	}
}

bool PadIsConnected(int pad)
{
	return IsValidPad(pad) && g_Pads[pad].Device != NULL;
}

bool PadIsPress(int pad, PAD_BUTTON button)
{
	return IsValidPad(pad) && (g_Pads[pad].State.Buttons & button) != 0;
}

ACTION_AXIS PadGetLeftStick(int pad)
{
	return IsValidPad(pad) ? g_Pads[pad].State.LeftStick : ACTION_AXIS{};
}

ACTION_AXIS PadGetRightStick(int pad)
{
	return IsValidPad(pad) ? g_Pads[pad].State.RightStick : ACTION_AXIS{};
}

void PadRumble(int index, float smallMotor, float largeMotor, float seconds)
{
	if (!IsValidPad(index)) return;
	PAD_DEVICE& pad = g_Pads[index];
	if (!g_RumbleEnabled || pad.OutputDevice == INVALID_HANDLE_VALUE || g_Window == NULL || GetForegroundWindow() != g_Window) {
		pad.Rumble = {};
	}
	else {
		PadCalcRumble(pad.Rumble, smallMotor, largeMotor, seconds);
	}
	SendOutput(pad);
}

void PadSetRumbleEnabled(bool enabled)
{
	g_RumbleEnabled = enabled;
	if (!enabled) {
		for (int i = 0; i < PAD_COUNT; ++i) PadRumble(i, 0.0f, 0.0f, 0.0f);
	}
}

bool PadIsRumbleEnabled(void)
{
	return g_RumbleEnabled;
}

void PadSetLightColor(int index, float red, float green, float blue)
{
	if (!IsValidPad(index)) return;
	PAD_DEVICE& pad = g_Pads[index];
	pad.LightColor[0] = red;
	pad.LightColor[1] = green;
	pad.LightColor[2] = blue;
	SendOutput(pad);
}
