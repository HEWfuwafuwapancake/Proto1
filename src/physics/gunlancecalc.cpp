#include "physics/gunlancecalc.h"

// 調整用の値
static constexpr float SHOT_INTERVAL = 0.25f;
static constexpr float RUSH_SECONDS = 2.0f;
static constexpr float LIFT_SECONDS = 0.35f;
static constexpr float BURST_INTERVAL = 0.2f;
static constexpr int BURST_MAX_ROUNDS = 3;
static constexpr float TIME_EPSILON = 0.00001f; // フレームを刻んだときの丸め誤差だけを吸収する。

void GunlanceCalc::Reset()
{
	*this = {};
}

void GunlanceCalc::StopAction()
{
	m_Phase.Reset();
	m_BurstRounds = 0;
}

bool GunlanceCalc::CanFire(float dt) const
{
	return IsIdle() && m_ShotCooldown <= dt + TIME_EPSILON;
}

void GunlanceCalc::RecordShot()
{
	m_ShotCooldown = SHOT_INTERVAL;
}

void GunlanceCalc::StartRush()
{
	m_Phase.Start(Rush);
}

void GunlanceCalc::StartImpale(int remaining)
{
	m_BurstRounds = remaining < 0 || remaining > BURST_MAX_ROUNDS ? BURST_MAX_ROUNDS : remaining;
	m_Phase.Start(Lift);
}

GUNLANCE_STEP GunlanceCalc::Step(float dt, bool pressed)
{
	m_ShotCooldown = m_ShotCooldown > dt ? m_ShotCooldown - dt : 0.0f;
	if (IsRushing() && !pressed) StopAction();
	const float seconds[PhaseCount]{ RUSH_SECONDS, LIFT_SECONDS, BURST_INTERVAL * m_BurstRounds };
	const ARM_PHASE_STEP step = m_Phase.Step(dt, seconds, IsRushing() ? 1 : PhaseCount,
		m_Phase.GetPhase() == Burst ? BURST_INTERVAL : 0.0f);
	return { step.Ticks, step.Finished && step.Ticks > 0 };
}

float GunlanceCalc::GetLiftProgress() const
{
	return m_Phase.GetPhase() == Lift ? m_Phase.GetTime() / LIFT_SECONDS : 1.0f;
}
