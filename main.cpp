#include <SDKDDKVer.h>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <mmsystem.h>
#include <stdio.h>
#include "dx11.h"
#include "main.h"
#include "shader.h"
#include "keyboard.h"
#include "mouse.h"
#include "camera.h"
#include "field.h"
#include "cube.h"
#include "polygon2D.h"

#pragma comment (lib, "winmm.lib")


/*------------------------------------------------------------------------------
	ウィンドウ情報
------------------------------------------------------------------------------*/
static constexpr char WINDOW_CLASS[] = "Proto1"; // メインウィンドウクラス名
static constexpr char TITLE[] = "Proto1";        // タイトルバーのテキスト


/*------------------------------------------------------------------------------
	ウィンドウプロシージャ プロトタイプ宣言
------------------------------------------------------------------------------*/
LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

bool Initialize(HWND hWnd);
void Finalize(void);
void Update(void);
void Draw(void);


int APIENTRY WinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPSTR lpCmdLine, _In_ int nCmdShow)
{
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
	DWORD windowStyle = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
	// 指定したクライアント領域を確保するために新たな矩形座標を計算
	AdjustWindowRect(&windowRect, windowStyle, FALSE);

	// 新たなWindowの矩形座標から幅と高さを算出
	int windowWidth = windowRect.right - windowRect.left;
	int windowHeight = windowRect.bottom - windowRect.top;

	/* メインウィンドウの作成 */
	HWND hWnd = CreateWindow(WINDOW_CLASS, TITLE, windowStyle,
		CW_USEDEFAULT, CW_USEDEFAULT, windowWidth, windowHeight, nullptr, nullptr, hInstance, nullptr);

	//kokonihenkou
	//初期化処理（失敗したら後片付けをして終了する）
	if (!Initialize(hWnd)) {
		Finalize();
		return -1;
	}


	ShowWindow(hWnd, nCmdShow);
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
			// FPS（1秒間に何回 Update・Draw したか）をタイトルバーに表示する
			char title[64];
			DWORD elapsed = dwCurrentTime - dwFPSLastTime;	// 1000ミリ秒ちょうどより少し長いので、四捨五入して表示する
			sprintf_s(title, "%s  FPS: %lu", TITLE, (dwFrameCount * 1000 + elapsed / 2) / elapsed);
			SetWindowText(hWnd, title);

			dwFPSLastTime = dwCurrentTime;				// FPSを測定した時刻を保存
			dwFrameCount = 0;							// カウントをクリア
		}

		if ((dwCurrentTime - dwExecLastTime) >= FRAME_TIME[frameIndex])	// 1/60秒ごとに実行
		{
			// 次のフレームの予定時刻（今の時刻ではなく予定時刻から進めるので、Sleep の寝すぎでずれていかない）
			dwExecLastTime += FRAME_TIME[frameIndex];
			frameIndex = (frameIndex + 1) % 3;
			if ((dwCurrentTime - dwExecLastTime) > 100) {
				dwExecLastTime = dwCurrentTime;	// 大きく遅れたとき（ウィンドウをつかんで動かしていた など）は追いつこうとせず仕切り直す
			}

			Update();			// 更新処理
			Draw();				// 描画処理

			dwFrameCount++;		// 処理回数のカウントを加算
		}
		else
		{
			// 次のフレームまで少し休む（休まずに回り続けると、CPU を1コアまるごと使ってしまう）
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

	case WM_INPUT: // マウスの移動量（Raw Input）
		MouseOnRawInput(lParam);
		return DefWindowProc(hWnd, uMsg, wParam, lParam);

	case WM_ACTIVATE: // ウィンドウが選ばれた／選ばれなくなった
		// 選ばれている間だけマウスでカメラを回す（終了確認のメッセージボックスが出たときなどは自動で解除）
		MouseSetLock(LOWORD(wParam) != WA_INACTIVE);
		return DefWindowProc(hWnd, uMsg, wParam, lParam);

	case WM_KEYDOWN:
		if (wParam == VK_ESCAPE)
		{
			SendMessage(hWnd, WM_CLOSE, 0, 0);
		}
		break;

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


bool Initialize(HWND hWnd)
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

	//マウス初期化
	MouseInitialize(hWnd);

	//カメラ初期化
	CameraInitialize();

	//3Dオブジェクト初期化
	FieldInitialize();
	CubeInitialize();

	//2Dポリゴン初期化
	Polygon2DInitialize();

	return true;
}


void Finalize(void)
{
	//初期化と逆の順番で終了処理をする
	Polygon2DFinalize();

	CubeFinalize();
	FieldFinalize();

	CameraFinalize();
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
	//入力は一番最初に更新する
	KeyboardUpdate();
	MouseUpdate();

	//Tab でマウスのロック（カーソルを隠してカメラを回す）を切り替える
	if (KeyboardIsTrigger(VK_TAB)) {
		MouseSetLock(!MouseIsLocked());
	}

	CameraUpdate();
	FieldUpdate();
	CubeUpdate();
	Polygon2DUpdate();
}

void Draw(void)
{
	Shader_Begin();

	Clear();

	// ---- 3D描画 ----
	SetDepthEnable(true);
	CameraDraw();

	// ライト（斜め上から当たる白い光）
	LIGHT light{};
	light.Enable = TRUE;
	light.Direction = { 0.5f, -1.0f, 0.8f, 0.0f };
	light.Diffuse = { 1.0f, 1.0f, 1.0f, 1.0f };
	light.Ambient = { 0.3f, 0.3f, 0.3f, 1.0f };
	Shader_SetLight(light);

	FieldDraw();
	CubeDraw();

	// ---- 2D描画（UI）：3Dの上に重ねるので最後に描く ----
	SetDepthEnable(false);
	light.Enable = FALSE;
	Shader_SetLight(light);

	Polygon2DDraw();

	Present();
}
