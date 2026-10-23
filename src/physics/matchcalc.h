#pragma once

#include "core/timer.h"

/// @file matchcalc.h
/// @brief 試合の流れの計算（制限時間・場外で +1・時間切れ）
/// @details DirectX と Windows を使わないので、単体テスト（tests/Proto1Tests）からそのまま使える

/// @brief 試合の状態
enum MATCH_STATE
{
	MATCH_STATE_PLAYING,	///< 試合中
	MATCH_STATE_FINISHED,	///< 時間切れ（得点しない）
};

/// @brief MatchCalcStep で、このフレームに起きたこと
enum MATCH_EVENT
{
	MATCH_EVENT_NONE,		///< 何も起きていない
	MATCH_EVENT_SCORED,		///< 得点が増えた
	MATCH_EVENT_TIME_UP,	///< 時間切れになった（このフレームだけ。得点も増えていたときはこちらを返す）
};

/// @brief 試合1回分のデータ
struct MATCH
{
	MATCH_STATE State;		///< 試合の状態
	TIMER Timer;			///< 残り時間
	int Score;				///< 得点（場外にぶっ飛ばした回数）
	int LastOutCount;		///< 前のフレームの、ダミーが場外に落ちた回数の合計（増えた分を数えるため）
};

/// @brief 試合を最初の状態にして始める
/// @param[out] match 試合
/// @param seconds 制限時間（秒）
/// @param outCount 今の、ダミーが場外に落ちた回数の合計（ここから増えた分だけ得点になる）
void MatchCalcStart(MATCH& match, float seconds, int outCount);

/// @brief 試合を1フレーム進める（得点を足してから、時間を進める）
/// @param[in,out] match 試合
/// @param dt 進める秒数
/// @param outCount 今の、ダミーが場外に落ちた回数の合計
/// @return このフレームに起きたこと。時間切れのあとは何も起きず、得点も増えない
MATCH_EVENT MatchCalcStep(MATCH& match, float dt, int outCount);

/// @brief 画面に出す残り秒数を取得する
/// @param match 試合
/// @return 残り秒数を切り上げた整数（始めは制限時間そのまま。時間切れで 0）
int MatchCalcDisplaySeconds(const MATCH& match);
