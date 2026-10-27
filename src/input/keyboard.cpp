#include <Windows.h>
#include "input/keyboard.h"
#include "input/keyboardcalc.h"


static BYTE g_KeyState[256] = {};		// 今のフレームのキーの状態
static BYTE g_OldKeyState[256] = {};	// 1つ前のフレームのキーの状態
static KeyboardTriggerCalc g_Triggers[256]; // 短い押下をフレームまで保持する

void KeyboardOnKeyDown(int key, bool repeat)
{
	g_Triggers[key & 0xFF].Press(repeat, (GetKeyState(VK_SHIFT) & 0x8000) != 0);
}

void KeyboardReset(void)
{
	memset(g_KeyState, 0, sizeof(g_KeyState));
	memset(g_OldKeyState, 0, sizeof(g_OldKeyState));
	for (KeyboardTriggerCalc& trigger : g_Triggers) trigger.Reset();
}


void KeyboardUpdate(void)
{
	// 前のフレームの状態を取っておいてから、今の状態を取得する
	memcpy(g_OldKeyState, g_KeyState, sizeof(g_KeyState));

	// このゲームのウィンドウが選ばれていないときは、何も押されていない扱いにする
	if (GetActiveWindow() == NULL || !GetKeyboardState(g_KeyState)) {
		KeyboardReset();
	}
	for (KeyboardTriggerCalc& trigger : g_Triggers) trigger.Update();
}

bool KeyboardIsPress(int key)
{
	return (g_KeyState[key & 0xFF] & 0x80) != 0;	// 最上位ビットが1なら押されている
}

bool KeyboardIsTrigger(int key)
{
	return g_Triggers[key & 0xFF].IsTriggered()
		|| ((g_KeyState[key & 0xFF] & ~g_OldKeyState[key & 0xFF]) & 0x80) != 0;
}

bool KeyboardWasShiftDown(int key)
{
	const KeyboardTriggerCalc& trigger = g_Triggers[key & 0xFF];
	return trigger.IsTriggered() ? trigger.HadShift() : KeyboardIsPress(VK_SHIFT);
}

bool KeyboardIsRelease(int key)
{
	return ((~g_KeyState[key & 0xFF] & g_OldKeyState[key & 0xFF]) & 0x80) != 0;
}
