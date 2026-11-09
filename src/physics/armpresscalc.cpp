#include "physics/armpresscalc.h"

// 調整用の値
static constexpr double HOLD_SECONDS = 0.2;

void ArmPressCalc::Reset(bool pressed)
{
	m_State = pressed ? State::WaitForRelease : State::Idle;
	m_Seconds = 0.0;
}

ARM_INPUT ArmPressCalc::Step(bool pressed, float dt)
{
	ARM_INPUT input{};
	input.Pressed = pressed;
	if (!pressed) {
		input.Tap = m_State == State::Timing;
		Reset();
	}
	else if (m_State != State::WaitForRelease) {
		m_State = State::Timing;
		m_Seconds += dt;
		if (m_Seconds >= HOLD_SECONDS) {
			input.HoldStarted = true;
			m_State = State::WaitForRelease;
		}
	}
	return input;
}
