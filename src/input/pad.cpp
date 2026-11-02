#include "input/pad.h"

#include "core/gametime.h"
#include <vector>
#include <Xinput.h>
#include <setupapi.h>
#include <cfgmgr32.h>
#include <winusb.h>
#include <cstring>
#include "core/mathutil.h"
extern "C" {
#include <hidsdi.h>
}

#pragma comment(lib, "hid.lib")
#pragma comment(lib, "XInput.lib")
#pragma comment(lib, "winusb.lib")
#pragma comment(lib, "setupapi.lib")
#pragma comment(lib, "cfgmgr32.lib")

#ifdef _DEBUG
#include <iostream>
#include "core/debug_ostream.h"
#endif


//------------------------------------------------------------------------------
//	調整用の値
//------------------------------------------------------------------------------
static constexpr float STICK_DEAD_ZONE = 0.12f;	// スティックの遊び（手を離したときのずれを 0 にする大きさ。0〜1）
static constexpr float XINPUT_POLL_INTERVAL = 1.0f; // 未割り当ての XInput を探す間隔（秒）
static constexpr bool RUMBLE_ENABLED_AT_START = false;	// 起動したときの振動（true：振動する、false：振動しない）。ゲーム中は SHARE／V で切り替える

static constexpr UINT RAW_INPUT_BUFFER_SIZE = 1024;	// WM_INPUT のデータを受け取る大きさ（DS4 の USB は 64 バイト＋ヘッダー）
static constexpr int DS4_USB_OUTPUT_DEFAULT_SIZE = 32; // HID から出力の長さを取得できなかったとき
static constexpr int DUALSENSE_USB_OUTPUT_DEFAULT_SIZE = 63; // DualSense の USB 出力レポート全体
static constexpr ULONG SWITCH2_WRITE_TIMEOUT_MS = 1000; // 初期化の送信待ちの上限
static constexpr ULONG SWITCH2_READ_TIMEOUT_MS = 100;   // 初期化の返信待ちの上限
static constexpr DWORD SWITCH2_RUMBLE_CLOSE_WAIT_MS = 100; // 閉じるときに振動の書き込みを待つ上限
static constexpr ULONGLONG SWITCH2_RUMBLE_STUCK_MS = 1000; // 振動の書き込みがこれだけ終わらなければ取り消す

// No.54 段階0で実機から調べた、WinUSB（MI_01）のデバイスインターフェース。
static const GUID SWITCH2_USB_INTERFACE_GUID = {
	0x6F13725E, 0xEF0E, 0x4FD3, { 0xAE, 0x5F, 0xB2, 0xDE, 0x98, 0x9E, 0xC8, 0x25 }
};


/// @brief 1台の入力・出力・振動の状態（切断した番号だけを空ける）
struct PAD_DEVICE
{
	HANDLE Device = NULL;                         ///< Raw Input の機器
	int XInputIndex = -1;                         ///< XInput の番号（未使用は -1）
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
	HANDLE Switch2UsbDevice = INVALID_HANDLE_VALUE; ///< Switch 2 Pro の初期化用 USB ハンドル
	WINUSB_INTERFACE_HANDLE Switch2Interface = NULL; ///< 同じ機器のインターフェース1
	unsigned char Switch2RumbleCounter = 0;       ///< Switch 2 Pro の振動の送信カウンター
	bool Switch2RumbleOn = false;                 ///< Switch 2 Pro に最後に送った振動が 0 でないか
	OVERLAPPED Switch2Write = {};                 ///< Switch 2 Pro の振動の書き込み（待たずに送る）
	bool Switch2Writing = false;                  ///< 振動の書き込みがまだ終わっていないか
	ULONGLONG Switch2WriteStartMs = 0;            ///< 振動の書き込みを始めた時刻（終わらないときに取り消す）
};
static PAD_DEVICE g_Pads[PAD_COUNT];
static HWND g_Window = NULL;
static bool g_RumbleEnabled = RUMBLE_ENABLED_AT_START;
static float g_XInputPollRemaining = 0.0f;
#ifdef _DEBUG
static std::vector<unsigned long long> g_UnsupportedIds; // 同じ VID/PID は一度だけ知らせる
#endif

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

