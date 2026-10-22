#include <Windows.h>
#include "keyboard.h"


static BYTE g_KeyState[256] = {};		// 今のフレームのキーの状態
static BYTE g_OldKeyState[256] = {};	// 1つ前のフレームのキーの状態


void KeyboardUpdate(void)
{
	// 前のフレームの状態を取っておいてから、今の状態を取得する
	memcpy(g_OldKeyState, g_KeyState, sizeof(g_KeyState));

	// このゲームのウィンドウが選ばれていないときは、何も押されていない扱いにする
	if (GetActiveWindow() == NULL || !GetKeyboardState(g_KeyState)) {
		memset(g_KeyState, 0, sizeof(g_KeyState));
	}
}

bool KeyboardIsPress(int key)
{
	return (g_KeyState[key & 0xFF] & 0x80) != 0;	// 最上位ビットが1なら押されている
}

bool KeyboardIsTrigger(int key)
{
	return ((g_KeyState[key & 0xFF] & ~g_OldKeyState[key & 0xFF]) & 0x80) != 0;
}

bool KeyboardIsRelease(int key)
{
	return ((~g_KeyState[key & 0xFF] & g_OldKeyState[key & 0xFF]) & 0x80) != 0;
}
