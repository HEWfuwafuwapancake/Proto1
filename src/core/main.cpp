#include <SDKDDKVer.h>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <mmsystem.h>
#include <imm.h>
#include <stdio.h>
#include <math.h>
#include <sstream>
#include "core/debugwindow.h"
#include "core/dx11.h"
#include "core/framestats.h"
#include "core/gametime.h"
#include "core/main.h"
#include "core/shader.h"
#include "input/action.h"
#include "input/keyboard.h"
#include "input/mouse.h"
#include "input/pad.h"
#include "game/game.h"
#include "render/number2d.h"
#include "render/shape.h"
#include "render/model.h"
#include "generated/sourceid.h" // ビルドの前に tools/make-sourceid.ps1 が中間フォルダに作る（SOURCE_ID）

#pragma comment (lib, "winmm.lib")
#pragma comment (lib, "imm32.lib")


/*------------------------------------------------------------------------------
	ウィンドウ情報
------------------------------------------------------------------------------*/
static constexpr char WINDOW_CLASS[] = "Proto1"; // メインウィンドウクラス名
static constexpr char TITLE[] = "Proto1";        // タイトルバーのテキスト
// 最大化できる枠ありのウィンドウ（No.83。描くのは 1280×720 のまま、Present がウィンドウいっぱいに引き伸ばす）。
static constexpr DWORD WINDOW_STYLE = WS_OVERLAPPEDWINDOW;

// 振動をオンにしたときに知らせる短い振動（強さ 0〜1・秒）
static constexpr float RUMBLE_ON_NOTICE_STRENGTH = 0.5f;
static constexpr float RUMBLE_ON_NOTICE_SECONDS = 0.2f;


/*------------------------------------------------------------------------------
	ウィンドウプロシージャ プロトタイプ宣言
------------------------------------------------------------------------------*/
LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

static Game g_Game;

static DWORD g_Fps = 0;	// 直近1秒の FPS
static FRAME_STATS_RESULT g_FrameStats{}; // 直近1秒の最大間隔・平均処理時間・カクつき
static uint64_t g_Progress = 0; // 直近1秒に通信の入力で進んだフレーム数
static HWND g_MainWindow = nullptr;
static bool g_Fullscreen = false;
static bool g_FullscreenEnterDown = false; // Alt+Enter の Enter を決定操作へ渡さない
static RECT g_WindowRect{}; // 全画面にする直前の枠ありウィンドウの位置と大きさ

/// @brief 全画面を設定する（true：今いるモニター全体、false：保存した枠ありの位置と大きさ）
static void SetFullscreen(HWND hWnd, bool fullscreen)
{
	if (g_Fullscreen == fullscreen) return;
	if (fullscreen) {
		MONITORINFO monitor{};
		monitor.cbSize = sizeof(monitor);
		if (!GetMonitorInfo(MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST), &monitor)
			|| !GetWindowRect(hWnd, &g_WindowRect)) return;
		g_Fullscreen = true;
		SetWindowLongPtr(hWnd, GWL_STYLE, WS_POPUP | WS_VISIBLE);
		const RECT& rect = monitor.rcMonitor;
		SetWindowPos(hWnd, HWND_TOP, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, SWP_FRAMECHANGED);
	}
	else {
		g_Fullscreen = false;
		SetWindowLongPtr(hWnd, GWL_STYLE, WINDOW_STYLE | WS_VISIBLE);
		SetWindowPos(hWnd, HWND_TOP, g_WindowRect.left, g_WindowRect.top,
			g_WindowRect.right - g_WindowRect.left, g_WindowRect.bottom - g_WindowRect.top, SWP_FRAMECHANGED);
	}
	g_Game.SetFullscreen(g_Fullscreen);
}

static int GetEquipSlot()
{
	const NetSession& net = g_Game.GetNetSession();
	return net.IsOnline() ? (net.GetLocalSlot() < 0 ? 0 : net.GetLocalSlot()) : ActionGetEquipPlayer();
}

