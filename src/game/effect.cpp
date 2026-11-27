#include "game/effect.h"
#include "core/gametime.h"
#include "render/shape.h"
#include <math.h>

namespace
{
	/// @brief 粒の飛ばし方
	enum EFFECT_SPREAD
	{
		EFFECT_SPREAD_CONE, ///< 向きのまわりの円すいの中へ飛ばす（180° 以上で全方向）
		EFFECT_SPREAD_RING, ///< 向きに垂直な面の中で、等しい間隔で外へ広げる
		EFFECT_SPREAD_STAY, ///< その場にとどまる（広がって消える光）
	};

	/// @brief 1つの型の出し方
	struct EFFECT_DESC
	{
		int Count; ///< 出す粒の数
		EFFECT_SPREAD Spread; ///< 飛ばし方
		float SpreadAngle; ///< 円すいの半分の角度（ラジアン。EFFECT_SPREAD_CONE だけ）
		float MinSpeed; ///< いちばん遅い速さ
		float MaxSpeed; ///< いちばん速い速さ
		float Gravity; ///< 重力
		float Drag; ///< 1秒あたりに速さを減らす割合
		float MinLife; ///< いちばん短い寿命（秒）
		float MaxLife; ///< いちばん長い寿命（秒）
		float StartSize; ///< 出たときの大きさ
		float EndSize; ///< 消えるときの大きさ
		PARTICLE_COLOR StartColor; ///< 出たときの色
		PARTICLE_COLOR EndColor; ///< 消えるときの色
		PARTICLE_SHAPE Shape; ///< 形
	};

	// 調整用の値（plan/11_エフェクトと手ごたえ の 2-1・2-5）
	constexpr float PI = 3.14159265f;
	constexpr PARTICLE_COLOR SPARK_COLOR{ 1.0f, 0.97f, 0.8f, 1.0f };
	constexpr PARTICLE_COLOR SPARK_END_COLOR{ 1.0f, 0.65f, 0.2f, 0.0f };
	constexpr PARTICLE_COLOR RING_COLOR{ 1.0f, 1.0f, 1.0f, 0.85f };
	constexpr PARTICLE_COLOR RING_END_COLOR{ 1.0f, 1.0f, 1.0f, 0.0f };
	constexpr EFFECT_DESC EFFECT_TABLE[EFFECT_COUNT] = {
		// 数, 飛ばし方, 角度, 速さ, 重力, 減り, 寿命, 大きさ, 色, 形
		{ 6, EFFECT_SPREAD_CONE, PI * 0.45f, 4.0f, 8.0f, 12.0f, 4.0f, 0.12f, 0.22f, 0.07f, 0.0f, SPARK_COLOR, SPARK_END_COLOR, PARTICLE_SHAPE_SPHERE }, // 弱い火花
		{ 10, EFFECT_SPREAD_CONE, PI * 0.5f, 6.0f, 11.0f, 12.0f, 3.0f, 0.16f, 0.28f, 0.09f, 0.0f, SPARK_COLOR, SPARK_END_COLOR, PARTICLE_SHAPE_SPHERE }, // 中の火花
		{ 18, EFFECT_SPREAD_CONE, PI * 0.55f, 8.0f, 15.0f, 14.0f, 2.5f, 0.2f, 0.36f, 0.12f, 0.0f, SPARK_COLOR, SPARK_END_COLOR, PARTICLE_SHAPE_SPHERE }, // 強い火花
		{ 16, EFFECT_SPREAD_RING, 0.0f, 7.0f, 7.0f, 0.0f, 9.0f, 0.18f, 0.18f, 0.07f, 0.03f, RING_COLOR, RING_END_COLOR, PARTICLE_SHAPE_SPHERE }, // 輪
		{ 24, EFFECT_SPREAD_RING, 0.0f, 11.0f, 11.0f, 0.0f, 8.0f, 0.24f, 0.24f, 0.1f, 0.04f, RING_COLOR, RING_END_COLOR, PARTICLE_SHAPE_SPHERE }, // 大きな輪
		{ 14, EFFECT_SPREAD_CONE, PI * 0.06f, 16.0f, 30.0f, 0.0f, 2.0f, 0.3f, 0.45f, 0.1f, 0.02f, SPARK_COLOR, { 1.0f, 0.9f, 0.5f, 0.0f }, PARTICLE_SHAPE_SPHERE }, // 決め手の筋
		{ 1, EFFECT_SPREAD_STAY, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.3f, 0.3f, 0.3f, 0.8f, { 0.6f, 0.6f, 0.6f, 1.0f }, { 0.6f, 0.6f, 0.6f, 0.0f }, PARTICLE_SHAPE_SPHERE }, // ガードの光（No.132 から）
		{ 1, EFFECT_SPREAD_STAY, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.3f, 0.3f, 0.3f, 1.2f, { 1.0f, 0.9f, 0.2f, 1.0f }, { 1.0f, 0.9f, 0.2f, 0.0f }, PARTICLE_SHAPE_SPHERE }, // パリィの光（No.132 から）
	};
	constexpr uint32_t EFFECT_SEED = 20261127u; // 粒のばらつき専用の乱数の種

