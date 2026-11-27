#include "physics/particlecalc.h"

namespace
{
	float Progress(const PARTICLE& particle)
	{
		if (particle.Life <= 0.0f) return 1.0f;
		const float t = particle.Age / particle.Life;
		return t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t);
	}

	float Lerp(float a, float b, float t)
	{
		return a + (b - a) * t;
	}
}

void ParticleCalc::Reset()
{
	for (PARTICLE& particle : m_Particles) particle.Life = 0.0f;
	m_Next = 0;
}

void ParticleCalc::Spawn(const PARTICLE& particle)
{
	if (particle.Life <= 0.0f) return;
	// 空いている枠があっても、出した順に枠を回す（いっぱいのときに消えるのは、いちばん古く出した粒になる）。
	m_Particles[m_Next] = particle;
	m_Particles[m_Next].Age = 0.0f;
	m_Next = (m_Next + 1) % MAX_PARTICLES;
}

void ParticleCalc::Step(float dt)
{
	if (dt <= 0.0f) return;
	for (PARTICLE& particle : m_Particles) {
		if (particle.Life <= 0.0f) continue;
		particle.Age += dt;
		if (particle.Age >= particle.Life) {
			particle.Life = 0.0f;
			continue;
		}
		particle.Velocity.Y -= particle.Gravity * dt;
		const float keep = particle.Drag * dt < 1.0f ? 1.0f - particle.Drag * dt : 0.0f;
		particle.Velocity = Vec3Scale(particle.Velocity, keep);
		particle.Position = Vec3Add(particle.Position, Vec3Scale(particle.Velocity, dt));
	}
}

int ParticleCalc::GetAliveCount() const
{
	int count = 0;
	for (const PARTICLE& particle : m_Particles) {
		if (particle.Life > 0.0f) ++count;
	}
	return count;
}

float ParticleCalcSize(const PARTICLE& particle)
{
	return Lerp(particle.StartSize, particle.EndSize, Progress(particle));
}

PARTICLE_COLOR ParticleCalcColor(const PARTICLE& particle)
{
	const float t = Progress(particle);
	return { Lerp(particle.StartColor.R, particle.EndColor.R, t), Lerp(particle.StartColor.G, particle.EndColor.G, t),
		Lerp(particle.StartColor.B, particle.EndColor.B, t), Lerp(particle.StartColor.A, particle.EndColor.A, t) };
}
