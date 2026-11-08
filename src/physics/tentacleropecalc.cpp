#include "physics/tentacleropecalc.h"
#include <algorithm>
#include <cmath>

// 調整用の値
static constexpr float ROPE_MIN_LENGTH = 0.00001f; // 向きを決められる最小の長さ

static VEC3 UnitOr(const VEC3& v, const VEC3& fallback)
{
	const float length = Vec3Length(v);
	return length > ROPE_MIN_LENGTH ? Vec3Scale(v, 1.0f / length) : fallback;
}

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

void TentacleRopeCalc::SolveLink(int index)
{
	// 節の長さに戻す。根元は動かさず、ほかは同じ重さとして両側を半分ずつ動かす。
	const VEC3 link = Vec3Sub(m_Points[index], m_Points[index - 1]);
	const float length = Vec3Length(link);
	if (length < ROPE_MIN_LENGTH) return;
	const VEC3 correction = Vec3Scale(link, (length - m_Lengths[index]) / length);
	if (index == 1) {
		m_Points[index] = Vec3Sub(m_Points[index], correction);
	} else {
		m_Points[index - 1] = Vec3Add(m_Points[index - 1], Vec3Scale(correction, 0.5f));
		m_Points[index] = Vec3Sub(m_Points[index], Vec3Scale(correction, 0.5f));
	}
}

void TentacleRopeCalc::LimitBend(int index, const VEC3& previousDirection, float maxBend)
{
	// 1つ前の節の向きから maxBend より曲がっていたら、上限の角度まで戻す（鎖のつなぎ目）。
	const VEC3 link = Vec3Sub(m_Points[index], m_Points[index - 1]);
	const float length = Vec3Length(link);
	if (length < ROPE_MIN_LENGTH) return;
	const VEC3 direction = Vec3Scale(link, 1.0f / length);
	const float cosine = Vec3Dot(direction, previousDirection);
	const float cosMax = cosf(maxBend);
	if (cosine >= cosMax) return;
	const VEC3 side = UnitOr(Vec3Sub(direction, Vec3Scale(previousDirection, cosine)), { 1.0f, 0.0f, 0.0f });
	const VEC3 limited = Vec3Add(Vec3Scale(previousDirection, cosMax), Vec3Scale(side, sinf(maxBend)));
	const VEC3 corrected = Vec3Add(m_Points[index - 1], Vec3Scale(limited, length));
	// 上限に当たって戻した分は勢いにしない（跳ね返ってガチャガチャしないように）。
	m_Previous[index] = Vec3Add(m_Previous[index], Vec3Sub(corrected, m_Points[index]));
	m_Points[index] = corrected;
}

void TentacleRopeCalc::PushOutOfBody(int index, const TENTACLE_ROPE_PARAMS& params)
{
	if (params.BodyRadius <= 0.0f) return;
	const VEC3 axis = Vec3Sub(params.BodyTop, params.BodyBottom);
	const float axisLength2 = Vec3Dot(axis, axis);
	if (axisLength2 < ROPE_MIN_LENGTH) return;
	const float t = std::max(0.0f, std::min(1.0f, Vec3Dot(Vec3Sub(m_Points[index], params.BodyBottom), axis) / axisLength2));
	const VEC3 closest = Vec3Add(params.BodyBottom, Vec3Scale(axis, t));
	const VEC3 away = Vec3Sub(m_Points[index], closest);
	const float distance = Vec3Length(away);
	if (distance < params.BodyRadius && distance > ROPE_MIN_LENGTH) {
		m_Points[index] = Vec3Add(closest, Vec3Scale(away, params.BodyRadius / distance));
	}
}

void TentacleRopeCalc::Step(const VEC3& anchor, const TENTACLE_ROPE_PARAMS& params, float dt)
{
	if (m_Count <= 0) return;
	if (dt <= 0.0f) {
		m_Points[0] = m_Previous[0] = anchor;
		return;
	}
	const int substeps = std::max(1, params.Substeps);
	const int iterations = std::max(1, params.Iterations);
	const float h = dt / substeps;
	const VEC3 start = m_Points[0];
	// 根元の1回分の動き。揺れはこれとの差だけを減らす（体と一緒に動く分は減らさない）。
	const VEC3 anchorMove = Vec3Scale(Vec3Sub(anchor, start), 1.0f / substeps);
	const float relativeKeep = std::max(0.0f, 1.0f - params.Damping * h);
	const float airKeep = std::max(0.0f, 1.0f - params.AirDrag * h);
	const VEC3 gravity{ 0.0f, -params.Gravity * h * h, 0.0f };
	for (int step = 1; step <= substeps; ++step) {
		m_Points[0] = m_Previous[0] = Vec3Add(start, Vec3Scale(Vec3Sub(anchor, start), static_cast<float>(step) / substeps));

		// 1. 勢い（前の位置からの動き）を続ける。根元に対する揺れと、空気に対する動きを少しずつ減らし、重力を足す
		for (int i = 1; i <= m_Count; ++i) {
			VEC3 moved = Vec3Sub(m_Points[i], m_Previous[i]);
			moved = Vec3Add(anchorMove, Vec3Scale(Vec3Sub(moved, anchorMove), relativeKeep));
			moved = Vec3Scale(moved, airKeep);
			m_Previous[i] = m_Points[i];
			m_Points[i] = Vec3Add(Vec3Add(m_Points[i], moved), gravity);
		}

		// 2. 節の長さ・つなぎ目の曲がる上限・体へのめり込みを、根元から順に何回か合わせる
		for (int iteration = 0; iteration < iterations; ++iteration) {
			for (int i = 1; i <= m_Count; ++i) {
				SolveLink(i);
				const VEC3 previousDirection = i == 1 ? params.RootDirection
					: UnitOr(Vec3Sub(m_Points[i - 1], m_Points[i - 2]), params.RootDirection);
				LimitBend(i, previousDirection, i == 1 ? params.MaxRootBend : (i == m_Count ? params.MaxTipBend : params.MaxBend));
				PushOutOfBody(i, params);
			}
		}
		// 3. 最後に体へのめり込みをもう一度直す（節の長さ合わせで引き戻された点があっても体を優先する）
		for (int i = 1; i <= m_Count; ++i) PushOutOfBody(i, params);
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
