#include "core/debugwindow.h"
#include <string>

// 調整用の値（枠を含む幅・高さと、モニター端からの余白。ピクセル）
static constexpr int WINDOW_WIDTH = 720;
static constexpr int WINDOW_HEIGHT = 400;
static constexpr int WINDOW_MARGIN = 20;
static constexpr wchar_t WINDOW_CLASS[] = L"Proto1DebugWindow";

static HWND g_Window = nullptr;
static HWND g_Text = nullptr;
static bool g_ClassRegistered = false;

// ゲームと違うモニターを1つ探すための情報。見つからなければゲーム側の作業領域を使う。
struct DEBUG_MONITOR
{
	HMONITOR GameMonitor;
	RECT Work;
	bool Found;
};

static BOOL CALLBACK FindOtherMonitor(HMONITOR monitor, HDC, LPRECT, LPARAM parameter)
{
	DEBUG_MONITOR& placement = *reinterpret_cast<DEBUG_MONITOR*>(parameter);
	if (monitor == placement.GameMonitor) return TRUE;
	MONITORINFO info{};
	info.cbSize = sizeof(info);
	if (!GetMonitorInfo(monitor, &info)) return TRUE;
	placement.Work = info.rcWork;
	placement.Found = true;
	return FALSE;
}

static LRESULT CALLBACK DebugWindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) {
	case WM_CREATE:
		g_Text = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
			WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL,
			0, 0, 0, 0, window, nullptr, GetModuleHandle(nullptr), nullptr);
		if (g_Text == nullptr) return -1;
		SendMessageW(g_Text, WM_SETFONT, reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)), TRUE);
		return 0;
	case WM_SIZE:
		if (g_Text != nullptr) MoveWindow(g_Text, 0, 0, LOWORD(lParam), HIWORD(lParam), TRUE);
		return 0;
	case WM_SETFOCUS:
		if (g_Text != nullptr) SetFocus(g_Text);
		return 0;
	case WM_CLOSE:
		// ×は情報の表示だけを閉じる。ゲームのメッセージループは止めない。
		ShowWindow(window, SW_HIDE);
		return 0;
	case WM_DESTROY:
		g_Text = nullptr;
		return 0;
	default:
		return DefWindowProcW(window, message, wParam, lParam);
	}
}

bool DebugWindowInitialize(HWND gameWindow)
{
	DebugWindowFinalize();
	const HINSTANCE instance = GetModuleHandle(nullptr);
	WNDCLASSEXW windowClass{};
	windowClass.cbSize = sizeof(windowClass);
	windowClass.lpfnWndProc = DebugWindowProc;
	windowClass.hInstance = instance;
	windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
	windowClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
	windowClass.lpszClassName = WINDOW_CLASS;
	if (!RegisterClassExW(&windowClass)) return false;
	g_ClassRegistered = true;

	DEBUG_MONITOR placement{};
	placement.GameMonitor = MonitorFromWindow(gameWindow, MONITOR_DEFAULTTONEAREST);
	MONITORINFO info{};
	info.cbSize = sizeof(info);
	if (!GetMonitorInfo(placement.GameMonitor, &info)) {
		DebugWindowFinalize();
		return false;
	}
	placement.Work = info.rcWork;
	EnumDisplayMonitors(nullptr, nullptr, FindOtherMonitor, reinterpret_cast<LPARAM>(&placement));
	// 2枚以上なら別のモニターの左上、1枚ならゲームと同じモニターの右上。
	const RECT& work = placement.Work;
	const int x = placement.Found ? work.left + WINDOW_MARGIN : (int)work.right - WINDOW_WIDTH - WINDOW_MARGIN;
	g_Window = CreateWindowExW(0, WINDOW_CLASS, L"Proto1 Debug", WS_OVERLAPPEDWINDOW,
		x, work.top + WINDOW_MARGIN, WINDOW_WIDTH, WINDOW_HEIGHT, nullptr, nullptr, instance, nullptr);
	if (g_Window == nullptr) {
		DebugWindowFinalize();
		return false;
	}
	ShowWindow(g_Window, SW_SHOWNOACTIVATE);
	return true;
}

void DebugWindowFinalize()
{
	if (g_Window != nullptr) DestroyWindow(g_Window);
	g_Window = nullptr;
	g_Text = nullptr;
	if (g_ClassRegistered) UnregisterClassW(WINDOW_CLASS, GetModuleHandle(nullptr));
	g_ClassRegistered = false;
}

void DebugWindowSetText(const char* text)
{
	if (g_Text == nullptr) return;
	// ゲームのマルチバイト文字列を Unicode の EDIT へ渡し、日本語も同じ内容で出す。
	const int length = MultiByteToWideChar(CP_ACP, 0, text, -1, nullptr, 0);
	if (length <= 0) return;
	std::wstring wide(length, L'\0');
	MultiByteToWideChar(CP_ACP, 0, text, -1, &wide[0], length);
	SetWindowTextW(g_Text, wide.c_str());
}
