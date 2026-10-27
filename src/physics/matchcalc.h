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
class MatchCalc
{
public:
	/// @brief 試合を最初の状態にして始める
	/// @param seconds 制限時間（秒）
	/// @param outCount 今の、ダミーが場外に落ちた回数の合計（ここから増えた分だけ得点になる）
	void Start(float seconds, int outCount);

	/// @brief 試合を1フレーム進める（得点を足してから、時間を進める）
	/// @param dt 進める秒数
	/// @param outCount 今の、ダミーが場外に落ちた回数の合計
	/// @return このフレームに起きたこと。時間切れのあとは何も起きず、得点も増えない
	MATCH_EVENT Step(float dt, int outCount);

	/// @brief 画面に出す残り秒数を取得する
	/// @return 残り秒数を切り上げた整数（始めは制限時間そのまま。時間切れで 0）
	int GetDisplaySeconds() const;

	/// @brief 試合の状態を取得する
	MATCH_STATE GetState() const { return m_State; }

	/// @brief 残り時間を取得する
	const TIMER& GetTimer() const { return m_Timer; }

	/// @brief 得点（場外にぶっ飛ばした回数）を取得する
	int GetScore() const { return m_Score; }

	/// @brief 前のフレームの、ダミーが場外に落ちた回数の合計（増えた分を数えるため）を取得する
	int GetLastOutCount() const { return m_LastOutCount; }

private:
	MATCH_STATE m_State{}; ///< 試合の状態
	TIMER m_Timer{}; ///< 残り時間
	int m_Score{}; ///< 得点（場外にぶっ飛ばした回数）
	int m_LastOutCount{}; ///< 前のフレームの、ダミーが場外に落ちた回数の合計（増えた分を数えるため）
};
