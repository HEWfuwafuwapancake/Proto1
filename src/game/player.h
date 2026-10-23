#pragma once

#include "core/dx11.h"

/// @brief このフレームで場外から復活したか（保持中の触腕を解除するため）
bool PlayerDidRespawn(void);

/// @file player.h
/// @brief プレイヤー（仮の形のカプセル）。浮島の上を歩く・ジャンプする・落ちる
/// @details 操作は「行動」で受け取る（移動：W/A/S/D・左スティック、ジャンプ：Space・×）。
///          移動の向きはカメラの左右の向きに合わせる。場外に落ちたら浮島の真ん中に戻す。
///          速さ・ジャンプ・重力・形は player.cpp の「調整用の値」、計算は physics/playercalc.h

/// @brief プレイヤーを浮島の真ん中に立たせる
void PlayerInitialize(void);

/// @brief プレイヤーの終了処理（今は何もしない）
void PlayerFinalize(void);

/// @brief 行動とカメラの向きから、プレイヤーを1フレーム分動かす
/// @details 毎フレーム、ActionUpdate のあと・CameraUpdate の前に呼ぶ（カメラは動いたあとのプレイヤーを追いかける）
void PlayerUpdate(void);

/// @brief プレイヤーを描く
void PlayerDraw(void);

/// @brief プレイヤーの足元の位置を取得する
/// @return 足元の位置（ワールド座標）
XMFLOAT3 PlayerGetPosition(void);

/// @brief プレイヤーの足元の位置を変える（ダミーの頭に乗ったときに押し出されるなど。速度はそのまま）
/// @param position 新しい足元の位置（ワールド座標）
void PlayerSetPosition(const XMFLOAT3& position);

/// @brief プレイヤーの高さ（カプセルの全体の高さ）を取得する
/// @return 高さ（カメラが頭のあたりを見るのに使う）
float PlayerGetHeight(void);
