#include "core/debugwindow.h"
#include <string>
#include <algorithm>
#include <commctrl.h>

#pragma comment(lib, "comctl32.lib")

// 調整用の値（枠を含む幅・高さと、モニター端からの余白。ピクセル）
static constexpr int WINDOW_WIDTH = 760;
static constexpr int WINDOW_HEIGHT = 760;
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
static constexpr int METER_ROWS = 8;
static constexpr int METER_ID = 300;
static constexpr int METER_MAX = 100;
static constexpr int METER_VALUE_WIDTH = 96;
static constexpr int TABLE_ROWS = 8;
static constexpr int TABLE_COLUMNS = 6;
static constexpr int TABLE_ID = 400;
static constexpr int TABLE_LABEL_WIDTH = 110; // 表の腕の名前の列の幅
static constexpr int TABLE_CELL_WIDTH = 72; // 表の数字の欄の幅
static constexpr int TABLE_NOTE_MIN_WIDTH = 200; // 表の右端の説明の列の最小の幅
static constexpr int TEXT_MIN_HEIGHT = 160; // 情報の文字の欄の最小の高さ（これより狭ければスクロールする）
static constexpr int CONTENT_MIN_WIDTH = TABLE_LABEL_WIDTH + (TABLE_CELL_WIDTH + CONTROL_MARGIN) * (TABLE_COLUMNS - 1)
	+ TABLE_NOTE_MIN_WIDTH + CONTROL_MARGIN * 3; // これより狭ければ横にスクロールする
static constexpr int WHEEL_LINES = 3; // ホイール1目盛りでスクロールする行の数

static HWND g_Window = nullptr;
static HWND g_Text = nullptr;
static bool g_ClassRegistered = false;
static bool g_InLayout = false;
static int g_ScrollX = 0;
static int g_ScrollY = 0;
static HWND g_Labels[CHOICE_ROWS] = {};
static HWND g_Choices[CHOICE_ROWS] = {};
static HWND g_ResetButton = nullptr;
static int g_ChoiceCounts[CHOICE_ROWS] = {};
static bool g_Changed[CHOICE_ROWS] = {};
static bool g_ResetRequested = false;
static bool g_ChoicesEnabled = true;
static HWND g_MeterHeading = nullptr;
static HWND g_Meters[METER_ROWS] = {};
static HWND g_MeterLabels[METER_ROWS] = {};
static HWND g_MeterNumbers[METER_ROWS] = {};
static bool g_MeterUsed[METER_ROWS] = {};
static bool g_MeterChanged[METER_ROWS] = {};
static bool g_MeterDragging[METER_ROWS] = {};
static int g_MeterValues[METER_ROWS] = {};
static int g_MeterShownValues[METER_ROWS] = {};
static std::wstring g_MeterNames[METER_ROWS];
static HWND g_TableHeading = nullptr;
static HWND g_TableHeaders[TABLE_COLUMNS + 1] = {}; // 数字の列の見出しと、右端の説明の列の見出し
static HWND g_TableLabels[TABLE_ROWS] = {};
static HWND g_TableNotes[TABLE_ROWS] = {};
static HWND g_TableCells[TABLE_ROWS][TABLE_COLUMNS] = {};
static HWND g_TableDashes[TABLE_ROWS][TABLE_COLUMNS] = {};
static int g_TableRowCount = 0;
static int g_TableColumnCount = 0;
static bool g_TableUsed[TABLE_ROWS][TABLE_COLUMNS] = {};
static bool g_TableChanged[TABLE_ROWS][TABLE_COLUMNS] = {};
static int g_TableValues[TABLE_ROWS][TABLE_COLUMNS] = {};
static int g_TableShownValues[TABLE_ROWS][TABLE_COLUMNS] = {};
static bool g_TableEnabled = true;
static bool g_SettingText = false; // ゲームから数字を書き換えている間は、ユーザーの変更として扱わない

