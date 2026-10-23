#pragma once

#include "core/dx11.h"

/// @file dummy.h
/// @brief ダミーの敵（その場に立つだけのカプセルが3体。動かない・攻撃しない）
/// @details プレイヤーがぶつかると押される。場外に落ちたら、しばらく消えてから最初の場所に戻る。
///          置き場所・復活までの秒数・形は dummy.cpp の「調整用の値」、計算は physics/dummycalc.h

/// @brief ダミーの数
static constexpr int DUMMY_COUNT = 3;

/// @brief ダミーを最初の場所に立たせる（StageInitialize のあとに呼ぶ）
void DummyInitialize(void);

/// @brief ダミーの終了処理（今は何もしない）
void DummyFinalize(void);

/// @brief ダミーを1フレーム分動かす（重力・着地・プレイヤーに押される・場外と復活）
/// @details 毎フレーム、PlayerUpdate のあとに呼ぶ（動いたあとのプレイヤーに押される）
void DummyUpdate(void);

/// @brief ダミーを描く（復活を待っている間は描かない）
void DummyDraw(void);

/// @brief ダミー全員が場外に落ちた回数の合計を取得する（No.14 のスコアに使う）
/// @return 落ちた回数の合計
int DummyGetTotalOutCount(void);