/// タイトルバーに FPS・装備・振動のオン・オフを出す（残り時間・得点は画面に出す。hud.cpp）
static void RefreshTitle(HWND hWnd)
{
	char title[1024];
	const int equipPlayer = GetEquipSlot();
	const NetSession& net = g_Game.GetNetSession();
	const int mode = ActionGetEquipMode(net.IsOnline() ? ActionGetEquipPlayer() : equipPlayer);
	const ArmManager& arms = g_Game.GetArms(equipPlayer);
	const std::string progress = net.IsOnline() ? " 進行: " + std::to_string(g_Progress) + "/s" : "";
	char modeText[96] = "";
	if (mode >= 0) {
		sprintf_s(modeText, "  [%s腕を切り替え中：%d段目 数字キー 1-9,0（Q / E で次の段）]",
			ActionCalcEquipModeHand(mode) == 0 ? "左" : "右", ActionCalcEquipModePage(mode) + 1);
	}
	sprintf_s(title, "%s%s  FPS: %lu（最大 %.0fms・処理 %.1fms・カクつき %d）%s  %dP%s 左：%s 右：%s%s  振動: %s（SHARE / V） %s", TITLE, net.GetDesyncText().c_str(),
		g_Fps, g_FrameStats.MaxIntervalMs, g_FrameStats.AverageWorkMs, g_FrameStats.HitchCount, progress.c_str(),
		equipPlayer + 1, net.IsOnline() ? "" : "（F1-F4）", ArmGetName(arms.GetType(0)), ArmGetName(arms.GetType(1)),
		modeText,
		PadIsRumbleEnabled() ? "オン" : "オフ", net.GetStatusText().c_str());
	SetWindowText(hWnd, title);
	DebugWindowSetText(title);
}

bool Initialize(HWND hWnd, const NET_OPTIONS& options);
void Finalize(void);
void Update(void);
void Draw(void);


int APIENTRY WinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPSTR lpCmdLine, _In_ int nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
#ifdef _M_X64
	// sinf などが CPU によって FMA3 の計算に切り替わると、PC ごとに結果が少しずれる（オンラインの DESYNC）。
	// どの CPU でも同じ計算にそろえる。
	_set_FMA3_enable(0);
#endif
	SessionCalcSetProgram(SOURCE_ID);
	// 起動は最大化したウィンドウ。--windowed は 1280×720 のウィンドウ、--fullscreen は枠なし全画面（No.83）。
	bool windowed = false;
	bool fullscreen = false;
	bool debugWindow = false;
#if defined(_DEBUG)
	debugWindow = true;
