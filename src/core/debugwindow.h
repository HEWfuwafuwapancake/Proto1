#pragma once

#include <Windows.h>

/// @file debugwindow.h
/// @brief タイトルバーの情報と汎用の選択欄を表示するデバッグ用ウィンドウ

/// @brief ゲームと別のモニターを優先してウィンドウを作り、フォーカスを移さず表示する
/// @param gameWindow ゲームのウィンドウ（表示するモニターを選ぶために使う）
/// @return 作成できたら true
bool DebugWindowInitialize(HWND gameWindow);

/// @brief デバッグ用ウィンドウとクラスを破棄する（作っていないときも呼べる）
void DebugWindowFinalize();

/// @brief タイトルバーと同じマルチバイト文字列を表示する（未作成・非表示でも呼べる）
/// @param text 日本語を含む表示文字列。元の文字列は変更せず、長い行は表示幅に合わせて折り返す
void DebugWindowSetText(const char* text);

/// @brief 行の名前と選択肢を設定する（行0〜7、起動時に1回。未作成なら何もしない）
void DebugWindowSetChoices(int row, const char* label, const char* const* options, int count);
/// @brief 行の選択を表示する（ユーザーの変更通知は発生させない）
void DebugWindowSetSelection(int row, int index);
/// @brief 前に読んでからユーザーが選び直したらtrueを返し、段をindexへ入れて通知を消す
bool DebugWindowTakeSelection(int row, int& index);
/// @brief ふつうに戻すボタンが押されたらtrueを返し、通知を消す
bool DebugWindowTakeReset();
/// @brief 選択欄と戻すボタンを使えるか設定する（true：選べる、false：灰色で表示だけ）
void DebugWindowSetChoicesEnabled(bool enabled);

/// @brief 1Pの腕名と耐久を表示する（ユーザー変更通知は出さず、ドラッグ中の位置は上書きしない）
/// @param hand 左0・右1
/// @param armName 腕のマルチバイトの名前
/// @param value 残り耐久0〜100（無制限は-1）
/// @param enabled 操作を設定する（true：つまみを動かせる、false：灰色で表示だけ）
void DebugWindowSetMeter(int hand, const char* armName, int value, bool enabled);
/// @brief 前に読んでからユーザーがつまみを動かしたらtrueを返し、valueへ耐久を入れて通知を消す
/// @param hand 左0・右1
/// @param value 変更後の耐久を書く先
bool DebugWindowTakeMeter(int hand, int& value);
