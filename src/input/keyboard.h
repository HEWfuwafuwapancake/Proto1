#pragma once

/// @file keyboard.h
/// @brief キーボード入力
/// @details キーは仮想キーコードで指定する（例：VK_LEFT、VK_SPACE、'W'（英字は大文字））

/// @brief キーボードの状態を読み込む（毎フレーム、Update の最初に呼ぶ）
/// @details このゲームのウィンドウが選ばれていないときは、何も押されていない扱いにする
void KeyboardUpdate(void);

/// @brief ウィンドウで受け取ったキーの押下を次の更新まで記憶する
/// @param key 仮想キーコード
/// @param repeat 自動リピートか（true：リピート、false：新しい押下）
void KeyboardOnKeyDown(int key, bool repeat);

/// @brief キーの状態と記憶した押下を消す（ウィンドウの非選択時）
void KeyboardReset(void);

/// @brief 指定キーを押した時点で Shift を押していたかを返す
/// @param key 仮想キーコード
/// @return Shift を押しながら押していれば true
bool KeyboardWasShiftDown(int key);

/// @brief キーを押しているか
/// @param key 仮想キーコード
/// @return 押している間ずっと true
bool KeyboardIsPress(int key);

/// @brief キーを押した瞬間か
/// @param key 仮想キーコード
/// @return 押した瞬間の1フレームだけ true
bool KeyboardIsTrigger(int key);

/// @brief キーを離した瞬間か
/// @param key 仮想キーコード
/// @return 離した瞬間の1フレームだけ true
bool KeyboardIsRelease(int key);