	VEC3 Normalize(const VEC3& v, const VEC3& fallback)
	{
		const float length = Vec3Length(v);
		return length > 1.0e-5f ? Vec3Scale(v, 1.0f / length) : fallback;
	}

	VEC3 Cross(const VEC3& a, const VEC3& b)
	{
		return { a.Y * b.Z - a.Z * b.Y, a.Z * b.X - a.X * b.Z, a.X * b.Y - a.Y * b.X };
	}

	// 向きに垂直な2本の向き（輪の面・円すいの広がりに使う）。
	void MakeBasis(const VEC3& axis, VEC3* side, VEC3* up)
	{
		const VEC3 helper = fabsf(axis.Y) < 0.9f ? VEC3{ 0.0f, 1.0f, 0.0f } : VEC3{ 1.0f, 0.0f, 0.0f };
		*side = Normalize(Cross(helper, axis), { 1.0f, 0.0f, 0.0f });
		*up = Cross(axis, *side);
	}
}

void EffectManager::Initialize()
{
	m_Random.SetSeed(EFFECT_SEED);
	Reset();
}

void EffectManager::Reset()
{
	m_Particles.Reset();
}

void EffectManager::Finalize()
{
}

void EffectManager::Update()
{
	m_Particles.Step(GameTimeGetDeltaTime());
}

VEC3 EffectManager::RandomInCone(const VEC3& axis, float halfAngle)
{
	VEC3 side, up;
	MakeBasis(axis, &side, &up);
	// 円すいの中で、向きからの角度と、まわりの角度を選ぶ。
	const float tilt = acosf(1.0f - m_Random.NextFloat(0.0f, 1.0f) * (1.0f - cosf(halfAngle)));
	const float around = m_Random.NextFloat(0.0f, 2.0f * PI);
	const VEC3 spread = Vec3Add(Vec3Scale(side, cosf(around)), Vec3Scale(up, sinf(around)));
	return Vec3Add(Vec3Scale(axis, cosf(tilt)), Vec3Scale(spread, sinf(tilt)));
}

void EffectManager::Spawn(EFFECT_ID id, const VEC3& position, const VEC3& direction)
{
	if (id < 0 || id >= EFFECT_COUNT) return;
	const EFFECT_DESC& desc = EFFECT_TABLE[id];
	const VEC3 axis = Normalize(direction, { 0.0f, 1.0f, 0.0f });
	VEC3 side, up;
	MakeBasis(axis, &side, &up);
	for (int i = 0; i < desc.Count; ++i) {
		PARTICLE particle;
		particle.Position = position;
		VEC3 heading{};
		if (desc.Spread == EFFECT_SPREAD_CONE) heading = RandomInCone(axis, desc.SpreadAngle);
		else if (desc.Spread == EFFECT_SPREAD_RING) {
			const float angle = 2.0f * PI * (float)i / (float)desc.Count;
			heading = Vec3Add(Vec3Scale(side, cosf(angle)), Vec3Scale(up, sinf(angle)));
		}
		particle.Velocity = Vec3Scale(heading, m_Random.NextFloat(desc.MinSpeed, desc.MaxSpeed));
		particle.Gravity = desc.Gravity;
		particle.Drag = desc.Drag;
		particle.Life = m_Random.NextFloat(desc.MinLife, desc.MaxLife);
		particle.StartSize = desc.StartSize;
		particle.EndSize = desc.EndSize;
		particle.StartColor = desc.StartColor;
		particle.EndColor = desc.EndColor;
		particle.Shape = desc.Shape;
		m_Particles.Spawn(particle);
	}
}

void EffectManager::Draw() const
{
	for (int i = 0; i < m_Particles.GetCapacity(); ++i) {
		if (!m_Particles.IsAlive(i)) continue;
		const PARTICLE& particle = m_Particles.Get(i);
		const float size = ParticleCalcSize(particle);
		const PARTICLE_COLOR c = ParticleCalcColor(particle);
		if (size <= 0.0f || c.A <= 0.0f) continue;
		const XMFLOAT3 center{ particle.Position.X, particle.Position.Y, particle.Position.Z };
		const XMFLOAT4 color{ c.R, c.G, c.B, c.A };
		if (particle.Shape == PARTICLE_SHAPE_BOX) ShapeDrawBox(center, { size, size, size }, color);
		else ShapeDrawSphere(center, size, color);
	}
}
