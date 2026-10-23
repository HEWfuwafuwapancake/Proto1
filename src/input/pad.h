#pragma once

#include <Windows.h>
#include "input/padcalc.h"

/// @file pad.h
/// @brief PS4 コントローラー（DUALSHOCK 4）を USB で読む（Raw Input。ドライバや外部ライブラリは使わない）
/// @details 読むのは最初に見つけた DUALSHOCK 4 の1台だけ。つないでいないとき・外れたとき・
///          ウィンドウが選ばれていないときは「何も押していない」扱いにする

/// @brief Raw Input にゲームパッドを登録する（つないだ・外れたの知らせも受け取る）
/// @param hWnd 入力を受け取るウィンドウ
void PadInitialize(HWND hWnd);

/// @brief パッドの終了処理をする（状態を「何も押していない」に戻す）
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

/// @brief DUALSHOCK 4 をつないでいて、データが届いているか
/// @return つないでいれば true
bool PadIsConnected(void);

/// @brief パッドのボタンを押しているか（このフレームの状態）
/// @param button ボタン（PAD_BUTTON）
/// @return 押していれば true（つないでいないときは false）
bool PadIsPress(PAD_BUTTON button);

/// @brief 左スティックの向きを取得する（遊びを取り除いたもの）
/// @return X：右がプラス、Y：上がプラス（長さ 0〜1。つないでいないときは 0）
ACTION_AXIS PadGetLeftStick(void);

/// @brief 右スティックの向きを取得する（遊びを取り除いたもの）
/// @return X：右がプラス、Y：上がプラス（長さ 0〜1。つないでいないときは 0）
ACTION_AXIS PadGetRightStick(void);
