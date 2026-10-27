#include "physics/matchcalc.h"
#include <math.h>


void MatchCalc::Start(float seconds, int outCount)
{
	m_State = MATCH_STATE_PLAYING;
	TimerStart(m_Timer, seconds);
	m_Score = 0;
	m_LastOutCount = outCount;
}

MATCH_EVENT MatchCalc::Step(float dt, int outCount)
{
	if (m_State != MATCH_STATE_PLAYING) {
		return MATCH_EVENT_NONE;
	}

	// 前のフレームから増えた分だけ得点（時間切れと同じフレームの場外も入る）
	MATCH_EVENT event = MATCH_EVENT_NONE;
	if (outCount > m_LastOutCount) {
		m_Score += outCount - m_LastOutCount;
		event = MATCH_EVENT_SCORED;
	}
	m_LastOutCount = outCount;

	TimerUpdate(m_Timer, dt);
	if (TimerIsFinished(m_Timer)) {
		m_State = MATCH_STATE_FINISHED;
		event = MATCH_EVENT_TIME_UP;
	}
	return event;
}

int MatchCalc::GetDisplaySeconds() const
{
	// 浮動小数点の誤差で 90.0000001 → 91 にならないよう、少しだけ引いてから切り上げる
	return (int)ceilf(TimerGetRemaining(m_Timer) - 1.0e-4f);
}