// Switch 2 Pro の振動の書き込みが終わったか調べる。waitMs まで待っても終わらなければ取り消す。
static bool FinishSwitch2Write(PAD_DEVICE& pad, DWORD waitMs)
{
	if (!pad.Switch2Writing) return true;
	if (WaitForSingleObject(pad.Switch2Write.hEvent, waitMs) == WAIT_TIMEOUT) {
		if (waitMs == 0 && GetTickCount64() - pad.Switch2WriteStartMs < SWITCH2_RUMBLE_STUCK_MS) return false;
#ifdef _DEBUG
		if (waitMs == 0) hal::dout << "Switch 2 Pro：振動の書き込みが終わらないので取り消しました" << std::endl;
#endif
		CancelIoEx(pad.OutputDevice, &pad.Switch2Write);
	}
	DWORD written = 0;
	if (!GetOverlappedResult(pad.OutputDevice, &pad.Switch2Write, &written, TRUE)) WarnOutputUnavailable(pad);
	pad.Switch2Writing = false;
	return true;
}

// 送信するバイト列が変わったときだけ書く。非選択中は必ず振動 0 にする。
static void SendOutput(PAD_DEVICE& pad)
{
	const bool active = g_Window != NULL && GetForegroundWindow() == g_Window;
	if (pad.Model == PAD_MODEL_XINPUT) {
		XINPUT_VIBRATION vibration{};
		vibration.wLeftMotorSpeed = (WORD)(Saturate(active ? pad.Rumble.LargeMotor : 0.0f) * 65535.0f + 0.5f);
		vibration.wRightMotorSpeed = (WORD)(Saturate(active ? pad.Rumble.SmallMotor : 0.0f) * 65535.0f + 0.5f);
		pad.OutputReport = { (unsigned char)vibration.wLeftMotorSpeed, (unsigned char)(vibration.wLeftMotorSpeed >> 8),
			(unsigned char)vibration.wRightMotorSpeed, (unsigned char)(vibration.wRightMotorSpeed >> 8) };
		if (pad.OutputReport != pad.LastOutput) {
			XInputSetState((DWORD)pad.XInputIndex, &vibration);
			pad.LastOutput = pad.OutputReport;
		}
		return;
	}
	if (pad.Model == PAD_MODEL_SWITCH_PRO || pad.OutputDevice == INVALID_HANDLE_VALUE) return;
	if (pad.Model == PAD_MODEL_SWITCH2_PRO) {
		// 振動中は毎回送り直す（送らないと止まる）。止めるときは強さ 0 を1回だけ送る。
		// 機器が受け取るまで書き込みが終わらないことがあり、待つとゲームが固まるので、
		// 前の書き込みが終わっていなければこの回は送らない（止めるのは次の回に送る）。
		const float smallMotor = active ? pad.Rumble.SmallMotor : 0.0f;
		const float largeMotor = active ? pad.Rumble.LargeMotor : 0.0f;
		const bool on = smallMotor > 0.0f || largeMotor > 0.0f;
		if (!on && !pad.Switch2RumbleOn) return;
		if (!FinishSwitch2Write(pad, 0)) return;
		if (!PadCalcBuildSwitch2ProRumble(pad.OutputReport.data(), (int)pad.OutputReport.size(),
			smallMotor, largeMotor, pad.Switch2RumbleCounter++)) {
			WarnOutputUnavailable(pad);
			return;
		}
		ResetEvent(pad.Switch2Write.hEvent);
		if (WriteFile(pad.OutputDevice, pad.OutputReport.data(), (DWORD)pad.OutputReport.size(), NULL, &pad.Switch2Write)
			|| GetLastError() == ERROR_IO_PENDING) {
			pad.Switch2Writing = true;
			pad.Switch2WriteStartMs = GetTickCount64();
		}
		else WarnOutputUnavailable(pad);
		pad.Switch2RumbleOn = on;
		return;
	}
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
	if (pad.Model == PAD_MODEL_XINPUT) SendOutput(pad);
	if (pad.OutputDevice != INVALID_HANDLE_VALUE) {
		FinishSwitch2Write(pad, SWITCH2_RUMBLE_CLOSE_WAIT_MS);
		SendOutput(pad); // 外れたときも、閉じる前に停止を送る（外れていれば失敗してよい）
		FinishSwitch2Write(pad, SWITCH2_RUMBLE_CLOSE_WAIT_MS);
		CloseHandle(pad.OutputDevice);
		pad.OutputDevice = INVALID_HANDLE_VALUE;
	}
	if (pad.Switch2Write.hEvent != NULL) {
		CloseHandle(pad.Switch2Write.hEvent);
		pad.Switch2Write = {};
	}
	pad.OutputReport.clear();
	pad.LastOutput.clear();
	if (pad.Switch2Interface != NULL) {
		WinUsb_Free(pad.Switch2Interface);
		pad.Switch2Interface = NULL;
	}
	if (pad.Switch2UsbDevice != INVALID_HANDLE_VALUE) {
		CloseHandle(pad.Switch2UsbDevice);
		pad.Switch2UsbDevice = INVALID_HANDLE_VALUE;
	}
	pad.Switch2RumbleOn = false;
}

