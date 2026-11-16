#include "physics/lasercalc.h"
#include "physics/areacalc.h"
#include "physics/collision.h"

void LaserCalc::Reset()
{
	m_Charging = false;
	m_Seconds = 0.0f;
	m_Recover = 0.0f;
	m_LastFire = LASER_FIRE_NONE;
}

LASER_FIRE LaserCalc::Fire(LASER_FIRE fire, const LASER_PARAMS& params)
{
	m_Charging = false;
	m_Seconds = 0.0f;
	m_LastFire = fire;
	m_Recover = fire == LASER_FIRE_CANNON ? params.CannonRecoverSeconds : params.ShotRecoverSeconds;
	return fire;
}

LASER_FIRE LaserCalc::Step(const ARM_INPUT& input, float dt, const LASER_PARAMS& params)
{
	if (dt < 0.0f) dt = 0.0f;
	m_Recover = m_Recover > dt ? m_Recover - dt : 0.0f;
	if (m_Charging) {
		m_Seconds += dt;
		// 離したとき：溜めきっていれば砲、まだなら単発。
		if (input.HoldReleased) return Fire(m_Seconds >= params.ChargeSeconds ? LASER_FIRE_CANNON : LASER_FIRE_SHOT, params);
		// 押していないのに離したことにならない（ガードなどで入力を取り上げられた）ときは、撃たずにやめる。
		if (!input.Pressed) {
			m_Charging = false;
			m_Seconds = 0.0f;
		}
		return LASER_FIRE_NONE;
	}
	if (m_Recover > 0.0f) return LASER_FIRE_NONE;
	if (input.Tap) return Fire(LASER_FIRE_SHOT, params);
	if (input.HoldStarted) {
		// 長押しに達したのと同じフレームに離していれば（休みの間ずっと押していた）、単発にする。
		if (input.HoldReleased) return Fire(LASER_FIRE_SHOT, params);
		m_Charging = true;
		m_Seconds = params.HoldStartSeconds;
	}
	return LASER_FIRE_NONE;
}

float LaserCalc::GetCharge(const LASER_PARAMS& params) const
{
	if (!m_Charging || params.ChargeSeconds <= 0.0f) return 0.0f;
	const float rate = m_Seconds / params.ChargeSeconds;
	return rate > 1.0f ? 1.0f : rate;
}

VEC3 LaserCalcAimPoint(const VEC3& eye, const VEC3& direction, float length, float radius, const TENTACLE_TARGET* targets, int count)
{
	float distance = length;
	AreaCalcFirstOnRay(eye, direction, length, radius, targets, count, &distance);
	return Vec3Add(eye, Vec3Scale(direction, distance));
}

int LaserCalcPierce(const VEC3& origin, const VEC3& direction, float length, float radius,
	const TENTACLE_TARGET* targets, int count, bool* hits)
{
	const CAPSULE beam{ origin, Vec3Add(origin, Vec3Scale(direction, length)), radius };
	int total = 0;
	for (int i = 0; i < count; ++i) {
		hits[i] = targets[i].Available && CollisionCapsuleCapsule(beam, targets[i].Body).Hit;
		if (hits[i]) ++total;
	}
	return total;
}
