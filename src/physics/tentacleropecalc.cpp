#include "physics/tentacleropecalc.h"
#include <algorithm>
#include <cmath>

// 調整用の値
static constexpr float ROPE_MIN_LENGTH = 0.00001f; // 向きを決められる最小の長さ

void TentacleRopeCalc::Reset(const VEC3& anchor, const VEC3& direction, const float* lengths, int count)
{
	m_Count = std::max(0, std::min(count, MAX_POINTS - 1));
	m_Total = 0.0f;
	m_Points[0] = m_Previous[0] = anchor;
	for (int i = 1; i <= m_Count; ++i) {
		m_Lengths[i] = std::max(0.0f, lengths[i - 1]);
		m_Total += m_Lengths[i];
		m_Points[i] = m_Previous[i] = Vec3Add(m_Points[i - 1], Vec3Scale(direction, m_Lengths[i]));
	}
}

void TentacleRopeCalc::Step(const VEC3& anchor, const TENTACLE_ROPE_PARAMS& params, float dt)
{
	if (m_Count <= 0) return;
	m_Points[0] = m_Previous[0] = anchor;
	if (dt <= 0.0f) return;

	// 1. 慣性（前のフレームからの動き）を空気抵抗で弱めて続け、重力を足す
	const float keep = std::max(0.0f, 1.0f - params.Drag * dt);
	for (int i = 1; i <= m_Count; ++i) {
		const VEC3 moved = Vec3Scale(Vec3Sub(m_Points[i], m_Previous[i]), keep);
		m_Previous[i] = m_Points[i];
		m_Points[i] = Vec3Add(Vec3Add(m_Points[i], moved), { 0.0f, -params.Gravity * dt * dt, 0.0f });
	}

	// 2. 根元から順に、前の節の向きへ少し寄せ（曲がりにくさ）、節の長さに戻す。体の中に入ったら外へ出す（体を優先）
	const VEC3 axis = Vec3Sub(params.BodyTop, params.BodyBottom);
	const float axisLength2 = Vec3Dot(axis, axis);
	for (int i = 1; i <= m_Count; ++i) {
		if (i >= 2) {
			const VEC3 previous = Vec3Sub(m_Points[i - 1], m_Points[i - 2]);
			const float length = Vec3Length(previous);
			if (length > ROPE_MIN_LENGTH) {
				const VEC3 straight = Vec3Add(m_Points[i - 1], Vec3Scale(previous, m_Lengths[i] / length));
				m_Points[i] = Vec3Add(Vec3Scale(m_Points[i], 1.0f - params.Stiffness), Vec3Scale(straight, params.Stiffness));
			}
		}
		const VEC3 link = Vec3Sub(m_Points[i], m_Points[i - 1]);
		const float length = Vec3Length(link);
		const VEC3 direction = length > ROPE_MIN_LENGTH ? Vec3Scale(link, 1.0f / length) : VEC3{ 0.0f, -1.0f, 0.0f };
		m_Points[i] = Vec3Add(m_Points[i - 1], Vec3Scale(direction, m_Lengths[i]));
		// 体の中に入ったら外へ出す（節の長さより優先する）
		if (params.BodyRadius > 0.0f && axisLength2 > ROPE_MIN_LENGTH) {
			const float t = std::max(0.0f, std::min(1.0f, Vec3Dot(Vec3Sub(m_Points[i], params.BodyBottom), axis) / axisLength2));
			const VEC3 closest = Vec3Add(params.BodyBottom, Vec3Scale(axis, t));
			const VEC3 away = Vec3Sub(m_Points[i], closest);
			const float distance = Vec3Length(away);
			if (distance < params.BodyRadius && distance > ROPE_MIN_LENGTH) {
				m_Points[i] = Vec3Add(closest, Vec3Scale(away, params.BodyRadius / distance));
			}
		}
	}
}

VEC3 TentacleRopeCalc::GetPoint(int index) const
{
	return m_Points[std::max(0, std::min(index, m_Count))];
}

VEC3 TentacleRopeCalc::GetPointAt(float ratio) const
{
	if (m_Count <= 0 || m_Total <= 0.0f) return m_Points[0];
	float remaining = std::max(0.0f, std::min(1.0f, ratio)) * m_Total;
	for (int i = 1; i <= m_Count; ++i) {
		if (remaining <= m_Lengths[i] || i == m_Count) {
			const float t = m_Lengths[i] > 0.0f ? std::min(1.0f, remaining / m_Lengths[i]) : 1.0f;
			return Vec3Add(m_Points[i - 1], Vec3Scale(Vec3Sub(m_Points[i], m_Points[i - 1]), t));
		}
		remaining -= m_Lengths[i];
	}
	return m_Points[m_Count];
}