static std::wstring ToWide(const char* text)
{
	if (text == nullptr) return {};
	const int length = MultiByteToWideChar(CP_ACP, 0, text, -1, nullptr, 0);
	if (length <= 0) return {};
	std::wstring wide(length, L'\0');
	MultiByteToWideChar(CP_ACP, 0, text, -1, &wide[0], length);
	return wide;
}

static HWND CreateChild(HWND window, const wchar_t* className, const wchar_t* text, DWORD style, int id = 0, DWORD exStyle = 0)
{
	HWND child = CreateWindowExW(exStyle, className, text, WS_CHILD | style, 0, 0, 0, 0, window,
		reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), GetModuleHandle(nullptr), nullptr);
	if (child) SendMessageW(child, WM_SETFONT, reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)), TRUE);
	return child;
}

static int CountChoiceRows()
{
	int rows = 0;
	for (int count : g_ChoiceCounts) if (count > 0) ++rows;
	return rows;
}

static int CountMeterRows()
{
	int rows = 0;
	for (bool used : g_MeterUsed) if (used) ++rows;
	return rows;
}

// 情報の文字の欄を除いた、部品の並びの高さ。
static int GetFixedHeight()
{
	const int meters = CountMeterRows();
	int height = CONTROL_MARGIN * 2 + ROW_HEIGHT * (CountChoiceRows() + 1);
	if (meters > 0) height += ROW_HEIGHT * (meters + 1);
	if (g_TableRowCount > 0) height += ROW_HEIGHT * (g_TableRowCount + 2);
	return height;
}

// スクロールバーの範囲を、中身の大きさと見えている大きさに合わせる。位置ははみ出さないように収める。
static void UpdateScrollBar(HWND window, int bar, int content, int page, int& position)
{
	position = (std::max)(0, (std::min)(position, content - page));
	SCROLLINFO info{ sizeof(SCROLLINFO), SIF_RANGE | SIF_PAGE | SIF_POS | SIF_DISABLENOSCROLL };
	info.nMin = 0;
	info.nMax = content - 1;
	info.nPage = static_cast<UINT>((std::max)(0, page));
	info.nPos = position;
	SetScrollInfo(window, bar, &info, TRUE);
}