#endif
	// 表示だけの引数はここで取りのぞき、通信の設定へ混ぜない。
	std::istringstream arguments(lpCmdLine);
	std::string word, netArguments;
	while (arguments >> word) {
		if (word == "--windowed") windowed = true;
		else if (word == "--fullscreen") fullscreen = true;
		else if (word == "--debugwindow") debugWindow = true;
		else netArguments += word + " ";
	}
	NET_OPTIONS options;
	if (!SessionCalcParseOptions(netArguments, options)) {
		MessageBox(nullptr, "起動引数を確認してください。\n--host [--delay 0-120]\n--join <IPv4>\n--windowed：1280x720 のウィンドウで起動\n--fullscreen：枠なし全画面で起動\n--debugwindow：デバッグ情報を別ウィンドウに表示\n通信の指定なし：モード選択（表示の引数は併用できます）", TITLE, MB_OK | MB_ICONERROR);
		return -1;
	}

	int a = 1;
	int b = 2;
	int c = 0;
	c = a + b;

	/* ウィンドウクラスの登録 */
	WNDCLASSEX wcex{};
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.lpfnWndProc = WndProc;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, IDI_APPLICATION);
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = NULL;
	wcex.lpszMenuName = nullptr; // メニューは作らない
	wcex.lpszClassName = WINDOW_CLASS;
	wcex.hIconSm = LoadIcon(wcex.hInstance, IDI_APPLICATION);
	RegisterClassEx(&wcex);

	// これがスズキヨシアキの変更部分 v(^o^)v

	// クライアント領域のサイズを持った矩形 (左からleft, top, right, bottom)
	RECT windowRect = { 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT };
	// ウィンドウのスタイル
	const DWORD windowStyle = WINDOW_STYLE;
	// 指定したクライアント領域を確保するために新たな矩形座標を計算
	AdjustWindowRect(&windowRect, windowStyle, FALSE);

	// 新たなWindowの矩形座標から幅と高さを算出
	int windowWidth = windowRect.right - windowRect.left;
	int windowHeight = windowRect.bottom - windowRect.top;

	/* メインウィンドウの作成 */
	HWND hWnd = CreateWindow(WINDOW_CLASS, TITLE, windowStyle,
		CW_USEDEFAULT, CW_USEDEFAULT, windowWidth, windowHeight, nullptr, nullptr, hInstance, nullptr);
	if (hWnd == nullptr) return -1;
	g_MainWindow = hWnd;
	// 文字入力をしないゲームでは IME を外し、Q / E や数字を通常のキーとして受け取る。
	ImmAssociateContext(hWnd, nullptr);

	//kokonihenkou
	//初期化処理（失敗したら後片付けをして終了する）
	if (!Initialize(hWnd, options)) {
		Finalize();
		return -1;
	}


	SetFullscreen(hWnd, fullscreen);
	if (debugWindow && !DebugWindowInitialize(hWnd)) {
		MessageBox(hWnd, "デバッグ用ウィンドウの作成に失敗しました", TITLE, MB_OK | MB_ICONERROR);
		Finalize();
		DestroyWindow(hWnd);
		return -1;
	}
	RefreshTitle(hWnd);
	ShowWindow(hWnd, fullscreen || windowed ? nCmdShow : SW_SHOWMAXIMIZED);
	UpdateWindow(hWnd);

	// 時間計測用
	DWORD dwExecLastTime;
	DWORD dwFPSLastTime;
	DWORD dwCurrentTime;
	DWORD dwFrameCount;

	// フレームカウント初期化
	timeBeginPeriod(1);	// 分解能を設定
	dwExecLastTime = dwFPSLastTime = timeGetTime();	// システム時刻をミリ秒単位で取得
	dwCurrentTime = dwFrameCount = 0;
	uint64_t advancedLast = 0;
	FRAME_STATS frameStats{};
	LARGE_INTEGER counterFrequency{}, previousFrameStart{};
	QueryPerformanceFrequency(&counterFrequency);
	const double counterToMs = 1000.0 / static_cast<double>(counterFrequency.QuadPart);
	bool hasPreviousFrame = false;

	// 1フレームの長さ（ミリ秒）。17・17・16 を繰り返すと、3フレームで50ミリ秒＝ちょうど1秒に60回になる
	const DWORD FRAME_TIME[3] = { 17, 17, 16 };
	int frameIndex = 0;

	/* メッセージループ */
	MSG msg{};	// 最初の判定で中身が未設定にならないよう0で初期化
	do {
		// 来ているウィンドウメッセージを全部処理する
		// （1回に1つずつだと、マウスの移動（WM_INPUT）がたくさん来たときにフレームが遅れる）
		while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
			if (msg.message == WM_QUIT) {
				break;
			}
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		if (msg.message == WM_QUIT) {
			break;
		}

		dwCurrentTime = timeGetTime();				// システム時刻を取得

		if ((dwCurrentTime - dwFPSLastTime) >= 1000)	// 1秒ごとに実行
		{
			// FPS（1秒間に何回 Update・Draw したか）をタイトルバーに出す
			DWORD elapsed = dwCurrentTime - dwFPSLastTime;	// 1000ミリ秒ちょうどより少し長いので、四捨五入して表示する
			g_Fps = (dwFrameCount * 1000 + elapsed / 2) / elapsed;
			g_FrameStats = FrameStatsTake(frameStats);
			const uint64_t advanced = g_Game.GetNetSession().GetAdvancedFrames();
			g_Progress = ((advanced - advancedLast) * 1000 + elapsed / 2) / elapsed;
			advancedLast = advanced;
			RefreshTitle(hWnd);

			dwFPSLastTime = dwCurrentTime;				// FPSを測定した時刻を保存
			dwFrameCount = 0;							// カウントをクリア
		}

		if ((dwCurrentTime - dwExecLastTime) >= FRAME_TIME[frameIndex])	// 1/60秒ごとに実行
		{
			// 次のフレームの予定時刻（今の時刻ではなく予定時刻から進めるので、Sleep の寝すぎでずれていかない）
			dwExecLastTime += FRAME_TIME[frameIndex];
			frameIndex = (frameIndex + 1) % 3;
			const bool restartFrame = (dwCurrentTime - dwExecLastTime) >= 100;
			if (restartFrame) {
				dwExecLastTime = dwCurrentTime;	// 大きく遅れたとき（ウィンドウをつかんで動かしていた など）は追いつこうとせず仕切り直す
			}

			const int oldEquipPlayer = GetEquipSlot();
			const ARM_TYPE oldLeft = g_Game.GetArms(oldEquipPlayer).GetType(0);
			const ARM_TYPE oldRight = g_Game.GetArms(oldEquipPlayer).GetType(1);
			const int oldMode = ActionGetEquipMode(g_Game.GetNetSession().IsOnline() ? ActionGetEquipPlayer() : oldEquipPlayer);
			LARGE_INTEGER frameStart{};
			QueryPerformanceCounter(&frameStart);
			const float intervalMs = hasPreviousFrame && !restartFrame
				? static_cast<float>((frameStart.QuadPart - previousFrameStart.QuadPart) * counterToMs) : 0.0f;
			previousFrameStart = frameStart;
			hasPreviousFrame = true;
			Update();			// 更新処理
			const int equipPlayer = GetEquipSlot();
			if (oldLeft != g_Game.GetArms(equipPlayer).GetType(0) || oldRight != g_Game.GetArms(equipPlayer).GetType(1)
				|| oldEquipPlayer != equipPlayer
				|| oldMode != ActionGetEquipMode(g_Game.GetNetSession().IsOnline() ? ActionGetEquipPlayer() : equipPlayer)) RefreshTitle(hWnd);
			Draw();				// 描画処理
			LARGE_INTEGER workEnd{};
			QueryPerformanceCounter(&workEnd);
			const float workMs = static_cast<float>((workEnd.QuadPart - frameStart.QuadPart) * counterToMs);
			FrameStatsAdd(frameStats, intervalMs, workMs);
			Present(); // 垂直同期の待ちは処理時間に含めない（フレーム間隔には含まれる）

			dwFrameCount++;		// 処理回数のカウントを加算
		}
		else
		{
			// 次のフレームまで少し休む（休まずに回り続けると、CPU を1コアまるごと使ってしまう）
			g_Game.PollNetwork();
			Sleep(1);
		}
	} while (msg.message != WM_QUIT);


	//終了処理
	Finalize();


	return (int)msg.wParam;
}


