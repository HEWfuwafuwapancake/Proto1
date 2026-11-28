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
		EFFECT_SPREAD_COLUMN, ///< 向きに沿った柱の中に並べ、向きへ飛ばす（光の柱）
		EFFECT_SPREAD_INWARD, ///< まわりの球の上に置き、中心へ向けて飛ばす（溜め。球の半径は Jitter）
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
		float Height; ///< 柱の長さ（EFFECT_SPREAD_COLUMN だけ）
		float Jitter; ///< 柱の太さ（中心からずらす最大の距離。EFFECT_SPREAD_COLUMN だけ）
	};

	// 調整用の値（plan/11_エフェクトと手ごたえ の 2-1・2-5）
	constexpr float PI = 3.14159265f;
	constexpr PARTICLE_COLOR SPARK_COLOR{ 1.0f, 0.97f, 0.8f, 1.0f };
	constexpr PARTICLE_COLOR SPARK_END_COLOR{ 1.0f, 0.65f, 0.2f, 0.0f };
	constexpr PARTICLE_COLOR RING_COLOR{ 1.0f, 1.0f, 1.0f, 0.85f };
	constexpr PARTICLE_COLOR RING_END_COLOR{ 1.0f, 1.0f, 1.0f, 0.0f };
	constexpr PARTICLE_COLOR DUST_COLOR{ 0.72f, 0.66f, 0.56f, 0.55f }; // 土ぼこり
	constexpr PARTICLE_COLOR DUST_END_COLOR{ 0.72f, 0.66f, 0.56f, 0.0f };
	constexpr PARTICLE_COLOR DEBRIS_COLOR{ 0.62f, 0.64f, 0.7f, 1.0f };
	constexpr PARTICLE_COLOR PILLAR_COLOR{ 1.0f, 1.0f, 1.0f, 0.9f }; // 色は出すときにその人の色に変える
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
		// ここから No.227（plan/11 の 2-1b・2-2・2-3）。柱は最後の2つ（長さ・太さ）も使う
		{ 12, EFFECT_SPREAD_CONE, PI * 0.6f, 3.0f, 7.0f, 18.0f, 0.5f, 0.6f, 0.9f, 0.14f, 0.07f, DEBRIS_COLOR, { 0.45f, 0.45f, 0.5f, 0.0f }, PARTICLE_SHAPE_BOX }, // 腕の破片
		{ 1, EFFECT_SPREAD_STAY, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.22f, 0.22f, 0.2f, 0.75f, { 1.0f, 1.0f, 1.0f, 0.95f }, { 1.0f, 1.0f, 1.0f, 0.0f }, PARTICLE_SHAPE_SPHERE }, // 白い光
		{ 1, EFFECT_SPREAD_CONE, PI * 0.15f, 0.6f, 1.2f, -1.0f, 1.0f, 0.5f, 0.8f, 0.05f, 0.15f, { 0.55f, 0.55f, 0.55f, 0.5f }, { 0.45f, 0.45f, 0.45f, 0.0f }, PARTICLE_SHAPE_SPHERE }, // 腕の煙
		{ 2, EFFECT_SPREAD_CONE, PI * 0.35f, 2.0f, 4.0f, 10.0f, 2.0f, 0.1f, 0.2f, 0.035f, 0.0f, SPARK_COLOR, SPARK_END_COLOR, PARTICLE_SHAPE_SPHERE }, // 小さな火花
		{ 44, EFFECT_SPREAD_COLUMN, 0.0f, 4.0f, 9.0f, 0.0f, 1.0f, 0.5f, 0.9f, 0.22f, 0.05f, PILLAR_COLOR, { 1.0f, 1.0f, 1.0f, 0.0f }, PARTICLE_SHAPE_SPHERE, 7.0f, 0.6f }, // 壁の光の柱
		{ 32, EFFECT_SPREAD_COLUMN, 0.0f, 6.0f, 12.0f, 0.0f, 1.0f, 0.5f, 0.8f, 0.2f, 0.05f, PILLAR_COLOR, { 1.0f, 1.0f, 1.0f, 0.0f }, PARTICLE_SHAPE_SPHERE, 5.0f, 0.5f }, // 下の光の柱
		{ 28, EFFECT_SPREAD_COLUMN, 0.0f, 7.0f, 11.0f, 0.0f, 2.0f, 0.3f, 0.45f, 0.16f, 0.04f, PILLAR_COLOR, { 1.0f, 1.0f, 1.0f, 0.0f }, PARTICLE_SHAPE_SPHERE, 4.0f, 0.45f }, // 復活の光の柱
		{ 20, EFFECT_SPREAD_RING, 0.0f, 5.0f, 5.0f, 0.0f, 6.0f, 0.35f, 0.35f, 0.1f, 0.03f, RING_COLOR, RING_END_COLOR, PARTICLE_SHAPE_SPHERE }, // 足元の輪
		{ 10, EFFECT_SPREAD_CONE, PI, 1.5f, 3.0f, -1.0f, 1.5f, 0.3f, 0.5f, 0.06f, 0.0f, { 1.0f, 1.0f, 0.85f, 1.0f }, { 1.0f, 1.0f, 1.0f, 0.0f }, PARTICLE_SHAPE_SPHERE }, // 光の粒
		{ 1, EFFECT_SPREAD_CONE, PI * 0.2f, 0.8f, 1.5f, -1.5f, 1.0f, 0.6f, 1.0f, 0.12f, 0.3f, { 0.62f, 0.62f, 0.62f, 0.45f }, { 0.55f, 0.55f, 0.55f, 0.0f }, PARTICLE_SHAPE_SPHERE }, // 灰色の煙
		{ 1, EFFECT_SPREAD_CONE, PI * 0.2f, 0.9f, 1.7f, -1.5f, 1.0f, 0.7f, 1.1f, 0.15f, 0.4f, { 0.12f, 0.12f, 0.12f, 0.6f }, { 0.08f, 0.08f, 0.08f, 0.0f }, PARTICLE_SHAPE_SPHERE }, // 黒い煙
		// No.228（plan/11 の 2-5）
		{ 22, EFFECT_SPREAD_CONE, PI, 6.0f, 13.0f, 6.0f, 3.0f, 0.25f, 0.4f, 0.1f, 0.0f, { 1.0f, 0.95f, 0.35f, 1.0f }, { 1.0f, 0.75f, 0.1f, 0.0f }, PARTICLE_SHAPE_SPHERE }, // パリィの火花
		// No.229（plan/11 の 2-4）
		{ 12, EFFECT_SPREAD_RING, 0.0f, 3.0f, 3.0f, -0.5f, 3.0f, 0.35f, 0.35f, 0.12f, 0.24f, DUST_COLOR, DUST_END_COLOR, PARTICLE_SHAPE_SPHERE }, // 土ぼこりの輪
		{ 22, EFFECT_SPREAD_RING, 0.0f, 5.5f, 5.5f, -0.5f, 2.5f, 0.5f, 0.5f, 0.2f, 0.42f, DUST_COLOR, DUST_END_COLOR, PARTICLE_SHAPE_SPHERE }, // 大きな土ぼこりの輪
		{ 16, EFFECT_SPREAD_RING, 0.0f, 4.5f, 4.5f, 0.0f, 7.0f, 0.22f, 0.22f, 0.07f, 0.02f, RING_COLOR, RING_END_COLOR, PARTICLE_SHAPE_SPHERE }, // 空気の輪
		{ 7, EFFECT_SPREAD_CONE, PI * 0.25f, 2.0f, 4.0f, -0.5f, 3.0f, 0.3f, 0.45f, 0.12f, 0.26f, DUST_COLOR, DUST_END_COLOR, PARTICLE_SHAPE_SPHERE }, // 蹴った土ぼこり
		// No.230（plan/11 の 2-6 の ★★★）
		{ 12, EFFECT_SPREAD_CONE, PI, 3.0f, 6.0f, 2.0f, 3.0f, 0.25f, 0.35f, 0.08f, 0.0f, { 0.8f, 0.4f, 1.0f, 1.0f }, { 0.6f, 0.2f, 0.9f, 0.0f }, PARTICLE_SHAPE_SPHERE }, // つかんだ紫の粒
		{ 32, EFFECT_SPREAD_RING, 0.0f, 14.0f, 14.0f, 0.0f, 2.5f, 0.4f, 0.4f, 0.15f, 0.05f, { 1.0f, 0.95f, 0.75f, 0.8f }, { 1.0f, 0.9f, 0.6f, 0.0f }, PARTICLE_SHAPE_SPHERE }, // 衝撃波の輪
		{ 10, EFFECT_SPREAD_CONE, PI * 0.08f, 14.0f, 24.0f, 0.0f, 3.0f, 0.2f, 0.3f, 0.09f, 0.02f, SPARK_COLOR, { 1.0f, 1.0f, 1.0f, 0.0f }, PARTICLE_SHAPE_SPHERE }, // 突進の筋
		{ 10, EFFECT_SPREAD_CONE, PI * 0.5f, 5.0f, 10.0f, 4.0f, 3.0f, 0.15f, 0.3f, 0.08f, 0.0f, { 0.55f, 0.95f, 1.0f, 1.0f }, { 0.3f, 0.7f, 1.0f, 0.0f }, PARTICLE_SHAPE_SPHERE }, // レーザーの粒
		{ 1, EFFECT_SPREAD_STAY, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.2f, 0.2f, 0.2f, 1.0f, { 0.6f, 0.95f, 1.0f, 0.9f }, { 0.6f, 0.95f, 1.0f, 0.0f }, PARTICLE_SHAPE_SPHERE }, // 砲口の光
		{ 16, EFFECT_SPREAD_CONE, PI, 3.0f, 7.0f, 0.0f, 4.0f, 0.2f, 0.35f, 0.28f, 0.06f, { 1.0f, 0.7f, 0.2f, 1.0f }, { 1.0f, 0.25f, 0.05f, 0.0f }, PARTICLE_SHAPE_SPHERE }, // 爆発の火の玉
		{ 10, EFFECT_SPREAD_CONE, PI, 1.5f, 4.0f, -1.5f, 2.0f, 0.6f, 1.0f, 0.3f, 0.6f, { 0.2f, 0.2f, 0.2f, 0.6f }, { 0.15f, 0.15f, 0.15f, 0.0f }, PARTICLE_SHAPE_SPHERE }, // 爆発の煙
		// No.231（plan/11 の 2-1・2-6・2-7 の残り）
		{ 1, EFFECT_SPREAD_STAY, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.18f, 0.18f, 0.12f, 0.02f, { 1.0f, 1.0f, 1.0f, 0.55f }, { 1.0f, 1.0f, 1.0f, 0.0f }, PARTICLE_SHAPE_SPHERE }, // 軌跡
		{ 1, EFFECT_SPREAD_STAY, 0.0f, 0.0f, 0.0f, -0.6f, 0.0f, 0.4f, 0.4f, 0.12f, 0.3f, { 0.75f, 0.75f, 0.75f, 0.45f }, { 0.6f, 0.6f, 0.6f, 0.0f }, PARTICLE_SHAPE_SPHERE }, // 煙の尾
		{ 12, EFFECT_SPREAD_CONE, PI * 0.3f, 7.0f, 12.0f, 0.0f, 8.0f, 0.12f, 0.18f, 0.08f, 0.02f, { 1.0f, 1.0f, 1.0f, 0.9f }, { 0.8f, 0.9f, 1.0f, 0.0f }, PARTICLE_SHAPE_SPHERE }, // 刀の筋
		{ 22, EFFECT_SPREAD_CONE, PI * 0.4f, 9.0f, 16.0f, 0.0f, 7.0f, 0.15f, 0.22f, 0.11f, 0.02f, { 1.0f, 1.0f, 1.0f, 0.95f }, { 0.8f, 0.9f, 1.0f, 0.0f }, PARTICLE_SHAPE_SPHERE }, // 刀の大きな筋
		{ 3, EFFECT_SPREAD_INWARD, 0.0f, 2.5f, 3.5f, 0.0f, 0.0f, 0.2f, 0.25f, 0.03f, 0.07f, { 0.5f, 0.95f, 1.0f, 0.0f }, { 0.6f, 1.0f, 1.0f, 0.9f }, PARTICLE_SHAPE_SPHERE, 0.0f, 0.7f }, // 溜めの粒
		{ 18, EFFECT_SPREAD_COLUMN, 0.0f, 8.0f, 13.0f, 0.0f, 2.0f, 0.3f, 0.45f, 0.12f, 0.03f, { 0.6f, 1.0f, 0.9f, 0.9f }, { 0.6f, 1.0f, 0.9f, 0.0f }, PARTICLE_SHAPE_SPHERE, 0.6f, 0.45f }, // ジャンプ台の光
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

