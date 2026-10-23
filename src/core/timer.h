#pragma once

/// @file timer.h
/// @brief 「○秒数える」タイマー（制限時間・ダミーの復活までの時間などに使う）
/// @details DirectX と Windows を使わないので、単体テスト（tests/Proto1Tests）からそのまま使える

/// @brief タイマー1つ分のデータ
/// @details { } で初期化すると「数え終わった」状態になる。TimerStart で数え始める
struct TIMER
{
	float Duration;		///< 数える長さ（秒）。TimerStart で渡した秒数
	float Remaining;	///< 残りの秒数（0 より小さくならない）
};

/// @brief タイマーを数え始める（数えている途中でも、最初からやり直す）
/// @param[out] timer タイマー
/// @param seconds 数える秒数。負の値は 0 にする（すぐに数え終わる）
void TimerStart(TIMER& timer, float seconds);

/// @brief タイマーを進める（毎フレーム呼ぶ）
/// @param[in,out] timer タイマー
/// @param dt 進める秒数（ふつうは GameTimeGetDeltaTime() の値）
void TimerUpdate(TIMER& timer, float dt);

/// @brief 残りの秒数を取得する
/// @param timer タイマー
/// @return 残りの秒数（数え終わっていたら 0）
float TimerGetRemaining(const TIMER& timer);

/// @brief 数え終わったか調べる
/// @param timer タイマー
/// @return 残りが 0 なら true
bool TimerIsFinished(const TIMER& timer);