// HID と WinUSB の両方から USB 複合機器の親までたどり、同じ実機か確かめる。
static bool GetSwitch2UsbParent(DEVINST node, DEVINST& usbParent)
{
	static constexpr char prefix[] = "USB\\VID_057E&PID_2069\\";
	for (;;) {
		char id[MAX_DEVICE_ID_LEN]{};
		if (CM_Get_Device_IDA(node, id, MAX_DEVICE_ID_LEN, 0) != CR_SUCCESS) return false;
		if (_strnicmp(id, prefix, sizeof(prefix) - 1) == 0) {
			usbParent = node;
			return true;
		}
		DEVINST parent = 0;
		if (CM_Get_Parent(&parent, node, 0) != CR_SUCCESS) return false;
		node = parent;
	}
}

static SP_DEVICE_INTERFACE_DETAIL_DATA_A* GetInterfaceDetail(HDEVINFO devices,
	SP_DEVICE_INTERFACE_DATA& interfaceData, std::vector<BYTE>& buffer, SP_DEVINFO_DATA& deviceData)
{
	DWORD required = 0;
	SetupDiGetDeviceInterfaceDetailA(devices, &interfaceData, NULL, 0, &required, NULL);
	if (required < sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_A)) return nullptr;
	buffer.resize(required);
	auto* detail = reinterpret_cast<SP_DEVICE_INTERFACE_DETAIL_DATA_A*>(buffer.data());
	detail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_A);
	deviceData = {};
	deviceData.cbSize = sizeof(deviceData);
	if (!SetupDiGetDeviceInterfaceDetailA(devices, &interfaceData, detail, required, NULL, &deviceData)) return nullptr;
	return detail;
}

static bool GetSwitch2HidParent(HANDLE device, DEVINST& usbParent)
{
	UINT nameSize = 0;
	if (GetRawInputDeviceInfoA(device, RIDI_DEVICENAME, NULL, &nameSize) == (UINT)-1 || nameSize == 0) return false;
	std::vector<char> name((size_t)nameSize + 1, 0);
	if (GetRawInputDeviceInfoA(device, RIDI_DEVICENAME, name.data(), &nameSize) == (UINT)-1) return false;
	const HDEVINFO devices = SetupDiCreateDeviceInfoList(NULL, NULL);
	if (devices == INVALID_HANDLE_VALUE) return false;
	SP_DEVICE_INTERFACE_DATA interfaceData{};
	interfaceData.cbSize = sizeof(interfaceData);
	bool found = false;
	if (SetupDiOpenDeviceInterfaceA(devices, name.data(), 0, &interfaceData)) {
		SP_DEVINFO_DATA deviceData{};
		std::vector<BYTE> buffer;
		if (GetInterfaceDetail(devices, interfaceData, buffer, deviceData) != nullptr) {
			found = GetSwitch2UsbParent(deviceData.DevInst, usbParent);
		}
	}
	SetupDiDestroyDeviceInfoList(devices);
	return found;
}

