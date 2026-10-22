#include "mouse.h"


static HWND g_hWnd = NULL;
static bool g_Locked = false;

static int g_AccumX = 0;	// WM_INPUT で届いた移動量をためておく
static int g_AccumY = 0;
static int g_DeltaX = 0;	// このフレームの移動量
static int g_DeltaY = 0;


void MouseInitialize(HWND hWnd)
{
	g_hWnd = hWnd;

	// Raw Input（マウスの生の移動量）を受け取る登録。カーソルが画面の端で止まっても移動量が届く
	RAWINPUTDEVICE rid{};
	rid.usUsagePage = 0x01;	// 汎用デスクトップ
	rid.usUsage = 0x02;		// マウス
	rid.dwFlags = 0;
	rid.hwndTarget = hWnd;
	RegisterRawInputDevices(&rid, 1, sizeof(rid));
}

void MouseFinalize(void)
{
	MouseSetLock(false);
	g_hWnd = NULL;
}

void MouseUpdate(void)
{
	// ためた移動量をこのフレームの分として取り出す
	g_DeltaX = g_AccumX;
	g_DeltaY = g_AccumY;
	g_AccumX = 0;
	g_AccumY = 0;
}

void MouseOnRawInput(LPARAM lParam)
{
	RAWINPUT raw{};
	UINT size = sizeof(raw);
	if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, &raw, &size, sizeof(RAWINPUTHEADER)) == (UINT)-1) {
		return;
	}

	// ロック中で、相対移動（ふつうのマウス）のときだけ移動量をためる
	if (g_Locked && raw.header.dwType == RIM_TYPEMOUSE && !(raw.data.mouse.usFlags & MOUSE_MOVE_ABSOLUTE)) {
		g_AccumX += raw.data.mouse.lLastX;
		g_AccumY += raw.data.mouse.lLastY;
	}
}

void MouseSetLock(bool lock)
{
	if (!g_hWnd || lock == g_Locked) {
		return;
	}
	g_Locked = lock;

	if (lock) {
		// カーソルを隠して、ウィンドウの中（クライアント領域）に閉じ込める
		RECT rect;
		GetClientRect(g_hWnd, &rect);
		POINT lt = { rect.left, rect.top };
		POINT rb = { rect.right, rect.bottom };
		ClientToScreen(g_hWnd, &lt);
		ClientToScreen(g_hWnd, &rb);
		RECT clip = { lt.x, lt.y, rb.x, rb.y };
		ClipCursor(&clip);
		ShowCursor(FALSE);
	}
	else {
		ClipCursor(NULL);
		ShowCursor(TRUE);
	}

	// 切り替えた瞬間にカメラが飛ばないよう、たまっている分は捨てる
	g_AccumX = g_AccumY = 0;
	g_DeltaX = g_DeltaY = 0;
}

bool MouseIsLocked(void)
{
	return g_Locked;
}

int MouseGetDeltaX(void)
{
	return g_DeltaX;
}

int MouseGetDeltaY(void)
{
	return g_DeltaY;
}
