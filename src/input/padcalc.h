#pragma once

#include "input/actioncalc.h"

/// @file padcalc.h
/// @brief DS4 / DualSense・Switch Pro 形式・XInput の入力変換、出力レポート、スティックと振動の計算
/// @details DirectX と Windows を使わないので、単体テスト（tests/Proto1Tests）からそのまま使える

/// @brief 対応するパッドの入力形式
enum PAD_MODEL
{
	PAD_MODEL_NONE,       ///< 対応していない機器
	PAD_MODEL_DS4,        ///< DUALSHOCK 4（初代・2代目）
	PAD_MODEL_DUALSENSE,  ///< DualSense（PS5 純正）
	PAD_MODEL_XINPUT,     ///< Xbox 形式（XInput）
	PAD_MODEL_SWITCH_PRO, ///< Switch Pro コントローラー形式
};

/// @brief HID のメーカー番号と製品番号から、対応する機種を調べる（XInput は別途検出）
/// @param vendorId メーカー番号（VID）
/// @param productId 製品番号（PID）
/// @return 対応する機種。対応外なら PAD_MODEL_NONE
PAD_MODEL PadCalcIdentifyModel(unsigned int vendorId, unsigned int productId);

/// @brief パッドのボタン（PAD_STATE::Buttons のビット）
enum PAD_BUTTON : unsigned int
{
	PAD_BUTTON_CROSS = 1u << 0,		///< ×
	PAD_BUTTON_CIRCLE = 1u << 1,	///< ○
	PAD_BUTTON_SQUARE = 1u << 2,	///< □
	PAD_BUTTON_TRIANGLE = 1u << 3,	///< △
	PAD_BUTTON_L1 = 1u << 4,		///< L1
	PAD_BUTTON_R1 = 1u << 5,		///< R1
	PAD_BUTTON_L2 = 1u << 6,		///< L2（奥まで押し込まなくても、少し押せば押した扱い）
	PAD_BUTTON_R2 = 1u << 7,		///< R2（同上）
	PAD_BUTTON_SHARE = 1u << 8,		///< SHARE（DualSense では CREATE）
	PAD_BUTTON_OPTIONS = 1u << 9,	///< OPTIONS
	PAD_BUTTON_L3 = 1u << 10,		///< L3（左スティックの押し込み）
	PAD_BUTTON_R3 = 1u << 11,		///< R3（右スティックの押し込み）
	PAD_BUTTON_PS = 1u << 12,		///< PS ボタン
	PAD_BUTTON_DPAD_UP = 1u << 13,	///< 十字キー上
	PAD_BUTTON_DPAD_RIGHT = 1u << 14,	///< 十字キー右
	PAD_BUTTON_DPAD_DOWN = 1u << 15,	///< 十字キー下
	PAD_BUTTON_DPAD_LEFT = 1u << 16,	///< 十字キー左
};

/// @brief パッド1台の状態
struct PAD_STATE
{
	ACTION_AXIS LeftStick;	///< 左スティック（X：右がプラス、Y：上がプラス。どちらも -1〜1）
	ACTION_AXIS RightStick;	///< 右スティック（同上）
	unsigned int Buttons;	///< 押しているボタン（PAD_BUTTON のビットを「または」でまとめたもの）
};

/// @brief パッドの振動の強さと、止まるまでの時間
struct PAD_RUMBLE
{
	float SmallMotor;       ///< 右の小さいモーターの強さ（0〜1）
	float LargeMotor;       ///< 左の大きいモーターの強さ（0〜1）
	float RemainingSeconds; ///< 両方のモーターを止めるまでの残り秒数（0 なら停止中）
};

/// @brief DUALSHOCK 4 の USB 出力レポートを組み立てる（残りのバイトはすべて 0）
/// @details 番号は [0] 0x05、変更の印は [1] 0x07。各値を 0〜1 に収め、255 倍して四捨五入する。
///          小さいモーターは [4]、大きいモーターは [5]、赤・緑・青は [6]〜[8]。
/// @param[out] data 出力先（作れなかったときは書き換えない）
/// @param size 出力先のバイト数（9 以上）
/// @param smallMotor 小さいモーターの強さ（0〜1。範囲外は切る）
/// @param largeMotor 大きいモーターの強さ（0〜1。範囲外は切る）
/// @param red ライトバーの赤（0〜1。範囲外は切る）
/// @param green ライトバーの緑（0〜1。範囲外は切る）
/// @param blue ライトバーの青（0〜1。範囲外は切る）
/// @return 組み立てられたら true。data が null または size が 9 未満なら false
bool PadCalcBuildDs4UsbOutput(unsigned char* data, int size,
	float smallMotor, float largeMotor, float red, float green, float blue);

/// @brief DualSense の USB 出力を組み立てる（互換振動とライトバー。残りは0）
/// @details [0] は0x02、[1] は互換振動の印0x03、[2] は色の印0x04。
///          小さいモーターは [3]、大きいモーターは [4]、RGB は [45]〜[47]。
/// @param[out] data 出力先（作れなかったときは書き換えない）
/// @param size 出力先のバイト数（48以上。実機の長さはHIDから取得する）
/// @param smallMotor 小さいモーターに相当する振動の強さ（0〜1。範囲外は切る）
/// @param largeMotor 大きいモーターに相当する振動の強さ（0〜1。範囲外は切る）
/// @param red ライトバーの赤（0〜1。範囲外は切る）
/// @param green ライトバーの緑（0〜1。範囲外は切る）
/// @param blue ライトバーの青（0〜1。範囲外は切る）
/// @return 組み立てられたら true。data が null または size が48未満なら false
bool PadCalcBuildDualSenseUsbOutput(unsigned char* data, int size,
	float smallMotor, float largeMotor, float red, float green, float blue);