// 失敗した場合の Win32 エラーを返す。ハンドルは成功・失敗とも呼び出し側が閉じる。
static DWORD InitializeSwitch2Usb(PAD_DEVICE& pad, const char* path, int& commandIndex)
{
	pad.Switch2UsbDevice = CreateFileA(path, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, NULL);
	if (pad.Switch2UsbDevice == INVALID_HANDLE_VALUE) return GetLastError();
	if (!WinUsb_Initialize(pad.Switch2UsbDevice, &pad.Switch2Interface)) return GetLastError();
	USB_INTERFACE_DESCRIPTOR descriptor{};
	if (!WinUsb_QueryInterfaceSettings(pad.Switch2Interface, 0, &descriptor)) return GetLastError();
	if (descriptor.bInterfaceNumber != 1) return ERROR_NOT_SUPPORTED;
	UCHAR input = 0, output = 0;
	for (UCHAR i = 0; i < descriptor.bNumEndpoints; ++i) {
		WINUSB_PIPE_INFORMATION pipe{};
		if (!WinUsb_QueryPipe(pad.Switch2Interface, 0, i, &pipe)) return GetLastError();
		if (pipe.PipeType != UsbdPipeTypeBulk) continue;
		if (USB_ENDPOINT_DIRECTION_IN(pipe.PipeId)) input = pipe.PipeId;
		else output = pipe.PipeId;
	}
	if (input == 0 || output == 0) return ERROR_NOT_FOUND;
	ULONG readTimeout = SWITCH2_READ_TIMEOUT_MS, writeTimeout = SWITCH2_WRITE_TIMEOUT_MS;
	if (!WinUsb_SetPipePolicy(pad.Switch2Interface, input, PIPE_TRANSFER_TIMEOUT, sizeof(readTimeout), &readTimeout)
		|| !WinUsb_SetPipePolicy(pad.Switch2Interface, output, PIPE_TRANSFER_TIMEOUT, sizeof(writeTimeout), &writeTimeout)) {
		return GetLastError();
	}
	for (commandIndex = 0; commandIndex < PAD_SWITCH2_INIT_COMMAND_COUNT; ++commandIndex) {
		unsigned char command[64]{};
		const int length = PadCalcBuildSwitch2UsbCommand(command, sizeof(command), commandIndex);
		if (length == 0) return ERROR_INVALID_DATA;
		ULONG transferred = 0;
		if (!WinUsb_WritePipe(pad.Switch2Interface, output, command, (ULONG)length, &transferred, NULL)) return GetLastError();
		if (transferred != (ULONG)length) return ERROR_WRITE_FAULT;
		// 返事は読むだけで、来なくても次へ進む（SDL と同じ。つないだ直後は返事が 100ms に間に合わないことがあり、
		// そこでやめると「振動を有効にする」などの残りのコマンドが送られない）。
		unsigned char reply[64]{};
		WinUsb_ReadPipe(pad.Switch2Interface, input, reply, sizeof(reply), &transferred, NULL);
	}
	return ERROR_SUCCESS;
}

static void OpenOutputDevice(PAD_DEVICE& pad);

