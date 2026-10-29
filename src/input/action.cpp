#include <Windows.h>
#include "core/gametime.h"
#include <deque>
#include <math.h>
#include "input/action.h"
#include "input/keyboard.h"
#include "input/mouse.h"
#include "input/pad.h"


//------------------------------------------------------------------------------
//	調整用の値
//------------------------------------------------------------------------------
// 行動ごとのキー（ACTION の並びと同じ順番。マウスのボタンも仮想キーコードで読める）
static constexpr int ACTION_KEYS[ACTION_COUNT] = {
	VK_SPACE,		// ACTION_JUMP
	VK_LBUTTON,		// ACTION_ARM_LEFT（左クリック）
	VK_RBUTTON,		// ACTION_ARM_RIGHT（右クリック）
	'R',			// ACTION_RESTART
	'V',			// ACTION_RUMBLE_TOGGLE
	VK_RETURN,		// ACTION_DECIDE
	VK_LEFT,		// ACTION_TARGET_LEFT
	VK_RIGHT,		// ACTION_TARGET_RIGHT
};

// 行動ごとのパッドのボタン（ACTION の並びと同じ順番。企画書の「5. 操作方法」のとおり）
static constexpr PAD_BUTTON ACTION_PAD_BUTTONS[ACTION_TARGET_LEFT] = {
	PAD_BUTTON_CROSS,	// ACTION_JUMP（×）
	PAD_BUTTON_L1,		// ACTION_ARM_LEFT（L1）
	PAD_BUTTON_R1,		// ACTION_ARM_RIGHT（R1）
	PAD_BUTTON_OPTIONS,	// ACTION_RESTART（OPTIONS）
	PAD_BUTTON_SHARE,	// ACTION_RUMBLE_TOGGLE（SHARE）
	PAD_BUTTON_CIRCLE,	// ACTION_DECIDE（○）
};

// 移動のキー
static constexpr int MOVE_KEY_FORWARD = 'W';
static constexpr int MOVE_KEY_BACK = 'S';
static constexpr int MOVE_KEY_LEFT = 'A';
static constexpr int MOVE_KEY_RIGHT = 'D';

// 腕の切り替えモードのキー（押したあと数字キー 1〜6 で、その手の腕を選ぶ）
static constexpr int EQUIP_MODE_KEY_LEFT = 'Q';
static constexpr int EQUIP_MODE_KEY_RIGHT = 'E';

// マウス感度：マウスを1カウント動かしたときに回る角度（ラジアン）
// 大きくすると速く回る。0.001f（ゆっくり）〜 0.005f（速い）くらいが目安
static constexpr float MOUSE_SENSITIVITY = 0.00125f;
static constexpr bool  MOUSE_INVERT_Y = false;			// true にすると上下が逆になる

static constexpr float KEY_LOOK_SPEED = 120.0f * 3.14159265f / 180.0f;	// 矢印キーで1秒に回る角度（120度）
static constexpr float PAD_LOOK_SPEED = 180.0f * 3.14159265f / 180.0f;	// 右スティックを端まで倒したときに1秒に回る角度（180度）

static constexpr float TARGET_PAD_DEADZONE = 0.3f;	// 次の弾きを受け付ける右スティックXの遊び
static constexpr float TARGET_PAD_THRESHOLD = 0.7f;	// この大きさを超えた瞬間に切り替える
static constexpr float TARGET_MOUSE_WINDOW = 0.15f;	// マウス移動量を合計する直近の秒数
static constexpr float TARGET_MOUSE_DISTANCE = 40.0f;	// 切り替える横移動量（ピクセル）
static constexpr float TARGET_MOUSE_COOLDOWN = 0.3f;	// マウスで切り替えてから次を受け付けるまでの秒数（管理者確認済み）


/// @brief 1台の機器から作った、このフレームと前のフレームの行動
struct ACTION_STATE
{
	bool Press[ACTION_COUNT] = {};     ///< このフレームで押しているか
	bool OldPress[ACTION_COUNT] = {};  ///< 前のフレームで押していたか
	ACTION_AXIS Move = {};            ///< 移動の向き
	ACTION_AXIS Look = {};            ///< カメラを回す角度
	bool TargetPadReady = true;       ///< スティックを遊びへ戻したか
};
static ACTION_STATE g_Devices[ACTION_DEVICE_COUNT];
static ACTION_STATE g_SinglePlayer; // 段階Aでは既存のゲームにパッド0＋キーボードを渡す
static ACTION_EQUIP_KEY g_EquipKeys[ACTION_EQUIP_PLAYER_COUNT];
static int g_EquipModes[ACTION_EQUIP_PLAYER_COUNT];
static int g_EquipPlayer = 0;
static float g_TargetMouseCooldown = 0.0f;	// マウスで次の切り替えを受け付けるまでの残り秒数
struct TARGET_MOUSE_SAMPLE
{
	float Age;	// 入力からの秒数
	float Delta;	// 横の移動量
};
static std::deque<TARGET_MOUSE_SAMPLE> g_TargetMouseSamples;


