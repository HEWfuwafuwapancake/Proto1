#pragma once

#include "physics/matchcalc.h"

/// @file match.h
/// @brief 試合の流れ（制限時間 90 秒・ダミーを場外へぶっ飛ばすと +1・時間切れ・やり直し）
/// @details 時間切れのあとは、プレイヤー・ダミー・触腕が止まる（カメラは回せる）。
///          OPTIONS / R で、いつでも最初からやり直せる。制限時間は match.cpp の「調整用の値」、計算は physics/matchcalc.h

/// @brief 試合を始める（ダミーの初期化のあとに呼ぶ）
void MatchInitialize(void);

/// @brief 試合の終了処理（今は何もしない）
void MatchFinalize(void);

/// @brief やり直しの入力を調べてから、場外の回数を数えて時間を進める
/// @details 毎フレーム、DummyUpdate のあと（TentacleUpdate のあと）に呼ぶ。やり直すときは触腕・プレイヤー・ダミー・カメラを最初に戻す
void MatchUpdate(void);

/// @brief 試合中か取得する（時間切れのあとは、プレイヤー・ダミー・触腕を動かさないために使う）
/// @return 試合中なら true
bool MatchIsPlaying(void);

/// @brief 画面に出す残り秒数を取得する（No.15 の表示用）
/// @return 残り秒数（切り上げ。時間切れで 0）
int MatchGetRemainingSeconds(void);

/// @brief 得点を取得する（No.15 の表示用）
/// @return ダミーを場外へぶっ飛ばした回数
int MatchGetScore(void);

/// @brief 試合の状態を取得する（No.15 の表示用）
/// @return 試合中か、時間切れか
MATCH_STATE MatchGetState(void);
