#include "input/pad.h"

#ifdef _DEBUG
#include <iostream>
#include "core/debug_ostream.h"
#endif


//------------------------------------------------------------------------------
//	調整用の値
//------------------------------------------------------------------------------
static constexpr float STICK_DEAD_ZONE = 0.12f;	// スティックの遊び（手を離したときのずれを 0 にする大きさ。0〜1）

// DUALSHOCK 4 の見分け方（USB の VID と PID）
static constexpr DWORD DS4_VENDOR_ID = 0x054C;		// ソニー
static constexpr DWORD DS4_PRODUCT_ID_V1 = 0x05C4;	// 初代（CUH-ZCT1）
static constexpr DWORD DS4_PRODUCT_ID_V2 = 0x09CC;	// 2代目（CUH-ZCT2）

static constexpr UINT RAW_INPUT_BUFFER_SIZE = 1024;	// WM_INPUT のデータを受け取る大きさ（DS4 の USB は 64 バイト＋ヘッダー）


static HANDLE g_Device = NULL;			// 読んでいる DUALSHOCK 4（つないでいないときは NULL）
static PAD_STATE g_Received = {};		// WM_INPUT で最後に届いた状態
static PAD_STATE g_State = {};			// このフレームの状態（遊びを取り除いたもの）


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
	g_Device = NULL;
	g_Received = NeutralState();
	g_State = NeutralState();
}

void PadUpdate(void)
{
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
	PadCalcParseDs4UsbReport(last, (int)reportSize, g_Received);
}

void PadOnDeviceChange(WPARAM wParam, LPARAM lParam)
{
	// 読んでいる DUALSHOCK 4 が外れたら、何も押していない状態に戻して、次につないだものを読めるようにする
	if (wParam == GIDC_REMOVAL && (HANDLE)lParam == g_Device) {
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
