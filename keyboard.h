#pragma once

/// @file keyboard.h
/// @brief キーボード入力
/// @details キーは仮想キーコードで指定する（例：VK_LEFT、VK_SPACE、'W'（英字は大文字））

/// @brief キーボードの状態を読み込む（毎フレーム、Update の最初に呼ぶ）
/// @details このゲームのウィンドウが選ばれていないときは、何も押されていない扱いにする
void KeyboardUpdate(void);

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
