#pragma once

#include "input/actioncalc.h"

/// @file action.h
/// @brief 入力を「行動」に変える（ゲームはキーやボタンではなく、行動を見る）
/// @details キーボード＋マウスでも、パッド（DUALSHOCK 4）でも、ゲームの側は同じように動く。
///          キーの割り当ては action.cpp の「調整用の値」にまとめてある

/// @brief 行動の種類（ボタン1つで起きるもの）
enum ACTION
{
	ACTION_JUMP,		///< ジャンプ（×／Space）
	ACTION_ARM_LEFT,	///< 左の触腕：伸ばしてつかむ／つかんでいる間にもう一度で投げる（L1／左クリック）
	ACTION_ARM_RIGHT,	///< 右の触腕：同上（R1／右クリック）
	ACTION_RESTART,		///< やり直し（OPTIONS／R）

	ACTION_COUNT		///< 行動の数（配列の大きさに使う。行動ではない）
};

/// @brief 行動の状態を最初に戻す（何も押していない状態）
void ActionInitialize(void);

/// @brief キーボード・マウス・パッドの状態から、このフレームの行動を決める
/// @details 毎フレーム、KeyboardUpdate・MouseUpdate・PadUpdate のあとに1回だけ呼ぶ
void ActionUpdate(void);

/// @brief 行動のボタンを押しているか
/// @param action 行動
/// @return 押している間ずっと true
bool ActionIsPress(ACTION action);

/// @brief 行動のボタンを押した瞬間か
/// @param action 行動
/// @return 押した瞬間の1フレームだけ true
bool ActionIsTrigger(ACTION action);

/// @brief 行動のボタンを離した瞬間か
/// @param action 行動
/// @return 離した瞬間の1フレームだけ true
bool ActionIsRelease(ACTION action);

/// @brief 移動の向きを取得する（W/A/S/D と左スティックの、強く入れているほう）
/// @return X：左右（右がプラス）、Y：前後（前がプラス）。どちらも -1〜1 で、長さは 1 まで
ACTION_AXIS ActionGetMove(void);

/// @brief このフレームでカメラを回す角度を取得する（マウスの移動量 ＋ 矢印キー ＋ 右スティック）
/// @return X：左右に回す角度（ラジアン。右がプラス）、Y：上下に回す角度（ラジアン。上がプラス）
ACTION_AXIS ActionGetLook(void);