/*------------------------------------------------------------------------------
	ウィンドウプロシージャ
------------------------------------------------------------------------------*/
LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_DESTROY: // ウィンドウの破棄メッセージ
		PostQuitMessage(0); // WM_QUITメッセージの送信
		break;

	case WM_INPUT: // マウスの移動量・パッドの状態（Raw Input。それぞれ自分の分だけ読む）
		MouseOnRawInput(lParam);
		PadOnRawInput(lParam);
		return DefWindowProc(hWnd, uMsg, wParam, lParam);

	case WM_INPUT_DEVICE_CHANGE: // パッドをつないだ・外した
		PadOnDeviceChange(wParam, lParam);
		break;

	case WM_ACTIVATE: // ウィンドウが選ばれた／選ばれなくなった
		if (LOWORD(wParam) == WA_INACTIVE) KeyboardReset();
		// 選ばれている間だけマウスでカメラを回す（終了確認のメッセージボックスが出たときなどは自動で解除）
		// 【マウス操作は一旦止めている】カーソルを取られないように、ロックしない（2026-10-24、管理者の指示）。
		// 戻すときは、この行と Update の Tab の3行のコメントを外す。
		// MouseSetLock(LOWORD(wParam) != WA_INACTIVE);
		return DefWindowProc(hWnd, uMsg, wParam, lParam);

	case WM_KEYDOWN:
		if (wParam == VK_F11) {
			if ((lParam & (1LL << 30)) == 0) SetFullscreen(hWnd, !g_Fullscreen);
			break;
		}
		KeyboardOnKeyDown((int)wParam, (lParam & (1LL << 30)) != 0);
		if (wParam == VK_ESCAPE)
		{
			SendMessage(hWnd, WM_CLOSE, 0, 0);
		}
		break;

	case WM_SYSKEYDOWN:
		if (wParam == VK_RETURN && (lParam & (1LL << 29)) != 0) {
			g_FullscreenEnterDown = true;
			if ((lParam & (1LL << 30)) == 0) SetFullscreen(hWnd, !g_Fullscreen);
			return 0;
		}
		return DefWindowProc(hWnd, uMsg, wParam, lParam);

	case WM_SETCURSOR:
		if (g_Fullscreen && LOWORD(lParam) == HTCLIENT) {
			SetCursor(nullptr);
			return TRUE;
		}
		return DefWindowProc(hWnd, uMsg, wParam, lParam);

	case WM_CLOSE:
	{
		int ret = MessageBox(hWnd, "終了しますか？", "ゲーム", MB_OKCANCEL);
		if (ret == IDOK)
		{
			DestroyWindow(hWnd);
		}
		break;
	}

	default:
		// 通常のメッセージ処理はこの関数に任せる
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}
	return 0;
}


