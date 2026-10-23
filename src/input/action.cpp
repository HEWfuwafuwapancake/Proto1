#include <Windows.h>
#include "input/action.h"
#include "input/keyboard.h"
#include "input/mouse.h"

#ifdef _DEBUG
#include <iostream>
#include "core/debug_ostream.h"
#endif


//------------------------------------------------------------------------------
//	調整用の値
//------------------------------------------------------------------------------
// 行動ごとのキー（ACTION の並びと同じ順番。マウスのボタンも仮想キーコードで読める）
static constexpr int ACTION_KEYS[ACTION_COUNT] = {
	VK_SPACE,		// ACTION_JUMP
	VK_LBUTTON,		// ACTION_ARM_LEFT（左クリック）
	VK_RBUTTON,		// ACTION_ARM_RIGHT（右クリック）
	'E',			// ACTION_SLAP
	'R',			// ACTION_RESTART
};

// 移動のキー
static constexpr int MOVE_KEY_FORWARD = 'W';
static constexpr int MOVE_KEY_BACK = 'S';
static constexpr int MOVE_KEY_LEFT = 'A';
static constexpr int MOVE_KEY_RIGHT = 'D';

// マウス感度：マウスを1カウント動かしたときに回る角度（ラジアン）
// 大きくすると速く回る。0.001f（ゆっくり）〜 0.005f（速い）くらいが目安
static constexpr float MOUSE_SENSITIVITY = 0.00125f;
static constexpr bool  MOUSE_INVERT_Y = false;			// true にすると上下が逆になる

static constexpr float KEY_LOOK_SPEED = 2.0f * 3.14159265f / 180.0f;	// 矢印キーで1フレームに回る角度（2度）


static bool g_Press[ACTION_COUNT] = {};		// 今のフレームで押しているか
static bool g_OldPress[ACTION_COUNT] = {};	// 1つ前のフレームで押していたか
static ACTION_AXIS g_Move = {};				// 移動の向き
static ACTION_AXIS g_Look = {};				// このフレームでカメラを回す角度

#ifdef _DEBUG
// 確かめ用に出力ウィンドウに出す名前（ACTION の並びと同じ順番。No.8 でプレイヤーができたら消す）
static const char* const ACTION_NAMES[ACTION_COUNT] = {
	"ジャンプ", "左の触腕", "右の触腕", "はたく", "やり直し",
};
#endif


void ActionInitialize(void)
{
	for (int i = 0; i < ACTION_COUNT; i++) {
		g_Press[i] = false;
		g_OldPress[i] = false;
	}
	g_Move = { 0.0f, 0.0f };
	g_Look = { 0.0f, 0.0f };
}

void ActionUpdate(void)
{
	// ---- ボタン ----
	// 押した瞬間・離した瞬間を自分で判定するのは、No.6 でパッドのボタンも「または」でまとめるため
	for (int i = 0; i < ACTION_COUNT; i++) {
		g_OldPress[i] = g_Press[i];
		g_Press[i] = KeyboardIsPress(ACTION_KEYS[i]);

#ifdef _DEBUG
		if (g_Press[i] && !g_OldPress[i]) {
			hal::dout << "行動：" << ACTION_NAMES[i] << std::endl;
		}
#endif
	}

	// ---- 移動 ----
	g_Move = ActionCalcMoveFromKeys(
		KeyboardIsPress(MOVE_KEY_FORWARD), KeyboardIsPress(MOVE_KEY_BACK),
		KeyboardIsPress(MOVE_KEY_LEFT), KeyboardIsPress(MOVE_KEY_RIGHT));

	// ---- カメラを回す ----
	// マウス（ロック中でなければ移動量は 0）。マウスは下がプラスなので、上がプラスに直す
	g_Look.X = (float)MouseGetDeltaX() * MOUSE_SENSITIVITY;
	g_Look.Y = -(float)MouseGetDeltaY() * MOUSE_SENSITIVITY * (MOUSE_INVERT_Y ? -1.0f : 1.0f);

	// 矢印キー（マウスと足し合わせる）
	if (KeyboardIsPress(VK_LEFT))  g_Look.X -= KEY_LOOK_SPEED;
	if (KeyboardIsPress(VK_RIGHT)) g_Look.X += KEY_LOOK_SPEED;
	if (KeyboardIsPress(VK_UP))    g_Look.Y += KEY_LOOK_SPEED;
	if (KeyboardIsPress(VK_DOWN))  g_Look.Y -= KEY_LOOK_SPEED;
}

bool ActionIsPress(ACTION action)
{
	return g_Press[action];
}

bool ActionIsTrigger(ACTION action)
{
	return g_Press[action] && !g_OldPress[action];
}

bool ActionIsRelease(ACTION action)
{
	return !g_Press[action] && g_OldPress[action];
}

ACTION_AXIS ActionGetMove(void)
{
	return g_Move;
}

ACTION_AXIS ActionGetLook(void)
{
	return g_Look;
}
