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
static constexpr bool RUMBLE_ENABLED = false;	// 振動（true：振動する、false：振動しない。ライトバーはどちらでも光る）

// DUALSHOCK 4 の見分け方（USB の VID と PID）
static constexpr DWORD DS4_VENDOR_ID = 0x054C;		// ソニー
static constexpr DWORD DS4_PRODUCT_ID_V1 = 0x05C4;	// 初代（CUH-ZCT1）
static constexpr DWORD DS4_PRODUCT_ID_V2 = 0x09CC;	// 2代目（CUH-ZCT2）

static constexpr UINT RAW_INPUT_BUFFER_SIZE = 1024;	// WM_INPUT のデータを受け取る大きさ（DS4 の USB は 64 バイト＋ヘッダー）
static constexpr int DS4_USB_OUTPUT_DEFAULT_SIZE = 32; // HID から出力の長さを取得できなかったとき


static HANDLE g_Device = NULL;			// 読んでいる DUALSHOCK 4（つないでいないときは NULL）
static PAD_STATE g_Received = {};		// WM_INPUT で最後に届いた状態
static PAD_STATE g_State = {};			// このフレームの状態（遊びを取り除いたもの）
static HWND g_Window = NULL;             // 出力を使うゲームのウィンドウ
static HANDLE g_OutputDevice = INVALID_HANDLE_VALUE; // 入力とは別に開く、出力専用のハンドル
static bool g_OutputAttempted = false;   // 接続ごとに一度だけ開く（失敗しても入力は続ける）
static bool g_OutputWarningShown = false;
static PAD_RUMBLE g_Rumble = {};
static float g_LightColor[3] = {};
static std::vector<unsigned char> g_OutputReport;
static std::vector<unsigned char> g_LastOutput;


// 出力できなくても、入力には影響させない。接続中は同じ警告を繰り返さない。
static void WarnOutputUnavailable(void)
{
	if (g_OutputWarningShown) return;
	g_OutputWarningShown = true;
#ifdef _DEBUG
	hal::dout << "パッド：振動・ライトバーを出力できません（入力は続けます）" << std::endl;
#endif
}

// 送信するバイト列が変わったときだけ書く。非選択中は必ず振動 0 にする。
static void SendOutput(void)
{
	if (g_OutputDevice == INVALID_HANDLE_VALUE) return;
	const bool active = g_Window != NULL && GetForegroundWindow() == g_Window;
	if (!PadCalcBuildDs4UsbOutput(g_OutputReport.data(), (int)g_OutputReport.size(),
		active ? g_Rumble.SmallMotor : 0.0f, active ? g_Rumble.LargeMotor : 0.0f,
		g_LightColor[0], g_LightColor[1], g_LightColor[2])) {
		WarnOutputUnavailable();
		return;
	}
	if (g_OutputReport == g_LastOutput) return;
	DWORD written = 0;
	const DWORD size = (DWORD)g_OutputReport.size();
	if (!WriteFile(g_OutputDevice, g_OutputReport.data(), size, &written, NULL) || written != size) {
		WarnOutputUnavailable();
	}
	// 失敗時も同じ値で毎フレーム再送しない。値が変われば、停止も含め再び送る。
	g_LastOutput = g_OutputReport;
}

static void CloseOutputDevice(void)
{
	g_Rumble = {};
	if (g_OutputDevice != INVALID_HANDLE_VALUE) {
		SendOutput(); // 外れたときも、閉じる前に停止を送る（外れていれば失敗してよい）
		CloseHandle(g_OutputDevice);
		g_OutputDevice = INVALID_HANDLE_VALUE;
	}
	g_OutputReport.clear();
	g_LastOutput.clear();
}

// USB の入力レポートを読めた最初の一度だけ、同じ機器を出力用に開く。
static void OpenOutputDevice(void)
{
	g_OutputAttempted = true;
	UINT nameSize = 0;
	if (GetRawInputDeviceInfoA(g_Device, RIDI_DEVICENAME, NULL, &nameSize) == (UINT)-1 || nameSize == 0) {
		WarnOutputUnavailable();
		return;
	}
	std::vector<char> name((size_t)nameSize + 1, 0);
	if (GetRawInputDeviceInfoA(g_Device, RIDI_DEVICENAME, name.data(), &nameSize) == (UINT)-1) {
		WarnOutputUnavailable();
		return;
	}
	g_OutputDevice = CreateFileA(name.data(), GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL, OPEN_EXISTING, 0, NULL);
	if (g_OutputDevice == INVALID_HANDLE_VALUE) {
		WarnOutputUnavailable();
		return;
	}

	HIDP_CAPS caps{};
	PHIDP_PREPARSED_DATA preparsed = NULL;
	if (HidD_GetPreparsedData(g_OutputDevice, &preparsed)) {
		if (HidP_GetCaps(preparsed, &caps) != HIDP_STATUS_SUCCESS) caps.OutputReportByteLength = 0;
		HidD_FreePreparsedData(preparsed);
	}
	const int size = caps.OutputReportByteLength != 0
		? (int)caps.OutputReportByteLength : DS4_USB_OUTPUT_DEFAULT_SIZE;
	g_OutputReport.resize(size);
	SendOutput(); // つなぎ直したときも、現在のライトバーの色と停止状態を反映する
}


// 何も押していない状態
static PAD_STATE NeutralState(void)
{
	PAD_STATE state{};
	return state;
}