void EffectManager::Spawn(EFFECT_ID id, const VEC3& position, const VEC3& direction, const PARTICLE_COLOR* tint)
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
		else if (desc.Spread == EFFECT_SPREAD_COLUMN) {
			// 柱の中のどこか（向きに沿った長さと、まわりへのずれ）に置き、向きへ飛ばす。
			const float around = m_Random.NextFloat(0.0f, 2.0f * PI);
			const float offset = m_Random.NextFloat(0.0f, desc.Jitter);
			particle.Position = Vec3Add(Vec3Add(position, Vec3Scale(axis, m_Random.NextFloat(0.0f, desc.Height))),
				Vec3Add(Vec3Scale(side, cosf(around) * offset), Vec3Scale(up, sinf(around) * offset)));
			heading = axis;
		}
		else if (desc.Spread == EFFECT_SPREAD_INWARD) {
			// まわりの球の上から、中心へ向かう。
			const VEC3 outward = RandomInCone(axis, PI);
			particle.Position = Vec3Add(position, Vec3Scale(outward, desc.Jitter));
			heading = Vec3Scale(outward, -1.0f);
		}
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
		if (tint != nullptr) {
			particle.StartColor = { tint->R, tint->G, tint->B, desc.StartColor.A };
			particle.EndColor = { tint->R, tint->G, tint->B, desc.EndColor.A };
		}
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