// 入力開始の合図なので、最初の WM_INPUT を待たず、HID を見つけた時点で送る。
// Steam などがインターフェース1を開いていると失敗する（Win32=5）が、そのアプリが初期化済みなら入力は届くので、
// 失敗しても入力は受け取る（初期化されていなければ、そもそも入力が届かない）。
static void OpenSwitch2Device(PAD_DEVICE& pad)
{
	pad.OutputAttempted = true; // 失敗しても接続中は再送しない。
	DEVINST hidParent = 0;
	DWORD error = ERROR_NOT_FOUND;
	int commandIndex = -1;
	if (GetSwitch2HidParent(pad.Device, hidParent)) {
		const HDEVINFO devices = SetupDiGetClassDevsA(&SWITCH2_USB_INTERFACE_GUID, NULL, NULL,
			DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
		if (devices != INVALID_HANDLE_VALUE) {
			for (DWORD i = 0;; ++i) {
				SP_DEVICE_INTERFACE_DATA interfaceData{};
				interfaceData.cbSize = sizeof(interfaceData);
				if (!SetupDiEnumDeviceInterfaces(devices, NULL, &SWITCH2_USB_INTERFACE_GUID, i, &interfaceData)) break;
				std::vector<BYTE> buffer;
				SP_DEVINFO_DATA deviceData{};
				const auto* detail = GetInterfaceDetail(devices, interfaceData, buffer, deviceData);
				if (detail == nullptr) continue;
				char id[MAX_DEVICE_ID_LEN]{};
				static constexpr char prefix[] = "USB\\VID_057E&PID_2069&MI_01\\";
				DEVINST usbParent = 0;
				if (CM_Get_Device_IDA(deviceData.DevInst, id, MAX_DEVICE_ID_LEN, 0) != CR_SUCCESS
					|| _strnicmp(id, prefix, sizeof(prefix) - 1) != 0
					|| !GetSwitch2UsbParent(deviceData.DevInst, usbParent) || usbParent != hidParent) continue;
				error = InitializeSwitch2Usb(pad, detail->DevicePath, commandIndex);
				break;
			}
			SetupDiDestroyDeviceInfoList(devices);
		}
		else error = GetLastError();
	}
	if (error != ERROR_SUCCESS) CloseOutputDevice(pad);
#ifdef _DEBUG
	if (error == ERROR_SUCCESS) hal::dout << "Switch 2 Pro：USB の初期化が終わりました" << std::endl;
	else hal::dout << "Switch 2 Pro：USB の初期化に失敗（Win32=" << error
		<< "、コマンド=" << commandIndex + 1 << "。0は送信前）。Steam などが初期化済みなら入力は届きます。"
		<< "届かなければ抜き差ししてください" << std::endl;
#endif
	OpenOutputDevice(pad); // 振動は HID の出力で送るので、初期化の成否にかかわらず開く
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
	const bool switch2 = pad.Model == PAD_MODEL_SWITCH2_PRO; // 振動を毎フレーム送るので、待たずに書く
	pad.OutputDevice = CreateFileA(name.data(), GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL, OPEN_EXISTING, switch2 ? FILE_FLAG_OVERLAPPED : 0, NULL);
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
	if (pad.Model == PAD_MODEL_SWITCH_PRO) {
		// HID の長さだけを使う。Bluetooth で USB の合図が失敗しても警告せず続ける。
		pad.OutputReport.resize(caps.OutputReportByteLength);
		DWORD written = 0;
		static constexpr unsigned char commands[] = { 0x02, 0x04 };
		for (const unsigned char command : commands) {
			if (PadCalcBuildSwitchUsbCommand(pad.OutputReport.data(), (int)pad.OutputReport.size(), command)) {
				WriteFile(pad.OutputDevice, pad.OutputReport.data(), (DWORD)pad.OutputReport.size(), &written, NULL);
			}
		}
		if (PadCalcBuildSwitchInputMode(pad.OutputReport.data(), (int)pad.OutputReport.size(), 0)) {
			WriteFile(pad.OutputDevice, pad.OutputReport.data(), (DWORD)pad.OutputReport.size(), &written, NULL);
		}
		return;
	}
	if (switch2) {
		pad.OutputReport.resize(caps.OutputReportByteLength); // 振動のレポートは HID の長さ（64）で送る
		pad.Switch2Write.hEvent = CreateEventA(NULL, TRUE, FALSE, NULL);
		if (pad.Switch2Write.hEvent == NULL) {
			CloseHandle(pad.OutputDevice);
			pad.OutputDevice = INVALID_HANDLE_VALUE;
			WarnOutputUnavailable(pad);
		}
		return;
	}
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
	// Switch Pro 形式はゲームパッド（0x05）ではなくジョイスティック（0x04）と名乗る
	if (info.hid.usUsagePage != 0x01 || (info.hid.usUsage != 0x04 && info.hid.usUsage != 0x05)) return PAD_MODEL_NONE;
	const PAD_MODEL model = PadCalcIdentifyModel(info.hid.dwVendorId, info.hid.dwProductId);
#ifdef _DEBUG
	if (model == PAD_MODEL_NONE) {
		const unsigned long long id = ((unsigned long long)info.hid.dwVendorId << 32) | info.hid.dwProductId;
		bool shown = false;
		for (const auto old : g_UnsupportedIds) if (old == id) shown = true;
		if (!shown) {
			g_UnsupportedIds.push_back(id);
			hal::dout << "対応外のゲームパッド VID=0x" << std::hex << info.hid.dwVendorId
				<< " PID=0x" << info.hid.dwProductId << std::dec << std::endl;
		}
	}
#endif
	return model;
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
		if (g_Pads[i].Model != PAD_MODEL_NONE) continue;
		g_Pads[i].Device = device;
		g_Pads[i].Model = model;
#ifdef _DEBUG
		hal::dout << "パッド" << i << "："
			<< (model == PAD_MODEL_SWITCH2_PRO ? "Switch 2 Pro" : model == PAD_MODEL_SWITCH_PRO ? "Switch Pro"
				: model == PAD_MODEL_DUALSENSE ? "DualSense" : "DUALSHOCK 4") << " をつなぎました" << std::endl;
#endif
		// Switch Pro 形式は合図を受けるまで入力を送らない機種があるので、つないだ時点で送る
		if (model == PAD_MODEL_SWITCH_PRO) OpenOutputDevice(g_Pads[i]);
		if (model == PAD_MODEL_SWITCH2_PRO) OpenSwitch2Device(g_Pads[i]);
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
	g_XInputPollRemaining = 0.0f;

	// Raw Input にゲームパッドとジョイスティックを登録する。RIDEV_DEVNOTIFY で、つないだ・外したときに WM_INPUT_DEVICE_CHANGE が届く
	// （Switch Pro 形式はジョイスティックとして見える。対応外の機器は GetDeviceModel で捨てる）
	RAWINPUTDEVICE rid[2]{};
	rid[0].usUsagePage = 0x01;	// 汎用デスクトップ
	rid[0].usUsage = 0x05;		// ゲームパッド
	rid[0].dwFlags = RIDEV_DEVNOTIFY;
	rid[0].hwndTarget = hWnd;
	rid[1] = rid[0];
	rid[1].usUsage = 0x04;		// ジョイスティック
	RegisterRawInputDevices(rid, 2, sizeof(rid[0]));
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
	// 接続済みは毎フレーム読む。切断時はその番号だけを空け、ライトバーの予約色は残す。
	for (PAD_DEVICE& pad : g_Pads) {
		if (pad.Model != PAD_MODEL_XINPUT) continue;
		XINPUT_STATE input{};
		const DWORD result = XInputGetState((DWORD)pad.XInputIndex, &input);
		if (result == ERROR_DEVICE_NOT_CONNECTED) {
			CloseOutputDevice(pad);
			pad.Model = PAD_MODEL_NONE;
			pad.XInputIndex = -1;
			pad.Received = {};
			pad.State = {};
		}
		else if (result == ERROR_SUCCESS) {
			const XINPUT_GAMEPAD& value = input.Gamepad;
			PadCalcParseXInput(value.wButtons, value.bLeftTrigger, value.bRightTrigger,
				value.sThumbLX, value.sThumbLY, value.sThumbRX, value.sThumbRY, pad.Received);
		}
		else pad.Received = {};
	}
	g_XInputPollRemaining -= GameTimeGetDeltaTime();
	if (g_XInputPollRemaining <= 0.0f) {
		g_XInputPollRemaining = XINPUT_POLL_INTERVAL;
		for (DWORD user = 0; user < XUSER_MAX_COUNT; ++user) {
			bool assigned = false;
			for (const PAD_DEVICE& pad : g_Pads) if (pad.XInputIndex == (int)user) assigned = true;
			if (assigned) continue;
			XINPUT_STATE input{};
			if (XInputGetState(user, &input) != ERROR_SUCCESS) continue;
			for (PAD_DEVICE& pad : g_Pads) {
				if (pad.Model != PAD_MODEL_NONE) continue;
				pad.Model = PAD_MODEL_XINPUT;
				pad.XInputIndex = (int)user;
				const XINPUT_GAMEPAD& value = input.Gamepad;
				PadCalcParseXInput(value.wButtons, value.bLeftTrigger, value.bRightTrigger,
					value.sThumbLX, value.sThumbLY, value.sThumbRX, value.sThumbRY, pad.Received);
				break;
			}
		}
	}
	for (PAD_DEVICE& pad : g_Pads) {
		if (!active) pad.Rumble = {}; // 戻ってきたときに古い振動を再開しない
		else PadCalcUpdateRumble(pad.Rumble, GameTimeGetDeltaTime());
		SendOutput(pad);
		if (pad.Model == PAD_MODEL_NONE || !active) {
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
	if (pad.Model == PAD_MODEL_SWITCH_PRO && !pad.OutputAttempted) OpenOutputDevice(pad);
	const auto parseReport = pad.Model == PAD_MODEL_DUALSENSE
		? PadCalcParseDualSenseUsbReport : pad.Model == PAD_MODEL_SWITCH2_PRO
		? PadCalcParseSwitch2ProReport : pad.Model == PAD_MODEL_SWITCH_PRO
		? PadCalcParseSwitchProReport : PadCalcParseDs4UsbReport;
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
	return IsValidPad(pad) && g_Pads[pad].Model != PAD_MODEL_NONE;
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
	if (!g_RumbleEnabled || (pad.Model != PAD_MODEL_XINPUT && pad.OutputDevice == INVALID_HANDLE_VALUE)
		|| pad.Model == PAD_MODEL_SWITCH_PRO || g_Window == NULL || GetForegroundWindow() != g_Window) {
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