void ActionInitialize(void)
{
	for (ACTION_STATE& state : g_Devices) state = {};
	g_SinglePlayer = {};
	for (int player = 0; player < ACTION_EQUIP_PLAYER_COUNT; ++player) {
		g_EquipKeys[player] = { -1, -1 };
		g_EquipModes[player] = -1;
	}
	g_EquipPlayer = 0;
	g_TargetMouseCooldown = 0.0f;
	g_TargetMouseSamples.clear();
}

// 機器ごとの切り替え入力は押した瞬間だけ出す。自由カメラ用の横入力はそのまま残す。
static void SetTargetInput(ACTION_STATE& state, bool left, bool right)
{
	// 別の弾きが連続フレームで来ても、それぞれをトリガーにする。
	state.OldPress[ACTION_TARGET_LEFT] = state.OldPress[ACTION_TARGET_RIGHT] = false;
	state.Press[ACTION_TARGET_LEFT] = left;
	state.Press[ACTION_TARGET_RIGHT] = right;
}

static void UpdateKeyboardTargetInput(ACTION_STATE& state, float dt)
{
	bool left = KeyboardIsTrigger(ACTION_KEYS[ACTION_TARGET_LEFT]);
	bool right = KeyboardIsTrigger(ACTION_KEYS[ACTION_TARGET_RIGHT]);
	g_TargetMouseCooldown = fmaxf(0.0f, g_TargetMouseCooldown - dt);
	for (TARGET_MOUSE_SAMPLE& sample : g_TargetMouseSamples) sample.Age += dt;
	while (!g_TargetMouseSamples.empty() && g_TargetMouseSamples.front().Age >= TARGET_MOUSE_WINDOW) {
		g_TargetMouseSamples.pop_front();
	}
	if (!MouseIsLocked() || g_TargetMouseCooldown > 0.0f) {
		g_TargetMouseSamples.clear();
	}
	else {
		const float delta = (float)MouseGetDeltaX();
		if (delta != 0.0f) g_TargetMouseSamples.push_back({ 0.0f, delta });
		float distance = 0.0f;
		for (const TARGET_MOUSE_SAMPLE& sample : g_TargetMouseSamples) distance += sample.Delta;
		if (fabsf(distance) > TARGET_MOUSE_DISTANCE) {
			left = left || distance < 0.0f;
			right = right || distance > 0.0f;
			g_TargetMouseSamples.clear();
			g_TargetMouseCooldown = TARGET_MOUSE_COOLDOWN;
		}
	}
	SetTargetInput(state, left, right);
}

static void UpdatePadInput(int device, float dt)
{
	ACTION_STATE& state = g_Devices[device];
	for (int i = 0; i < ACTION_TARGET_LEFT; ++i) {
		state.OldPress[i] = state.Press[i];
		state.Press[i] = PadIsPress(device, ACTION_PAD_BUTTONS[i]);
	}
	state.Move = PadGetLeftStick(device);
	const ACTION_AXIS stick = PadGetRightStick(device);
	state.Look = { stick.X * PAD_LOOK_SPEED * dt, stick.Y * PAD_LOOK_SPEED * dt };
	if (fabsf(stick.X) <= TARGET_PAD_DEADZONE) state.TargetPadReady = true;
	const bool flick = state.TargetPadReady && fabsf(stick.X) > TARGET_PAD_THRESHOLD;
	SetTargetInput(state, flick && stick.X < 0.0f, flick && stick.X > 0.0f);
	if (flick) state.TargetPadReady = false;
}

static void UpdateEquipInput(void)
{
	for (int player = 0; player < ACTION_EQUIP_PLAYER_COUNT; ++player) {
		g_EquipKeys[player] = { -1, -1 };
		if (KeyboardIsTrigger(VK_F1 + player)) g_EquipPlayer = player;
	}
	int pressed = -1;
	bool equipShift = false;
	for (int number = 0; number < 6; ++number) {
		const int topKey = '1' + number;
		const int padKey = VK_NUMPAD1 + number;
		const int key = KeyboardIsTrigger(topKey) ? topKey : padKey;
		if (KeyboardIsTrigger(key)) {
			pressed = number;
			equipShift = KeyboardWasShiftDown(key);
			break;
		}
	}
	g_EquipKeys[g_EquipPlayer] = ActionCalcEquipKey(g_EquipModes[g_EquipPlayer], KeyboardIsTrigger(EQUIP_MODE_KEY_LEFT),
		KeyboardIsTrigger(EQUIP_MODE_KEY_RIGHT), pressed, equipShift);
}

