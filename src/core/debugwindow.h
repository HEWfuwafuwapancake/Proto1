#pragma once

#include <Windows.h>

/// @file debugwindow.h
/// @brief タイトルバーと同じ情報を表示する、読み取り専用のデバッグ用ウィンドウ

/// @brief ゲームと別のモニターを優先してウィンドウを作り、フォーカスを移さず表示する
/// @param gameWindow ゲームのウィンドウ（表示するモニターを選ぶために使う）
/// @return 作成できたら true
bool DebugWindowInitialize(HWND gameWindow);

/// @brief デバッグ用ウィンドウとクラスを破棄する（作っていないときも呼べる）
void DebugWindowFinalize();

/// @brief タイトルバーと同じマルチバイト文字列を表示する（未作成・非表示でも呼べる）
/// @param text 日本語を含む表示文字列。元の文字列は変更せず、長い行は表示幅に合わせて折り返す
void DebugWindowSetText(const char* text);
