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

/// @brief 耐久のメーターの1行を表示する（初めての行は足して並べ直す。ユーザー変更通知は出さず、ドラッグ中の位置は上書きしない）
/// @param row 行0〜7
/// @param label 行の名前（マルチバイト）
/// @param value 残り耐久0〜100（無制限は-1、装備していないは-2）
/// @param enabled 操作を設定する（true：つまみを動かせる、false：灰色で表示だけ）
void DebugWindowSetMeter(int row, const char* label, int value, bool enabled);
/// @brief 前に読んでからユーザーがつまみを動かしたらtrueを返し、valueへ耐久を入れて通知を消す
/// @param row 行0〜7
/// @param value 変更後の耐久を書く先
bool DebugWindowTakeMeter(int row, int& value);

/// @brief 数字を書き換えられる表の見出しを設定する（起動時に1回。列は6まで・行は8まで）
/// @param columns 数字の列の見出し（マルチバイト）
/// @param columnCount 数字の列の数
/// @param rows 行の名前（マルチバイト）
/// @param notes 行ごとの右端の説明（マルチバイト）
/// @param rowCount 行の数
void DebugWindowSetTable(const char* const* columns, int columnCount, const char* const* rows, const char* const* notes, int rowCount);
/// @brief 表の1欄の数字を表示する（打っている途中の欄は上書きしない。ユーザー変更通知は出さない）
/// @param row 行
/// @param column 列
/// @param value 表示する数字（0以上。負なら使わない欄として「―」を出す）
/// @param enabled 操作を設定する（true：書き換えられる、false：灰色で表示だけ）
void DebugWindowSetTableValue(int row, int column, int value, bool enabled);
/// @brief 前に読んでからユーザーが欄を書き換えたらtrueを返し、valueへ数字を入れて通知を消す
/// @param row 行
/// @param column 列
/// @param value 書き換えた数字を書く先
bool DebugWindowTakeTableValue(int row, int column, int& value);