static void Layout(HWND window)
{
	if (g_InLayout || g_Text == nullptr) return;
	g_InLayout = true;
	// スクロールバーが出ると見える幅・高さが変わるので、2回合わせる。
	RECT client{};
	int contentWidth = 0, contentHeight = 0, textHeight = 0;
	for (int pass = 0; pass < 2; ++pass) {
		GetClientRect(window, &client);
		const int fixed = GetFixedHeight();
		textHeight = (std::max)(TEXT_MIN_HEIGHT, static_cast<int>(client.bottom) - fixed);
		contentHeight = textHeight + fixed;
		contentWidth = (std::max)(CONTENT_MIN_WIDTH, static_cast<int>(client.right));
		UpdateScrollBar(window, SB_VERT, contentHeight, client.bottom, g_ScrollY);
		UpdateScrollBar(window, SB_HORZ, contentWidth, client.right, g_ScrollX);
	}
	const int left = -g_ScrollX;
	const int width = contentWidth;
	int y = -g_ScrollY;
	MoveWindow(g_Text, left, y, width, textHeight, FALSE);
	y += textHeight + CONTROL_MARGIN;
	const int labelWidth = (std::min)(LABEL_WIDTH, width / 2);
	const int choiceWidth = (std::max)(0, width - labelWidth - CONTROL_MARGIN * 3);
	for (int row = 0; row < CHOICE_ROWS; ++row) {
		if (g_ChoiceCounts[row] == 0) continue;
		MoveWindow(g_Labels[row], left + CONTROL_MARGIN, y + 4, labelWidth, ROW_HEIGHT - 4, FALSE);
		MoveWindow(g_Choices[row], left + labelWidth + CONTROL_MARGIN * 2, y, choiceWidth, DROPDOWN_HEIGHT, FALSE);
		y += ROW_HEIGHT;
	}
	MoveWindow(g_ResetButton, left + CONTROL_MARGIN, y, RESET_WIDTH, ROW_HEIGHT - 4, FALSE);
	y += ROW_HEIGHT;
	if (CountMeterRows() > 0) {
		MoveWindow(g_MeterHeading, left + CONTROL_MARGIN, y + 4, width - CONTROL_MARGIN * 2, ROW_HEIGHT - 4, FALSE);
		y += ROW_HEIGHT;
		const int meterWidth = (std::max)(0, choiceWidth - METER_VALUE_WIDTH - CONTROL_MARGIN);
		for (int row = 0; row < METER_ROWS; ++row) {
			if (!g_MeterUsed[row]) continue;
			MoveWindow(g_MeterLabels[row], left + CONTROL_MARGIN, y + 4, labelWidth, ROW_HEIGHT - 4, FALSE);
			MoveWindow(g_Meters[row], left + labelWidth + CONTROL_MARGIN * 2, y, meterWidth, ROW_HEIGHT - 4, FALSE);
			MoveWindow(g_MeterNumbers[row], left + labelWidth + CONTROL_MARGIN * 3 + meterWidth, y + 4,
				METER_VALUE_WIDTH, ROW_HEIGHT - 4, FALSE);
			y += ROW_HEIGHT;
		}
	}
	if (g_TableRowCount > 0) {
		MoveWindow(g_TableHeading, left + CONTROL_MARGIN, y + 4, width - CONTROL_MARGIN * 2, ROW_HEIGHT - 4, FALSE);
		y += ROW_HEIGHT;
		const int cellLeft = left + CONTROL_MARGIN + TABLE_LABEL_WIDTH;
		const int noteLeft = cellLeft + (TABLE_CELL_WIDTH + CONTROL_MARGIN) * g_TableColumnCount;
		const int noteWidth = (std::max)(0, left + width - CONTROL_MARGIN - noteLeft);
		for (int column = 0; column < g_TableColumnCount; ++column) {
			MoveWindow(g_TableHeaders[column], cellLeft + (TABLE_CELL_WIDTH + CONTROL_MARGIN) * column, y + 4,
				TABLE_CELL_WIDTH, ROW_HEIGHT - 4, FALSE);
		}
		MoveWindow(g_TableHeaders[TABLE_COLUMNS], noteLeft, y + 4, noteWidth, ROW_HEIGHT - 4, FALSE);
		y += ROW_HEIGHT;
		for (int row = 0; row < g_TableRowCount; ++row) {
			MoveWindow(g_TableLabels[row], left + CONTROL_MARGIN, y + 4, TABLE_LABEL_WIDTH - CONTROL_MARGIN, ROW_HEIGHT - 4, FALSE);
			for (int column = 0; column < g_TableColumnCount; ++column) {
				const int x = cellLeft + (TABLE_CELL_WIDTH + CONTROL_MARGIN) * column;
				MoveWindow(g_TableCells[row][column], x, y + 2, TABLE_CELL_WIDTH, ROW_HEIGHT - 6, FALSE);
				MoveWindow(g_TableDashes[row][column], x, y + 4, TABLE_CELL_WIDTH, ROW_HEIGHT - 4, FALSE);
			}
			MoveWindow(g_TableNotes[row], noteLeft, y + 4, noteWidth, ROW_HEIGHT - 4, FALSE);
			y += ROW_HEIGHT;
		}
	}
	// 動かした部品の古い絵が残って文字が重なって見えないよう、全体を描き直す。
	RedrawWindow(window, nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN | RDW_FRAME);
	g_InLayout = false;
}