static void UpdateKeyboardInput(float dt)
{
	ACTION_STATE& state = g_Devices[ACTION_DEVICE_KEYBOARD];
	for (int i = 0; i < ACTION_TARGET_LEFT; ++i) {
		state.OldPress[i] = state.Press[i];
		state.Press[i] = ACTION_KEYS[i] != 0 && KeyboardIsPress(ACTION_KEYS[i]);
	}
	UpdateKeyboardTargetInput(state, dt);
	state.Move = ActionCalcMoveFromKeys(
		KeyboardIsPress(MOVE_KEY_FORWARD), KeyboardIsPress(MOVE_KEY_BACK),
		KeyboardIsPress(MOVE_KEY_LEFT), KeyboardIsPress(MOVE_KEY_RIGHT));

	// ---- カメラを回す ----
	// マウス（ロック中でなければ移動量は 0）。マウスは下がプラスなので、上がプラスに直す
	state.Look.X = (float)MouseGetDeltaX() * MOUSE_SENSITIVITY;
	state.Look.Y = -(float)MouseGetDeltaY() * MOUSE_SENSITIVITY * (MOUSE_INVERT_Y ? -1.0f : 1.0f);

	// 矢印キーと右スティックは「1秒に回る角度 × 1フレームの秒数」（マウスは動かした量そのままなので dt を掛けない）
	// 矢印キー（マウスと足し合わせる）
	if (KeyboardIsPress(VK_LEFT))  state.Look.X -= KEY_LOOK_SPEED * dt;
	if (KeyboardIsPress(VK_RIGHT)) state.Look.X += KEY_LOOK_SPEED * dt;
	if (KeyboardIsPress(VK_UP))    state.Look.Y += KEY_LOOK_SPEED * dt;
	if (KeyboardIsPress(VK_DOWN))  state.Look.Y -= KEY_LOOK_SPEED * dt;
}

void ActionUpdate(void)
{
	const float dt = GameTimeGetDeltaTime();
	for (int device = 0; device < ACTION_DEVICE_KEYBOARD; ++device) UpdatePadInput(device, dt);
	UpdateKeyboardInput(dt);
	UpdateEquipInput();

	const ACTION_STATE& pad = g_Devices[0];
	const ACTION_STATE& keyboard = g_Devices[ACTION_DEVICE_KEYBOARD];
	for (int i = 0; i < ACTION_TARGET_LEFT; ++i) {
		// 既存の1人プレイでは、まとめた後に押下・解放の変化を判定する。
		g_SinglePlayer.OldPress[i] = g_SinglePlayer.Press[i];
		g_SinglePlayer.Press[i] = pad.Press[i] || keyboard.Press[i];
	}
	SetTargetInput(g_SinglePlayer,
		pad.Press[ACTION_TARGET_LEFT] || keyboard.Press[ACTION_TARGET_LEFT],
		pad.Press[ACTION_TARGET_RIGHT] || keyboard.Press[ACTION_TARGET_RIGHT]);
	g_SinglePlayer.Move = ActionCalcCombineAxis(keyboard.Move, pad.Move);
	g_SinglePlayer.Look = { keyboard.Look.X + pad.Look.X, keyboard.Look.Y + pad.Look.Y };
}

static bool IsValidDevice(int device)
{
	return device >= 0 && device < ACTION_DEVICE_COUNT;
}

bool ActionIsPress(int device, ACTION action)
{
	return IsValidDevice(device) && g_Devices[device].Press[action];
}

bool ActionIsTrigger(int device, ACTION action)
{
	return IsValidDevice(device) && g_Devices[device].Press[action] && !g_Devices[device].OldPress[action];
}

bool ActionIsRelease(int device, ACTION action)
{
	return IsValidDevice(device) && !g_Devices[device].Press[action] && g_Devices[device].OldPress[action];
}

ACTION_AXIS ActionGetMove(int device)
{
	return IsValidDevice(device) ? g_Devices[device].Move : ACTION_AXIS{};
}

ACTION_AXIS ActionGetLook(int device)
{
	return IsValidDevice(device) ? g_Devices[device].Look : ACTION_AXIS{};
}

bool ActionIsPress(ACTION action)
{
	return g_SinglePlayer.Press[action];
}

bool ActionIsTrigger(ACTION action)
{
	return g_SinglePlayer.Press[action] && !g_SinglePlayer.OldPress[action];
}

bool ActionIsRelease(ACTION action)
{
	return !g_SinglePlayer.Press[action] && g_SinglePlayer.OldPress[action];
}

ACTION_AXIS ActionGetMove(void)
{
	return g_SinglePlayer.Move;
}

ACTION_AXIS ActionGetLook(void)
{
	return g_SinglePlayer.Look;
}

ACTION_EQUIP_KEY ActionGetEquipKey(int player)
{
	return player >= 0 && player < ACTION_EQUIP_PLAYER_COUNT ? g_EquipKeys[player] : ACTION_EQUIP_KEY{ -1, -1 };
}

int ActionGetEquipMode(int player)
{
	return player >= 0 && player < ACTION_EQUIP_PLAYER_COUNT ? g_EquipModes[player] : -1;
}

int ActionGetEquipPlayer(void)
{
	return g_EquipPlayer;
}
