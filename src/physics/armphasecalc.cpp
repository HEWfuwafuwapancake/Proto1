#include "physics/armphasecalc.h"

// 調整用の値
static constexpr float TIME_EPSILON = 0.00001f; // 1/60 秒を何度も足したときの丸め誤差だけを吸収する。

void ArmPhaseCalc::Reset()
{
	*this = {};
}

void ArmPhaseCalc::Start(int phase)
{
	m_Phase = phase;
	m_Time = 0.0f;
	m_TickCount = 0;
}

ARM_PHASE_STEP ArmPhaseCalc::Step(float dt, const float* seconds, int count, float tickSeconds)
{
	ARM_PHASE_STEP step{ -1, false, 0 };
	if (dt <= 0.0f || m_Phase < 0 || seconds == nullptr) return step;
	if (m_Phase >= count) {
		Reset();
		step.Finished = true;
		return step;
	}
	m_Time += dt;
	if (tickSeconds > 0.0f) {
		const int total = static_cast<int>((m_Time + TIME_EPSILON) / tickSeconds);
		const int limit = static_cast<int>((seconds[m_Phase] + TIME_EPSILON) / tickSeconds);
		const int reached = total < limit ? total : limit;
		const int ticks = reached - m_TickCount;
		step.Ticks = ticks < ARM_PHASE_MAX_TICKS ? ticks : ARM_PHASE_MAX_TICKS;
		m_TickCount = reached;
	}
	if (m_Time + TIME_EPSILON < seconds[m_Phase]) return step;
	if (m_Phase + 1 < count) {
		Start(m_Phase + 1);
		step.Entered = m_Phase;
	}
	else {
		Reset();
		step.Finished = true;
	}
	return step;
}