/// @brief 振動を重ねる（各モーターは強いほう、残り時間は長いほうを残す）
/// @param[in,out] rumble 今の振動（残り時間が 0 以下なら古い強さを捨てる）
/// @param smallMotor 小さいモーターの強さ（0〜1。範囲外は切る）
/// @param largeMotor 大きいモーターの強さ（0〜1。範囲外は切る）
/// @param seconds 振動する秒数（0 以下なら、強さにかかわらず今の振動も止める）
void PadCalcRumble(PAD_RUMBLE& rumble, float smallMotor, float largeMotor, float seconds);

/// @brief 振動の残り時間を減らし、時間がなくなったら両方のモーターを止める
/// @param[in,out] rumble 今の振動（残り時間が 0 以下なら強さも 0 にする）
/// @param dt 減らす秒数（0 以下なら時間を進めない）
void PadCalcUpdateRumble(PAD_RUMBLE& rumble, float dt);

/// @brief DUALSHOCK 4 の USB のレポートの、スティック1本分の値（0〜255）を -1〜1 に変える
/// @param raw レポートの値（0：左・上の端、255：右・下の端）
/// @return -1〜1（0 が左の端、255 が右の端。上下のときは呼んだ側で向きを逆にする）
float PadCalcStickValue(unsigned char raw);

/// @brief スティックの真ん中の遊び（デッドゾーン）を取り除く
/// @details 手を離しても真ん中から少しずれた値が来るので、その分を 0 にする。
///          遊びの外側は 0 から 1 までなめらかにつながるように広げ直し、長さは 1 までにする
/// @param stick スティックの向き
/// @param deadZone 遊びの大きさ（0〜1 未満。0.1 なら、長さ 0.1 以下は 0）
/// @return 遊びを取り除いた向き（長さ 0〜1）
ACTION_AXIS PadCalcApplyDeadZone(ACTION_AXIS stick, float deadZone);

/// @brief DUALSHOCK 4 を USB でつないだときのレポートを読む
/// @details レポートの並び：[0] レポート番号（0x01）、[1]〜[4] 左X・左Y・右X・右Y、
///          [5] 方向キー（下位4ビット）と □×○△、[6] L1 R1 L2 R2 SHARE OPTIONS L3 R3、[7] PS ボタン（最下位ビット）。
///          スティックは遊びを取り除く前の値（遊びは PadCalcApplyDeadZone で取り除く）
/// @param data レポートのバイト列
/// @param size data のバイト数
/// @param[out] state 読んだ状態（読めなかったときは書き換えない）
/// @return 読めたら true。バイト数が足りないときや、レポート番号が 0x01 でないときは false
bool PadCalcParseDs4UsbReport(const unsigned char* data, int size, PAD_STATE& state);

/// @brief DualSense の64バイトの USB 入力レポートを読む
/// @details [0] は0x01、[1]〜[4] は左右スティック、[8]〜[10] は DS4 と同じビットのボタン。
///          [5]・[6] のトリガー量と [7] のカウンターはボタンとして読まない。CREATE は SHARE に対応。
///          Bluetooth の簡易入力も番号0x01だが短いため拒否する。遊びは別途取り除く。
/// @param data レポートのバイト列
/// @param size data のバイト数
/// @param[out] state 読んだ状態（読めなかったときは書き換えない）
/// @return 64バイトで番号0x01なら true。それ以外は false
bool PadCalcParseDualSenseUsbReport(const unsigned char* data, int size, PAD_STATE& state);

/// @brief XInput のボタン・トリガー・スティックを共通の状態へ変換する（上下は上がプラス）
/// @param buttons XInput のボタンビット
/// @param leftTrigger 左トリガー（0〜255）
/// @param rightTrigger 右トリガー（0〜255）
/// @param lx 左スティックX（-32768〜32767）
/// @param ly 左スティックY（同上）
/// @param rx 右スティックX（同上）
/// @param ry 右スティックY（同上）
/// @param[out] state 変換した状態（遊びは別途取り除く）
void PadCalcParseXInput(unsigned int buttons, unsigned char leftTrigger, unsigned char rightTrigger,
	short lx, short ly, short rx, short ry, PAD_STATE& state);

/// @brief Switch Pro 形式のフル（0x30）・簡易（0x3F）入力を読む。ボタンは位置で合わせる
/// @param data レポートのバイト列
/// @param size バイト数（12以上）
/// @param[out] state 読んだ状態（失敗時は変更しない。簡易入力のYは下がプラスとして反転する）
/// @return 対応するレポートを読めたら true
bool PadCalcParseSwitchProReport(const unsigned char* data, int size, PAD_STATE& state);

/// @brief Switch Pro の USB 用の合図を組み立てる（残りは0）
/// @param[out] data 出力先
/// @param size バイト数（2以上）
/// @param command 合図（0x02：ハンドシェイク、0x04：HIDだけで通信）
/// @return 組み立てられたら true（失敗時は変更しない）
bool PadCalcBuildSwitchUsbCommand(unsigned char* data, int size, unsigned char command);

/// @brief Switch Pro のフル入力モード指定を振動なしで組み立てる（残りは0）
/// @param[out] data 出力先
/// @param size バイト数（12以上）
/// @param counter 送信カウンター（下位4ビットを使う）
/// @return 組み立てられたら true（失敗時は変更しない）
bool PadCalcBuildSwitchInputMode(unsigned char* data, int size, unsigned char counter);
