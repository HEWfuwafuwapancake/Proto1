#pragma once

#include "core/dx11.h"
#include "physics/tentaclecalc.h"

/// @file dummy.h
/// @brief ダミーの敵（その場に立つだけのカプセルが3体。動かない・攻撃しない）
/// @details プレイヤーがぶつかると押される。攻撃されるとダメージ%が増え、% が高いほど遠くへ吹っ飛ぶ（DummyHit）。
///          場外に落ちたら、しばらく消えてから最初の場所に 0% で戻る。
///          置き場所・復活までの秒数・形・吹っ飛びは dummy.cpp の「調整用の値」、計算は physics/dummycalc.h・physics/knockbackcalc.h

/// @brief ダミーの数
static constexpr int DUMMY_COUNT = 3;

/// @brief ダミーを最初の場所に立たせる（StageInitialize のあとに呼ぶ）
void DummyInitialize(void);

/// @brief ダミーの終了処理（今は何もしない）
void DummyFinalize(void);

/// @brief ダミーを1フレーム分動かす（重力・着地・プレイヤーに押される・吹っ飛ばされている間の動き・場外と復活）
/// @details 毎フレーム、PlayerUpdate のあとに呼ぶ（動いたあとのプレイヤーに押される）
void DummyUpdate(void);

/// @brief ダミーを描く（復活を待っている間は描かない）
void DummyDraw(void);

/// @brief ダミーを攻撃する（ダメージ%を足して、% に応じた速さで吹っ飛ばす）
/// @details スマブラと同じく、ダメージを足してから、足したあとの % で吹っ飛ぶ速さを決める。
///          吹っ飛ぶ向きは攻撃の向き（横）に少し上向きの角度をつけたもの。復活を待っているダミーには当たらない
/// @param index ダミーの番号（0〜DUMMY_COUNT-1）
/// @param direction 攻撃の向き（攻撃した側から、ダミーへ。上下の向きと長さは使わない）
/// @param damage ダメージ（%）
/// @param power 攻撃の強さ（1 でふつう。大きいほど速く飛ぶ）
void DummyHit(int index, const XMFLOAT3& direction, float damage, float power);

/// @brief 敵の体とつかめる状態を取得する（無効な番号なら空の候補）
TENTACLE_TARGET DummyGetTarget(int index);
/// @brief 敵が生存しているか取得する
bool DummyIsActive(int index);
/// @brief 敵をつかむ。ほかの腕が保持中、または消えていれば false
bool DummyGrab(int index);
/// @brief 保持中の敵の中心位置を設定する
void DummyMoveHeld(int index, const VEC3& center);
/// @brief 保持を解除する。throwTarget が true なら指定した3D方向へ投げ、false ならダメージなしで離す
void DummyRelease(int index, bool throwTarget, const VEC3& direction, float damage, float power);

/// @brief ダミーのダメージ%を取得する（画面表示に使う）
/// @param index ダミーの番号（0〜DUMMY_COUNT-1）
/// @return ダメージ%（無効な番号なら 0）
float DummyGetPercent(int index);

/// @brief ダミー全員が場外に落ちた回数の合計を取得する（No.14 のスコアに使う）
/// @return 落ちた回数の合計
int DummyGetTotalOutCount(void);
