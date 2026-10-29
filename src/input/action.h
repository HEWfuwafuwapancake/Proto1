#pragma once

#include "input/actioncalc.h"

/// @file action.h
/// @brief 入力を「行動」に変える（ゲームはキーやボタンではなく、行動を見る）
/// @details キーボード＋マウスでも、パッド（DUALSHOCK 4）でも、ゲームの側は同じように動く。
///          キーの割り当ては action.cpp の「調整用の値」にまとめてある
///          機器番号ありの関数は機器ごとに独立し、番号なしの行動・移動・カメラは1人プレイ用にパッド0とキーボードをまとめる

/// @brief キーボード＋マウスの機器番号（0〜3は同じ番号のパッド）
static constexpr int ACTION_DEVICE_KEYBOARD = 4;
/// @brief 入力機器の数（パッド4台とキーボード＋マウス）
static constexpr int ACTION_DEVICE_COUNT = 5;
/// @brief 腕の切り替え先として選べる人数
static constexpr int ACTION_EQUIP_PLAYER_COUNT = 4;

/// @brief 機器の今の入力を整数に丸めて取得する（VERSUS_DEVICE_COMBINED は合成入力。範囲外は無入力）
/// @details 装備はキーボード・合成入力なら現在の選択先の値。ローカルでは Game が F1〜F4 のスロットへ配り直す
PLAYER_INPUT ActionGetPlayerInput(int device);

/// @brief 行動の状態を最初に戻す（何も押していない状態）
void ActionInitialize(void);

/// @brief キーボード・マウス・パッドの状態から、このフレームの行動を決める
/// @details 毎フレーム、KeyboardUpdate・MouseUpdate・PadUpdate のあとに1回だけ呼ぶ
void ActionUpdate(void);

/// @brief 指定機器の行動のボタンを押しているか（ターゲット切り替えは1フレームだけ）
/// @param device 機器番号（パッド0〜3、キーボード＋マウス4。範囲外は false）
/// @param action 行動
/// @return 押していれば true
bool ActionIsPress(int device, ACTION action);

/// @brief 指定機器の行動のボタンを押した瞬間か
/// @param device 機器番号（0〜4。範囲外は false）
/// @param action 行動
/// @return 押した瞬間の1フレームだけ true
bool ActionIsTrigger(int device, ACTION action);

/// @brief 指定機器の行動のボタンを離した瞬間か
/// @param device 機器番号（0〜4。範囲外は false）
/// @param action 行動
/// @return 離した瞬間の1フレームだけ true
bool ActionIsRelease(int device, ACTION action);

/// @brief 指定機器の移動の向きを取得する
/// @param device 機器番号（0〜4。範囲外は0）
/// @return X：右、Y：前がプラス。長さは0〜1
ACTION_AXIS ActionGetMove(int device);

/// @brief 指定機器のカメラを回す角度を取得する
/// @param device 機器番号（0〜4。範囲外は0）
/// @return X：右、Y：上がプラスの角度（ラジアン）
ACTION_AXIS ActionGetLook(int device);

/// @brief 1人プレイ用にパッド0とキーボードをまとめた行動のボタンを押しているか
/// @param action 行動
/// @return 押している間ずっと true（ターゲット切り替えは入力が出た1フレームだけtrue）
bool ActionIsPress(ACTION action);

/// @brief 1人プレイ用にパッド0とキーボードをまとめた行動のボタンを押した瞬間か
/// @param action 行動
/// @return 押した瞬間の1フレームだけ true
bool ActionIsTrigger(ACTION action);

/// @brief 1人プレイ用にパッド0とキーボードをまとめた行動のボタンを離した瞬間か
/// @param action 行動
/// @return 離した瞬間の1フレームだけ true
bool ActionIsRelease(ACTION action);

/// @brief 1人プレイの移動の向きを取得する（W/A/S/D とパッド0の左スティックの、強く入れているほう）
/// @return X：左右（右がプラス）、Y：前後（前がプラス）。どちらも -1〜1 で、長さは 1 まで
ACTION_AXIS ActionGetMove(void);

/// @brief 1人プレイのカメラを回す角度を取得する（マウスの移動量 ＋ 矢印キー ＋ パッド0の右スティック）
/// @return X：左右に回す角度（ラジアン。右がプラス）、Y：上下に回す角度（ラジアン。上がプラス）
ACTION_AXIS ActionGetLook(void);

/// @brief このフレームで押した数字キー 1〜6 の装備選択を取得する（Q・E の切り替えモード中はその手。モードでなければ Shift 同時押しで右手）
/// @param player プレイヤーのスロット番号（0〜3）。指定なしは1P。F1〜F4で選んだスロットにだけ入力が出る
/// @return 手と腕の番号。入力なしは両方 -1。同時に複数押したときは小さい番号を使う
ACTION_EQUIP_KEY ActionGetEquipKey(int player = 0);

/// @brief 腕の切り替えモード（Q：左手、E：右手）を取得する
/// @param player プレイヤーのスロット番号（0〜3）。指定なしは1P
/// @return 左 0・右 1・モードなし -1
int ActionGetEquipMode(int player = 0);

/// @brief F1〜F4で選んだ、キーボードによる腕の切り替え先を取得する
/// @return プレイヤーのスロット番号（0〜3。はじめは1P＝0）
int ActionGetEquipPlayer(void);