bool Initialize(HWND hWnd, const NET_OPTIONS& options)
{
	//COM初期化
	CoInitializeEx(nullptr, COINITBASE_MULTITHREADED);

	//DirectX初期化
	if (!DirectXInitialize(hWnd)) {
		return false;
	}

	//シェーダー初期化
	if (!Shader_Initialize(DirectXGetDevice(), DirectXGetDeviceContext())) {
		return false;
	}

	//ゲームの時間の初期化
	GameTimeInitialize();

	//マウス初期化
	MouseInitialize(hWnd);

	//パッド（PS4 コントローラー）初期化
	PadInitialize(hWnd);

	//行動（入力を「ジャンプ」「つかむ」などに変える仕組み）初期化
	ActionInitialize();

	//形（箱・球・カプセル）の描画の部品の初期化
	ShapeInitialize();
	ModelInitialize();

	//画面表示の描画部品まで初期化してから、ゲームを初期化する
	Number2DInitialize();
	if (!g_Game.Initialize(options)) {
		MessageBox(hWnd, g_Game.GetNetSession().GetError().c_str(), TITLE, MB_OK | MB_ICONERROR);
		return false;
	}

	return true;
}


void Finalize(void)
{
	//初期化と逆の順番で終了処理をする
	DebugWindowFinalize();
	g_Game.Finalize();
	Number2DFinalize();

	ModelFinalize();
	ShapeFinalize();

	PadFinalize();
	MouseFinalize();

	//シェーダー終了処理
	Shader_Finalize();

	//DirectX終了処理
	DirectXFinalize();

	//COM終了処理（CoInitializeEx と対）
	CoUninitialize();

	//タイマーの分解能を元に戻す（timeBeginPeriod と対）
	timeEndPeriod(1);
}

void Update(void)
{
	//ゲームの時間を1フレーム分進める（このフレームの計算はすべて同じ dt を使う）
	GameTimeUpdate();

	//入力は時間の次に更新する
	// 同じスレッドのデバッグ用ウィンドウを選んだときも、ゲームへキーを渡さない。
	if ((GetKeyState(VK_RETURN) & 0x8000) == 0) g_FullscreenEnterDown = false;
	if (GetActiveWindow() == g_MainWindow && !g_FullscreenEnterDown) KeyboardUpdate();
	else KeyboardReset();
	MouseUpdate();
	PadUpdate();

	//Tab でマウスのロック（カーソルを隠してカメラを回す）を切り替える
	//【マウス操作は一旦止めている】ロックしないので、マウスでカメラを回す・ターゲットを切り替えることもない。
	//戻すときは、この3行と WndProc の WM_ACTIVATE の MouseSetLock のコメントを外す（左右クリックの触腕は今も使える）
	//if (KeyboardIsTrigger(VK_TAB)) {
	//	MouseSetLock(!MouseIsLocked());
	//}

	//キーボード・マウスの状態を「行動」に変える（ロックの切り替えのあとなので、切り替えた瞬間はマウスの移動量が 0）
	ActionUpdate();

	//SHARE / V で振動のオン・オフを切り替える（オンにしたら短く振動して知らせる。タイトルバーにも出す）
	bool toggleRumble = false;
	for (int device = 0; device < ACTION_DEVICE_COUNT; ++device) {
		if (g_Game.GetNetSession().IsOnline() && device != g_Game.GetNetSession().GetLocalDevice()
			&& device != ACTION_DEVICE_KEYBOARD) continue;
		if (ActionIsTrigger(device, ACTION_RUMBLE_TOGGLE)) toggleRumble = true;
	}
	if (toggleRumble) {
		PadSetRumbleEnabled(!PadIsRumbleEnabled());
		if (PadIsRumbleEnabled()) {
			for (int pad = 0; pad < PAD_COUNT; ++pad) {
				if (g_Game.GetNetSession().IsOnline() && pad != g_Game.GetNetSession().GetLocalDevice()) continue;
				PadRumble(pad, RUMBLE_ON_NOTICE_STRENGTH, RUMBLE_ON_NOTICE_STRENGTH, RUMBLE_ON_NOTICE_SECONDS);
			}
		}
	}

	g_Game.Update();
}

void Draw(void)
{
	Shader_Begin();

	Clear();

	g_Game.Draw();
}
