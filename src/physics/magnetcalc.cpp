#include "physics/magnetcalc.h"

// 調整用の値
static constexpr float TIME_EPSILON = 0.000001f; // 秒数を足したときの丸め誤差だけを吸収する。

CAPSULE MagnetCalcBeam(const VEC3& shoulder, const VEC3& aim, const MAGNET_PARAMS& params)
{
	const VEC3 direction = TentacleCalcAim({}, {}, false, aim);
	return { shoulder, Vec3Add(shoulder, Vec3Scale(direction, params.Range)), params.BeamRadius };
}

void MagnetCalc::Reset()
{
	*this = {};
}

void MagnetCalc::Cooldown()
{
	m_State = MAGNET_COOLDOWN;
	m_Time = 0.0f;
	m_Target = -1;
}

MAGNET_EVENT MagnetCalc::Step(const VEC3& shoulder, const VEC3& aim, const VEC3& holdPosition,
	bool trigger, bool cancel, float dt, const MAGNET_PARAMS& params,
	const TENTACLE_TARGET* targets, int count)
{
	MAGNET_EVENT event{ -1, -1, false };
	const bool holding = m_State == MAGNET_PULLING || m_State == MAGNET_HOLDING;
	if (cancel || (holding && (targets == nullptr || m_Target < 0 || m_Target >= count))) {
		event.Released = m_Target;
		if (m_State != MAGNET_IDLE && m_State != MAGNET_COOLDOWN) Cooldown();
		return event;
	}
	if (dt <= 0.0f) return event;
	if (m_State == MAGNET_IDLE) {
		if (!trigger) return event;
		const CAPSULE beam = MagnetCalcBeam(shoulder, aim, params);
		int nearest = -1;
		float nearestDistance = 0.0f;
		for (int i = 0; targets != nullptr && i < count; ++i) {
			if (!targets[i].Available || !CollisionCapsuleCapsule(beam, targets[i].Body).Hit) continue;
			const VEC3 center = Vec3Scale(Vec3Add(targets[i].Body.Start, targets[i].Body.End), 0.5f);
			const float distance = Vec3LengthSq(Vec3Sub(center, shoulder));
			if (nearest < 0 || distance < nearestDistance) {
				nearest = i;
				nearestDistance = distance;
			}
		}
		if (nearest < 0) {
			Cooldown();
			return event;
		}
		m_State = MAGNET_PULLING;
		m_Time = 0.0f;
		m_Target = nearest;
		m_Center = Vec3Scale(Vec3Add(targets[nearest].Body.Start, targets[nearest].Body.End), 0.5f);
		event.Grabbed = nearest;
		return event;
	}
	if (holding) {
		if (trigger) {
			event.Released = m_Target;
			event.Repel = true;
			Cooldown();
			return event;
		}
		m_Time += dt;
		if (m_State == MAGNET_PULLING) {
			const VEC3 delta = Vec3Sub(holdPosition, m_Center);
			const float distance = Vec3Length(delta);
			const float step = params.PullSpeed * dt;
			// 到着したら引き寄せ先にそろえる。長いフレームでも行き過ぎない。
			if (distance <= step + params.ArrivalDistance) {
				m_Center = holdPosition;
				m_State = MAGNET_HOLDING;
				m_Time = 0.0f;
			}
			else m_Center = Vec3Add(m_Center, Vec3Scale(delta, step / distance));
		}
		else {
			m_Center = holdPosition;
			if (m_Time + TIME_EPSILON >= params.HoldSeconds) {
				event.Released = m_Target;
				Cooldown();
			}
		}
	}
	else if (m_State == MAGNET_COOLDOWN) {
		m_Time += dt;
		if (m_Time + TIME_EPSILON >= params.CooldownSeconds) Reset();
	}
	return event;
}
