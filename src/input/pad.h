#pragma once

#include <Windows.h>
#include "input/padcalc.h"

/// @file pad.h
/// @brief DUALSHOCK 4 / DualSense の USB 入力と、振動・ライトバーの出力（Raw Input と HID）
/// @details 最大4台を接続順に0〜3へ割り当てる。外れた番号は空き、ほかの番号は詰めない。
///          つないでいないとき・外れたとき・
///          ウィンドウが選ばれていないときは「何も押していない」扱いにする

/// @brief 読み取れるパッドの最大台数
static constexpr int PAD_COUNT = 4;

/// @brief Raw Input にゲームパッドを登録する（つないだ・外れたの知らせも受け取る）
/// @param hWnd 入力を受け取るウィンドウ
void PadInitialize(HWND hWnd);

/// @brief 振動を止めて出力用のハンドルを閉じ、状態を「何も押していない」に戻す
void PadFinalize(void);

/// @brief WM_INPUT で届いたパッドの状態を、このフレームの状態として取り出す（毎フレーム、MouseUpdate の次に呼ぶ）
void PadUpdate(void);

/// @brief WM_INPUT で届いたパッドのデータを読む（WndProc の WM_INPUT から呼ぶ。マウスのデータは無視する）
/// @param lParam WM_INPUT の lParam
void PadOnRawInput(LPARAM lParam);

/// @brief パッドをつないだ・外したことを受け取る（WndProc の WM_INPUT_DEVICE_CHANGE から呼ぶ）
/// @param wParam WM_INPUT_DEVICE_CHANGE の wParam（GIDC_ARRIVAL か GIDC_REMOVAL）
/// @param lParam WM_INPUT_DEVICE_CHANGE の lParam（機器のハンドル）
void PadOnDeviceChange(WPARAM wParam, LPARAM lParam);

/// @brief 対応するパッドが接続されているか（入力がまだ届いていなければボタン・スティックは0）
/// @param pad パッドの番号（0〜3。範囲外は未接続扱い）
/// @return つないでいれば true
bool PadIsConnected(int pad);

/// @brief パッドのボタンを押しているか（このフレームの状態）
/// @param pad パッドの番号（0〜3）
/// @param button ボタン（PAD_BUTTON）
/// @return 押していれば true（つないでいないときは false）
bool PadIsPress(int pad, PAD_BUTTON button);

/// @brief 左スティックの向きを取得する（遊びを取り除いたもの）
/// @param pad パッドの番号（0〜3）
/// @return X：右がプラス、Y：上がプラス（長さ 0〜1。つないでいないときは 0）
ACTION_AXIS PadGetLeftStick(int pad);

/// @brief 右スティックの向きを取得する（遊びを取り除いたもの）
/// @param pad パッドの番号（0〜3）
/// @return X：右がプラス、Y：上がプラス（長さ 0〜1。つないでいないときは 0）
ACTION_AXIS PadGetRightStick(int pad);

/// @brief パッドを振動させる（各モーターは今より強いほう、残り時間は長いほうを残す）
/// @param pad パッドの番号（0〜3。範囲外は何もしない）
/// @param smallMotor 右の小さいモーターの強さ（0〜1。範囲外は切る）
/// @param largeMotor 左の大きいモーターの強さ（0〜1。範囲外は切る）
/// @param seconds 振動する秒数（0 以下なら今の振動を止める）
/// @details 振動をオフにしているとき・未接続・出力不可・ウィンドウが選ばれていないときは振動しない
void PadRumble(int pad, float smallMotor, float largeMotor, float seconds);

/// @brief 全パッド共通の振動のオン・オフを設定する（true：振動する、false：振動しない。オフにした瞬間に全台の振動も止める）
/// @param enabled 振動するか
void PadSetRumbleEnabled(bool enabled);

/// @brief 振動がオンか
/// @return オンなら true（はじめは pad.cpp の RUMBLE_ENABLED_AT_START）
bool PadIsRumbleEnabled(void);

/// @brief パッドのライトバーの色を設定する（未接続なら、次につないだときに使う）
/// @param pad パッドの番号（0〜3。範囲外は何もしない）
/// @param red 赤（0〜1。範囲外は切る）
/// @param green 緑（0〜1。範囲外は切る）
/// @param blue 青（0〜1。範囲外は切る）
void PadSetLightColor(int pad, float red, float green, float blue);