// スクロールバーの操作から新しい位置を決める。
static void Scroll(HWND window, int bar, int request, int& position)
{
	SCROLLINFO info{ sizeof(SCROLLINFO), SIF_ALL };
	GetScrollInfo(window, bar, &info);
	int next = position;
	switch (request) {
	case SB_LINEUP: next -= ROW_HEIGHT; break;
	case SB_LINEDOWN: next += ROW_HEIGHT; break;
	case SB_PAGEUP: next -= static_cast<int>(info.nPage); break;
	case SB_PAGEDOWN: next += static_cast<int>(info.nPage); break;
	case SB_THUMBTRACK:
	case SB_THUMBPOSITION: next = info.nTrackPos; break;
	case SB_TOP: next = 0; break;
	case SB_BOTTOM: next = info.nMax; break;
	default: return;
	}
	if (next == position) return;
	position = next;
	Layout(window);
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

static bool CreateControls(HWND window)
{
	g_Text = CreateChild(window, L"EDIT", L"", WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL, 0, WS_EX_CLIENTEDGE);
	if (g_Text == nullptr) return false;
	for (int row = 0; row < CHOICE_ROWS; ++row) {
		g_Labels[row] = CreateChild(window, L"STATIC", L"", 0);
		g_Choices[row] = CreateChild(window, L"COMBOBOX", L"", WS_TABSTOP | CBS_DROPDOWNLIST | WS_VSCROLL, CHOICE_ID + row);
		if (!g_Labels[row] || !g_Choices[row]) return false;
	}
	g_ResetButton = CreateChild(window, L"BUTTON", L"ふつうに戻す", WS_TABSTOP | BS_PUSHBUTTON, RESET_ID);
	g_MeterHeading = CreateChild(window, L"STATIC", L"1P の腕の耐久（装備している腕だけ動かせる。オンライン中は動かせない）", 0);
	g_TableHeading = CreateChild(window, L"STATIC",
		L"耐久の減る量（当たり：1人でも効いた　外れ：誰にも効かなかった。オフラインだけ書き換えられ、すぐ効く）", 0);
	if (!g_ResetButton || !g_MeterHeading || !g_TableHeading) return false;
	for (int row = 0; row < METER_ROWS; ++row) {
		g_MeterLabels[row] = CreateChild(window, L"STATIC", L"", 0);
		g_Meters[row] = CreateChild(window, TRACKBAR_CLASSW, L"", WS_TABSTOP | TBS_HORZ | TBS_NOTICKS, METER_ID + row);
		g_MeterNumbers[row] = CreateChild(window, L"STATIC", L"", 0);
		if (!g_MeterLabels[row] || !g_Meters[row] || !g_MeterNumbers[row]) return false;
		SendMessageW(g_Meters[row], TBM_SETRANGE, TRUE, MAKELPARAM(0, METER_MAX));
		EnableWindow(g_Meters[row], FALSE);
		g_MeterShownValues[row] = -3;
	}
	for (int column = 0; column <= TABLE_COLUMNS; ++column) {
		g_TableHeaders[column] = CreateChild(window, L"STATIC", L"", SS_CENTER);
		if (!g_TableHeaders[column]) return false;
	}
	SetWindowTextW(g_TableHeaders[TABLE_COLUMNS], L"そのほかの内容");
	for (int row = 0; row < TABLE_ROWS; ++row) {
		g_TableLabels[row] = CreateChild(window, L"STATIC", L"", 0);
		g_TableNotes[row] = CreateChild(window, L"STATIC", L"", 0);
		if (!g_TableLabels[row] || !g_TableNotes[row]) return false;
		for (int column = 0; column < TABLE_COLUMNS; ++column) {
			g_TableCells[row][column] = CreateChild(window, L"EDIT", L"", WS_TABSTOP | ES_NUMBER | ES_CENTER,
				TABLE_ID + row * TABLE_COLUMNS + column, WS_EX_CLIENTEDGE);
			g_TableDashes[row][column] = CreateChild(window, L"STATIC", L"―", SS_CENTER);
			if (!g_TableCells[row][column] || !g_TableDashes[row][column]) return false;
			SendMessageW(g_TableCells[row][column], EM_SETLIMITTEXT, 3, 0);
			g_TableShownValues[row][column] = -2;
		}
	}
	return true;
}

static void ClearState()
{
	g_Text = nullptr;
	g_ResetButton = nullptr;
	g_MeterHeading = nullptr;
	g_TableHeading = nullptr;
	g_ScrollX = g_ScrollY = 0;
	for (int row = 0; row < CHOICE_ROWS; ++row) {
		g_Labels[row] = g_Choices[row] = nullptr;
		g_ChoiceCounts[row] = 0;
		g_Changed[row] = false;
	}
	g_ResetRequested = false;
	g_ChoicesEnabled = true;
	for (int row = 0; row < METER_ROWS; ++row) {
		g_Meters[row] = g_MeterLabels[row] = g_MeterNumbers[row] = nullptr;
		g_MeterUsed[row] = g_MeterChanged[row] = g_MeterDragging[row] = false;
		g_MeterValues[row] = 0;
		g_MeterShownValues[row] = -3;
		g_MeterNames[row].clear();
	}
	for (HWND& header : g_TableHeaders) header = nullptr;
	for (int row = 0; row < TABLE_ROWS; ++row) {
		g_TableLabels[row] = g_TableNotes[row] = nullptr;
		for (int column = 0; column < TABLE_COLUMNS; ++column) {
			g_TableCells[row][column] = g_TableDashes[row][column] = nullptr;
			g_TableUsed[row][column] = g_TableChanged[row][column] = false;
			g_TableValues[row][column] = 0;
			g_TableShownValues[row][column] = -2;
		}
	}
	g_TableRowCount = g_TableColumnCount = 0;
	g_TableEnabled = true;
}

static void OnTableEdit(int id)
{
	const int index = id - TABLE_ID;
	const int row = index / TABLE_COLUMNS;
	const int column = index % TABLE_COLUMNS;
	if (row < 0 || row >= g_TableRowCount || column >= g_TableColumnCount || !g_TableUsed[row][column]) return;
	wchar_t text[8] = {};
	GetWindowTextW(g_TableCells[row][column], text, 8);
	// 消して空にした途中は、まだ変えない。
	if (text[0] == L'\0') return;
	g_TableValues[row][column] = _wtoi(text);
	g_TableChanged[row][column] = true;
}

static LRESULT CALLBACK DebugWindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) {
	case WM_CREATE:
		return CreateControls(window) ? 0 : -1;
	case WM_SIZE:
		Layout(window);
		return 0;
	case WM_COMMAND: {
		const int id = LOWORD(wParam);
		if (id >= CHOICE_ID && id < CHOICE_ID + CHOICE_ROWS && HIWORD(wParam) == CBN_SELCHANGE) g_Changed[id - CHOICE_ID] = true;
		if (id == RESET_ID && HIWORD(wParam) == BN_CLICKED) g_ResetRequested = true;
		if (id >= TABLE_ID && id < TABLE_ID + TABLE_ROWS * TABLE_COLUMNS && HIWORD(wParam) == EN_CHANGE && !g_SettingText) OnTableEdit(id);
		return 0;
	}
	case WM_HSCROLL:
		// lParam が 0 ならウィンドウの横スクロールバー、それ以外はメーターのつまみ。
		if (lParam == 0) {
			Scroll(window, SB_HORZ, LOWORD(wParam), g_ScrollX);
			return 0;
		}
		for (int row = 0; row < METER_ROWS; ++row) {
			if (reinterpret_cast<HWND>(lParam) != g_Meters[row] || !IsWindowEnabled(g_Meters[row])) continue;
			g_MeterDragging[row] = LOWORD(wParam) == TB_THUMBTRACK;
			g_MeterValues[row] = static_cast<int>(SendMessageW(g_Meters[row], TBM_GETPOS, 0, 0));
			g_MeterChanged[row] = true;
		}
		return 0;
	case WM_VSCROLL:
		if (lParam == 0) Scroll(window, SB_VERT, LOWORD(wParam), g_ScrollY);
		return 0;
	case WM_MOUSEWHEEL: {
		const int notches = GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA;
		const int before = g_ScrollY;
		g_ScrollY -= notches * WHEEL_LINES * ROW_HEIGHT;
		g_ScrollY = (std::max)(0, g_ScrollY);
		if (g_ScrollY != before) Layout(window);
		return 0;
	}
	case WM_SETFOCUS:
		if (g_Text != nullptr) SetFocus(g_Text);
		return 0;
	case WM_CLOSE:
		// ×は情報の表示だけを閉じる。ゲームのメッセージループは止めない。
		ShowWindow(window, SW_HIDE);
		return 0;
	case WM_DESTROY:
		ClearState();
		return 0;
	default:
		return DefWindowProcW(window, message, wParam, lParam);
	}
}

