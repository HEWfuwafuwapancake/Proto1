#pragma once

#include <Windows.h>

/// @file debugwindow.h
/// @brief 通信の状態・FPS などの情報だけを表示するデバッグ用ウィンドウ（数値の変更はオプションの画面で行う）

/// @brief ゲームと別のモニターを優先してウィンドウを作り、フォーカスを移さず表示する
/// @param gameWindow ゲームのウィンドウ（表示するモニターを選ぶために使う）
/// @return 作成できたら true
bool DebugWindowInitialize(HWND gameWindow);

/// @brief デバッグ用ウィンドウとクラスを破棄する（作っていないときも呼べる）
void DebugWindowFinalize();

/// @brief 表示する情報の文字列を設定する（未作成・非表示でも呼べる。前と同じ内容なら何もしない）
/// @param text 日本語を含むマルチバイト文字列。行は "\r\n" で区切り、長い行は表示幅に合わせて折り返す
void DebugWindowSetText(const char* text);
