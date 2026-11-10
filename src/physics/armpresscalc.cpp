#include "physics/armpresscalc.h"

// 調整用の値
static constexpr double HOLD_SECONDS = 0.25; // これより短く離すと単押し、押し続けると長押し。
static constexpr double TAP_BUFFER_SECONDS = 0.2; // 腕の動作中に押した単押しを覚えておく秒数。
static constexpr double TIME_EPSILON = 0.00001; // 1/60 秒を足し続けたときの丸め誤差だけを吸収する。

void ArmPressCalc::Reset(bool pressed)
{
	m_State = pressed ? State::WaitForRelease : State::Idle;
	m_Seconds = 0.0;
	m_TapBuffer = 0.0;
}

ARM_INPUT ArmPressCalc::Step(bool pressed, float dt, bool canTap, bool canHold)
{
	ARM_INPUT input{};
	input.Pressed = pressed;
	m_TapBuffer = m_TapBuffer > dt ? m_TapBuffer - dt : 0.0;
	if (!pressed) {
		if (m_State == State::Timing) m_TapBuffer = TAP_BUFFER_SECONDS;
		m_State = State::Idle;
		m_Seconds = 0.0;
	}
	else if (m_State == State::Idle || m_State == State::Timing) {
		m_State = State::Timing;
		m_Seconds += dt;
		if (m_Seconds + TIME_EPSILON >= HOLD_SECONDS) m_State = State::HoldWaiting;
	}
	if (m_State == State::HoldWaiting && canHold) {
		input.HoldStarted = true;
		m_State = State::WaitForRelease;
		m_TapBuffer = 0.0;
	}
	if (m_TapBuffer > 0.0 && canTap) {
		input.Tap = true;
		m_TapBuffer = 0.0;
	}
	return input;
}
