#include "physics/matchcalc.h"
#include <math.h>


void MatchCalcStart(MATCH& match, float seconds, int outCount)
{
	match.State = MATCH_STATE_PLAYING;
	TimerStart(match.Timer, seconds);
	match.Score = 0;
	match.LastOutCount = outCount;
}

MATCH_EVENT MatchCalcStep(MATCH& match, float dt, int outCount)
{
	if (match.State != MATCH_STATE_PLAYING) {
		return MATCH_EVENT_NONE;
	}

	// 前のフレームから増えた分だけ得点（時間切れと同じフレームの場外も入る）
	MATCH_EVENT event = MATCH_EVENT_NONE;
	if (outCount > match.LastOutCount) {
		match.Score += outCount - match.LastOutCount;
		event = MATCH_EVENT_SCORED;
	}
	match.LastOutCount = outCount;

	TimerUpdate(match.Timer, dt);
	if (TimerIsFinished(match.Timer)) {
		match.State = MATCH_STATE_FINISHED;
		event = MATCH_EVENT_TIME_UP;
	}
	return event;
}

int MatchCalcDisplaySeconds(const MATCH& match)
{
	// 浮動小数点の誤差で 90.0000001 → 91 にならないよう、少しだけ引いてから切り上げる
	return (int)ceilf(TimerGetRemaining(match.Timer) - 1.0e-4f);
}
