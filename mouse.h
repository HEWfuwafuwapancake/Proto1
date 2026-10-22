#pragma once

#include <Windows.h>

/// @file mouse.h
/// @brief マウス入力（カメラを回すための移動量を取る）
/// @details ロック中はカーソルを隠してウィンドウの中に閉じ込め、どれだけ動かしたかだけを読む

/// @brief Raw Input（マウスの生の移動量）を受け取る登録をする
/// @param hWnd 移動量を受け取るウィンドウ
void MouseInitialize(HWND hWnd);

/// @brief ロックを外して、マウスの終了処理をする
void MouseFinalize(void);

/// @brief ためた移動量を、このフレームの移動量として取り出す（毎フレーム、Update の最初のほうで呼ぶ）
void MouseUpdate(void);

/// @brief WM_INPUT で届いた移動量をためる（WndProc の WM_INPUT から呼ぶ）
/// @param lParam WM_INPUT の lParam
void MouseOnRawInput(LPARAM lParam);

/// @brief マウスのロックの状態を設定する（今と同じ状態を指定したときは何もしない）
/// @details 状態が変わったときは、たまっている移動量とこのフレームの移動量を 0 に戻す（切り替えた瞬間にカメラが飛ばないように）
/// @param lock true：ロックする（カーソルを隠してウィンドウの中に閉じ込める）  false：ロックを外す（元に戻す）
void MouseSetLock(bool lock);

/// @brief マウスがロック中か
/// @return ロック中なら true
bool MouseIsLocked(void);

/// @brief このフレームで横に動いた量を取得する（MouseUpdate で取り出した値）
/// @details ロックを外すと MouseSetLock が 0 に戻し、ロック中でない間は移動量をためないので、ロック中でなければ 0
/// @return 移動量（右がプラス）
int MouseGetDeltaX(void);

/// @brief このフレームで縦に動いた量を取得する（MouseUpdate で取り出した値）
/// @details ロックを外すと MouseSetLock が 0 に戻し、ロック中でない間は移動量をためないので、ロック中でなければ 0
/// @return 移動量（下がプラス）
int MouseGetDeltaY(void);