// 機器が DUALSHOCK 4 か調べる
static bool IsDs4(HANDLE device)
{
	RID_DEVICE_INFO info{};
	info.cbSize = sizeof(info);
	UINT size = sizeof(info);
	if (GetRawInputDeviceInfoA(device, RIDI_DEVICEINFO, &info, &size) == (UINT)-1 || info.dwType != RIM_TYPEHID) {
		return false;
	}
	return info.hid.dwVendorId == DS4_VENDOR_ID
		&& (info.hid.dwProductId == DS4_PRODUCT_ID_V1 || info.hid.dwProductId == DS4_PRODUCT_ID_V2);
}


void PadInitialize(HWND hWnd)
{
	CloseOutputDevice();
	g_Window = hWnd;
	g_OutputAttempted = false;
	g_OutputWarningShown = false;
	for (float& color : g_LightColor) color = 0.0f;
	g_Device = NULL;
	g_Received = NeutralState();
	g_State = NeutralState();

	// Raw Input にゲームパッドを登録する。RIDEV_DEVNOTIFY で、つないだ・外したときに WM_INPUT_DEVICE_CHANGE が届く
	RAWINPUTDEVICE rid{};
	rid.usUsagePage = 0x01;	// 汎用デスクトップ
	rid.usUsage = 0x05;		// ゲームパッド
	rid.dwFlags = RIDEV_DEVNOTIFY;
	rid.hwndTarget = hWnd;
	RegisterRawInputDevices(&rid, 1, sizeof(rid));
}

void PadFinalize(void)
{
	CloseOutputDevice();
	g_Window = NULL;
	g_OutputAttempted = false;
	g_Device = NULL;
	g_Received = NeutralState();
	g_State = NeutralState();
}

void PadUpdate(void)
{
	if (g_Window == NULL || GetForegroundWindow() != g_Window) {
		g_Rumble = {}; // 戻ってきたときに、古い振動を再開しない
	}
	else {
		PadCalcUpdateRumble(g_Rumble, GameTimeGetDeltaTime());
	}
	SendOutput();

	// このゲームのウィンドウが選ばれていないときは、データが届かず最後の状態のまま残るので、何も押していない扱いにする
	if (g_Device == NULL || GetActiveWindow() == NULL) {
		g_State = NeutralState();
		return;
	}

	g_State = g_Received;
	g_State.LeftStick = PadCalcApplyDeadZone(g_Received.LeftStick, STICK_DEAD_ZONE);
	g_State.RightStick = PadCalcApplyDeadZone(g_Received.RightStick, STICK_DEAD_ZONE);
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

	// まだ1台も読んでいなければ、DUALSHOCK 4 かどうか調べて、最初の1台を使う
	if (g_Device == NULL) {
		if (!IsDs4(raw->header.hDevice)) {
			return;
		}
		g_Device = raw->header.hDevice;
#ifdef _DEBUG
		hal::dout << "パッド：DUALSHOCK 4 をつなぎました" << std::endl;
#endif
	}
	if (raw->header.hDevice != g_Device) {
		return;	// 2台目以降は読まない
	}

	// 1回の WM_INPUT にレポートが何個か入っていることがあるので、最後の1個を使う
	const DWORD reportSize = raw->data.hid.dwSizeHid;
	const DWORD count = raw->data.hid.dwCount;
	if (reportSize == 0 || count == 0) {
		return;
	}
	const BYTE* last = raw->data.hid.bRawData + reportSize * (count - 1);
	if (PadCalcParseDs4UsbReport(last, (int)reportSize, g_Received) && !g_OutputAttempted) {
		OpenOutputDevice(); // Bluetooth の入力レポート（0x11）では出力を開かない
	}
}

void PadOnDeviceChange(WPARAM wParam, LPARAM lParam)
{
	// 読んでいる DUALSHOCK 4 が外れたら、何も押していない状態に戻して、次につないだものを読めるようにする
	if (wParam == GIDC_REMOVAL && (HANDLE)lParam == g_Device) {
		CloseOutputDevice();
		g_OutputAttempted = false;
		g_OutputWarningShown = false;
		g_Device = NULL;
		g_Received = NeutralState();
		g_State = NeutralState();
#ifdef _DEBUG
		hal::dout << "パッド：DUALSHOCK 4 が外れました" << std::endl;
#endif
	}
}

bool PadIsConnected(void)
{
	return g_Device != NULL;
}

bool PadIsPress(PAD_BUTTON button)
{
	return (g_State.Buttons & button) != 0;
}

ACTION_AXIS PadGetLeftStick(void)
{
	return g_State.LeftStick;
}

ACTION_AXIS PadGetRightStick(void)
{
	return g_State.RightStick;
}

void PadRumble(float smallMotor, float largeMotor, float seconds)
{
	if (!RUMBLE_ENABLED || g_OutputDevice == INVALID_HANDLE_VALUE || g_Window == NULL || GetForegroundWindow() != g_Window) {
		g_Rumble = {};
	}
	else {
		PadCalcRumble(g_Rumble, smallMotor, largeMotor, seconds);
	}
	SendOutput();
}

void PadSetLightColor(float red, float green, float blue)
{
	g_LightColor[0] = red;
	g_LightColor[1] = green;
	g_LightColor[2] = blue;
	SendOutput();
}
