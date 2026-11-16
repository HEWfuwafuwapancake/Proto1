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
	m_Held = false;
	m_Skip = false;
}

void ArmPhaseCalc::Release()
{
	if (!m_Held) return;
	m_Held = false;
	m_Skip = true;
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
	// 離した次のフレームは、残りのためを飛ばして段階を終える。
	if (m_Skip) {
		m_Skip = false;
		if (m_Time < seconds[m_Phase]) m_Time = seconds[m_Phase];
	}
	// 押している間は、段階の終わりの時間で止めて待つ。
	if (m_Held && m_Time > seconds[m_Phase]) m_Time = seconds[m_Phase];
	if (tickSeconds > 0.0f) {
		const int total = static_cast<int>((m_Time + TIME_EPSILON) / tickSeconds);
		const int limit = static_cast<int>((seconds[m_Phase] + TIME_EPSILON) / tickSeconds);
		const int reached = total < limit ? total : limit;
		const int ticks = reached - m_TickCount;
		step.Ticks = ticks < ARM_PHASE_MAX_TICKS ? ticks : ARM_PHASE_MAX_TICKS;
		m_TickCount = reached;
	}
	if (m_Held || m_Time + TIME_EPSILON < seconds[m_Phase]) return step;
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
