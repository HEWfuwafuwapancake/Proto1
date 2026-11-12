#include "core/debugwindow.h"
#include <string>
#include <algorithm>

// 調整用の値（枠を含む幅・高さと、モニター端からの余白。ピクセル）
static constexpr int WINDOW_WIDTH = 720;
static constexpr int WINDOW_HEIGHT = 600;
static constexpr int WINDOW_MARGIN = 20;
static constexpr wchar_t WINDOW_CLASS[] = L"Proto1DebugWindow";
static constexpr int CHOICE_ROWS = 8;
static constexpr int CHOICE_ID = 100;
static constexpr int RESET_ID = 200;
static constexpr int ROW_HEIGHT = 32;
static constexpr int CONTROL_MARGIN = 8;
static constexpr int LABEL_WIDTH = 280;
static constexpr int RESET_WIDTH = 180;
static constexpr int DROPDOWN_HEIGHT = 240;

static HWND g_Window = nullptr;
static HWND g_Text = nullptr;
static bool g_ClassRegistered = false;
static HWND g_Labels[CHOICE_ROWS] = {};
static HWND g_Choices[CHOICE_ROWS] = {};
static HWND g_ResetButton = nullptr;
static int g_ChoiceCounts[CHOICE_ROWS] = {};
static bool g_Changed[CHOICE_ROWS] = {};
static bool g_ResetRequested = false;
static bool g_ChoicesEnabled = true;

static std::wstring ToWide(const char* text)
{
	if (text == nullptr) return {};
	const int length = MultiByteToWideChar(CP_ACP, 0, text, -1, nullptr, 0);
	if (length <= 0) return {};
	std::wstring wide(length, L'\0');
	MultiByteToWideChar(CP_ACP, 0, text, -1, &wide[0], length);
	return wide;
}

static void Layout(HWND window)
{
	RECT client{};
	GetClientRect(window, &client);
	int rows = 0;
	for (int count : g_ChoiceCounts) if (count > 0) ++rows;
	const int width = client.right;
	const int textHeight = (std::max)(0, static_cast<int>(client.bottom) - ROW_HEIGHT * (rows + 1) - CONTROL_MARGIN * 2);
	if (g_Text) MoveWindow(g_Text, 0, 0, width, textHeight, TRUE);
	const int labelWidth = (std::min)(LABEL_WIDTH, width / 2);
	const int choiceWidth = (std::max)(0, width - labelWidth - CONTROL_MARGIN * 3);
	int y = textHeight + CONTROL_MARGIN;
	for (int row = 0; row < CHOICE_ROWS; ++row) {
		if (g_ChoiceCounts[row] == 0) continue;
		MoveWindow(g_Labels[row], CONTROL_MARGIN, y + 4, labelWidth, ROW_HEIGHT - 4, TRUE);
		MoveWindow(g_Choices[row], labelWidth + CONTROL_MARGIN * 2, y, choiceWidth, DROPDOWN_HEIGHT, TRUE);
		y += ROW_HEIGHT;
	}
	if (g_ResetButton) MoveWindow(g_ResetButton, CONTROL_MARGIN, y, RESET_WIDTH, ROW_HEIGHT - 4, TRUE);
}

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
		for (int row = 0; row < CHOICE_ROWS; ++row) {
			g_Labels[row] = CreateWindowExW(0, L"STATIC", L"", WS_CHILD,
				0, 0, 0, 0, window, nullptr, GetModuleHandle(nullptr), nullptr);
			g_Choices[row] = CreateWindowExW(0, L"COMBOBOX", L"", WS_CHILD | WS_TABSTOP | CBS_DROPDOWNLIST | WS_VSCROLL,
				0, 0, 0, 0, window, reinterpret_cast<HMENU>(static_cast<INT_PTR>(CHOICE_ID + row)), GetModuleHandle(nullptr), nullptr);
			if (!g_Labels[row] || !g_Choices[row]) return -1;
			SendMessageW(g_Labels[row], WM_SETFONT, reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)), TRUE);
			SendMessageW(g_Choices[row], WM_SETFONT, reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)), TRUE);
		}
		g_ResetButton = CreateWindowExW(0, L"BUTTON", L"ふつうに戻す", WS_CHILD | WS_TABSTOP | BS_PUSHBUTTON,
			0, 0, 0, 0, window, reinterpret_cast<HMENU>(static_cast<INT_PTR>(RESET_ID)), GetModuleHandle(nullptr), nullptr);
		if (!g_ResetButton) return -1;
		SendMessageW(g_ResetButton, WM_SETFONT, reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)), TRUE);
		return 0;
	case WM_SIZE:
		Layout(window);
		return 0;
	case WM_COMMAND:
		if (LOWORD(wParam) >= CHOICE_ID && LOWORD(wParam) < CHOICE_ID + CHOICE_ROWS && HIWORD(wParam) == CBN_SELCHANGE) {
			g_Changed[LOWORD(wParam) - CHOICE_ID] = true;
		}
		if (LOWORD(wParam) == RESET_ID && HIWORD(wParam) == BN_CLICKED) g_ResetRequested = true;
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
		g_ResetButton = nullptr;
		for (int row = 0; row < CHOICE_ROWS; ++row) {
			g_Labels[row] = g_Choices[row] = nullptr;
			g_ChoiceCounts[row] = 0;
			g_Changed[row] = false;
		}
		g_ResetRequested = false;
		g_ChoicesEnabled = true;
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
	// 2枚以上なら別のモニターにあるのでゲームのすぐ後ろへ回す。1枚なら最大化したゲームに隠れないよう最前面に出す（No.83）。
	SetWindowPos(g_Window, placement.Found ? gameWindow : HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
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
	SetWindowTextW(g_Text, ToWide(text).c_str());
}

