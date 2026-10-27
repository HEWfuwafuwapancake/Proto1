#include "physics/shieldcalc.h"
#include "physics/tentaclecalc.h"

// 調整用の値
static constexpr float TIME_EPSILON = 0.00001f; // 60fpsで2秒を足したときの丸め誤差だけを吸収する。

CAPSULE ShieldCalcBashCapsule(const VEC3& shoulder, const VEC3& forward, const SHIELD_PARAMS& params)
{
	const VEC3 direction = TentacleCalcAim({}, {}, false, forward);
	return { shoulder, Vec3Add(shoulder, Vec3Scale(direction, params.BashRange)), params.BashRadius };
}

VEC3 ShieldCalcPushVelocity(const VEC3& forward, float speed, float up)
{
	const VEC3 direction = Vec3Add(TentacleCalcAim({}, {}, false, forward), { 0.0f, up, 0.0f });
	return Vec3Scale(direction, speed / Vec3Length(direction));
}

void ShieldCalc::Reset()
{
	*this = {};
}

void ShieldCalc::Cooldown()
{
	m_State = SHIELD_COOLDOWN;
	m_Time = 0.0f;
}

bool ShieldCalc::Step(bool trigger, bool cancel, float dt, const SHIELD_PARAMS& params)
{
	if (cancel) {
		if (m_State != SHIELD_COOLDOWN) Cooldown();
		return false;
	}
	if (dt <= 0.0f) return false;
	if (m_State == SHIELD_IDLE) {
		if (trigger) {
			m_State = SHIELD_GUARDING;
			m_Time = 0.0f;
		}
		return false;
	}
	if (m_State == SHIELD_GUARDING && trigger) {
		m_State = SHIELD_BASHING;
		m_Time = 0.0f;
		return true;
	}
	m_Time += dt;
	if (m_State == SHIELD_GUARDING && m_Time + TIME_EPSILON >= params.GuardSeconds) Cooldown();
	else if (m_State == SHIELD_BASHING && m_Time + TIME_EPSILON >= params.BashSeconds) Cooldown();
	else if (m_State == SHIELD_COOLDOWN && m_Time + TIME_EPSILON >= params.CooldownSeconds) Reset();
	return false;
}