bool DebugWindowInitialize(HWND gameWindow)
{
	DebugWindowFinalize();
	INITCOMMONCONTROLSEX controls{ sizeof(INITCOMMONCONTROLSEX), ICC_BAR_CLASSES };
	if (!InitCommonControlsEx(&controls)) return false;
	const HINSTANCE instance = GetModuleHandle(nullptr);
	WNDCLASSEXW windowClass{};
	windowClass.cbSize = sizeof(windowClass);
	windowClass.lpfnWndProc = DebugWindowProc;
	windowClass.hInstance = instance;
	windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
	windowClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
	windowClass.lpszClassName = WINDOW_CLASS;
	// 大きさを変えたら全体を描き直す（部品の古い絵が残らないように）。
	windowClass.style = CS_HREDRAW | CS_VREDRAW;
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
	// 2枚以上なら別のモニターの左上、1枚ならゲームと同じモニターの右上。作業領域より高ければ収める（残りはスクロール）。
	const RECT& work = placement.Work;
	const int height = (std::min)(WINDOW_HEIGHT, static_cast<int>(work.bottom - work.top) - WINDOW_MARGIN * 2);
	const int x = placement.Found ? work.left + WINDOW_MARGIN : (int)work.right - WINDOW_WIDTH - WINDOW_MARGIN;
	g_Window = CreateWindowExW(0, WINDOW_CLASS, L"Proto1 Debug", WS_OVERLAPPEDWINDOW | WS_VSCROLL | WS_HSCROLL | WS_CLIPCHILDREN,
		x, work.top + WINDOW_MARGIN, WINDOW_WIDTH, height, nullptr, nullptr, instance, nullptr);
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

void DebugWindowSetMeter(int row, const char* label, int value, bool enabled)
{
	if (!g_Window || row < 0 || row >= METER_ROWS) return;
	if (!g_MeterUsed[row]) {
		g_MeterUsed[row] = true;
		ShowWindow(g_MeterHeading, SW_SHOW);
		ShowWindow(g_MeterLabels[row], SW_SHOW);
		ShowWindow(g_Meters[row], SW_SHOW);
		ShowWindow(g_MeterNumbers[row], SW_SHOW);
		Layout(g_Window);
	}
	const std::wstring name = ToWide(label);
	if (g_MeterNames[row] != name) {
		g_MeterNames[row] = name;
		SetWindowTextW(g_MeterLabels[row], name.c_str());
	}
	if (!enabled) {
		g_MeterChanged[row] = false;
		g_MeterDragging[row] = false;
	}
	if ((IsWindowEnabled(g_Meters[row]) != FALSE) != enabled) {
		EnableWindow(g_Meters[row], enabled);
		EnableWindow(g_MeterLabels[row], enabled);
		EnableWindow(g_MeterNumbers[row], enabled);
	}
	const bool preserve = g_MeterDragging[row] || g_MeterChanged[row];
	const int position = static_cast<int>(SendMessageW(g_Meters[row], TBM_GETPOS, 0, 0));
	const int requested = value == -1 ? METER_MAX : (std::max)(0, (std::min)(METER_MAX, value));
	if (!preserve && position != requested) SendMessageW(g_Meters[row], TBM_SETPOS, TRUE, requested);
	const int shown = preserve ? position : value;
	if (g_MeterShownValues[row] != shown) {
		g_MeterShownValues[row] = shown;
		const std::wstring number = shown == -1 ? L"無制限" : (shown < -1 ? L"装備なし"
			: std::to_wstring((std::max)(0, (std::min)(METER_MAX, shown))));
		SetWindowTextW(g_MeterNumbers[row], number.c_str());
	}
}

bool DebugWindowTakeMeter(int row, int& value)
{
	if (!g_Window || row < 0 || row >= METER_ROWS || !g_MeterChanged[row]) return false;
	g_MeterChanged[row] = false;
	value = g_MeterValues[row];
	return true;
}

void DebugWindowSetTable(const char* const* columns, int columnCount, const char* const* rows, const char* const* notes, int rowCount)
{
	if (!g_Window || !columns || !rows || !notes) return;
	g_TableColumnCount = (std::max)(0, (std::min)(TABLE_COLUMNS, columnCount));
	g_TableRowCount = (std::max)(0, (std::min)(TABLE_ROWS, rowCount));
	ShowWindow(g_TableHeading, g_TableRowCount > 0 ? SW_SHOW : SW_HIDE);
	for (int column = 0; column < TABLE_COLUMNS; ++column) {
		const bool shown = column < g_TableColumnCount && g_TableRowCount > 0;
		if (shown) SetWindowTextW(g_TableHeaders[column], ToWide(columns[column]).c_str());
		ShowWindow(g_TableHeaders[column], shown ? SW_SHOW : SW_HIDE);
	}
	ShowWindow(g_TableHeaders[TABLE_COLUMNS], g_TableRowCount > 0 ? SW_SHOW : SW_HIDE);
	for (int row = 0; row < TABLE_ROWS; ++row) {
		const bool shown = row < g_TableRowCount;
		if (shown) {
			SetWindowTextW(g_TableLabels[row], ToWide(rows[row]).c_str());
			SetWindowTextW(g_TableNotes[row], ToWide(notes[row]).c_str());
		}
		ShowWindow(g_TableLabels[row], shown ? SW_SHOW : SW_HIDE);
		ShowWindow(g_TableNotes[row], shown ? SW_SHOW : SW_HIDE);
		for (int column = 0; column < TABLE_COLUMNS; ++column) {
			// 使う欄かどうかは DebugWindowSetTableValue で決まるまで隠しておく。
			g_TableUsed[row][column] = false;
			g_TableChanged[row][column] = false;
			g_TableShownValues[row][column] = -2;
			ShowWindow(g_TableCells[row][column], SW_HIDE);
			ShowWindow(g_TableDashes[row][column], shown && column < g_TableColumnCount ? SW_SHOW : SW_HIDE);
		}
	}
	Layout(g_Window);
}

void DebugWindowSetTableValue(int row, int column, int value, bool enabled)
{
	if (!g_Window || row < 0 || row >= g_TableRowCount || column < 0 || column >= g_TableColumnCount) return;
	HWND cell = g_TableCells[row][column];
	const bool used = value >= 0;
	if (g_TableUsed[row][column] != used) {
		g_TableUsed[row][column] = used;
		ShowWindow(cell, used ? SW_SHOW : SW_HIDE);
		ShowWindow(g_TableDashes[row][column], used ? SW_HIDE : SW_SHOW);
	}
	if (!used) return;
	if ((IsWindowEnabled(cell) != FALSE) != enabled) EnableWindow(cell, enabled);
	if (!enabled) g_TableChanged[row][column] = false;
	// 打っている途中（フォーカスがある間）と、まだ読まれていない変更は上書きしない。
	if (GetFocus() == cell || g_TableChanged[row][column] || g_TableShownValues[row][column] == value) return;
	g_TableShownValues[row][column] = value;
	g_SettingText = true;
	SetWindowTextW(cell, std::to_wstring(value).c_str());
	g_SettingText = false;
}

bool DebugWindowTakeTableValue(int row, int column, int& value)
{
	if (!g_Window || row < 0 || row >= TABLE_ROWS || column < 0 || column >= TABLE_COLUMNS || !g_TableChanged[row][column]) return false;
	g_TableChanged[row][column] = false;
	value = g_TableValues[row][column];
	// 打った値をゲームが収めた値で見せ直せるよう、次の SetTableValue で書き直させる。
	g_TableShownValues[row][column] = -2;
	return true;
}
