#include "core/timer.h"


void TimerStart(TIMER& timer, float seconds)
{
	if (seconds < 0.0f) {
		seconds = 0.0f;
	}
	timer.Duration = seconds;
	timer.Remaining = seconds;
}

void TimerUpdate(TIMER& timer, float dt)
{
	// 0 を過ぎても負にしない（残り時間をそのまま画面に出せるように）
	timer.Remaining -= dt;
	if (timer.Remaining < 0.0f) {
		timer.Remaining = 0.0f;
	}
}

float TimerGetRemaining(const TIMER& timer)
{
	return timer.Remaining;
}

bool TimerIsFinished(const TIMER& timer)
{
	return timer.Remaining <= 0.0f;
}