void DebugWindowSetChoices(int row, const char* label, const char* const* options, int count)
{
	if (!g_Window || row < 0 || row >= CHOICE_ROWS || !options || count <= 0) return;
	SetWindowTextW(g_Labels[row], ToWide(label).c_str());
	SendMessageW(g_Choices[row], CB_RESETCONTENT, 0, 0);
	for (int i = 0; i < count; ++i) {
		const std::wstring wide = ToWide(options[i]);
		SendMessageW(g_Choices[row], CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(wide.c_str()));
	}
	g_ChoiceCounts[row] = count;
	g_Changed[row] = false;
	ShowWindow(g_Labels[row], SW_SHOW);
	ShowWindow(g_Choices[row], SW_SHOW);
	ShowWindow(g_ResetButton, SW_SHOW);
	EnableWindow(g_Choices[row], g_ChoicesEnabled);
	Layout(g_Window);
}

void DebugWindowSetSelection(int row, int index)
{
	if (!g_Window || row < 0 || row >= CHOICE_ROWS || index < 0 || index >= g_ChoiceCounts[row]) return;
	if (SendMessageW(g_Choices[row], CB_GETCURSEL, 0, 0) != index) SendMessageW(g_Choices[row], CB_SETCURSEL, index, 0);
}

bool DebugWindowTakeSelection(int row, int& index)
{
	if (!g_Window || row < 0 || row >= CHOICE_ROWS || !g_Changed[row]) return false;
	g_Changed[row] = false;
	index = static_cast<int>(SendMessageW(g_Choices[row], CB_GETCURSEL, 0, 0));
	return index >= 0;
}

bool DebugWindowTakeReset()
{
	if (!g_Window) return false;
	const bool requested = g_ResetRequested;
	g_ResetRequested = false;
	return requested;
}

void DebugWindowSetChoicesEnabled(bool enabled)
{
	if (!g_Window || g_ChoicesEnabled == enabled) return;
	g_ChoicesEnabled = enabled;
	for (int row = 0; row < CHOICE_ROWS; ++row) {
		EnableWindow(g_Labels[row], enabled);
		EnableWindow(g_Choices[row], enabled);
	}
	EnableWindow(g_ResetButton, enabled);
}
