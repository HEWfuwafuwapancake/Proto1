#include <gtest/gtest.h>
#include "physics/particlecalc.h"

namespace
{
	PARTICLE MakeParticle(float life)
	{
		PARTICLE particle;
		particle.Life = life;
		particle.StartSize = 1.0f;
		particle.EndSize = 0.0f;
		return particle;
	}
}

// 寿命が来た粒は消え、進み具合に合わせて小さくなる。
TEST(ParticleCalcTest, ParticleFadesAndDisappearsAtLife)
{
	ParticleCalc calc;
	calc.Spawn(MakeParticle(0.5f));
	calc.Step(0.25f);
	EXPECT_EQ(1, calc.GetAliveCount());
	EXPECT_NEAR(0.5f, ParticleCalcSize(calc.Get(0)), 1.0e-4f);
	calc.Step(0.3f);
	EXPECT_EQ(0, calc.GetAliveCount());
}

// 重力で下へ速くなりながら落ちる。
TEST(ParticleCalcTest, GravityPullsParticleDown)
{
	ParticleCalc calc;
	PARTICLE particle = MakeParticle(5.0f);
	particle.Gravity = 10.0f;
	calc.Spawn(particle);
	calc.Step(0.5f);
	EXPECT_NEAR(-5.0f, calc.Get(0).Velocity.Y, 1.0e-4f);
	EXPECT_LT(calc.Get(0).Position.Y, 0.0f);
}

// 上限を超えて出すと、いちばん古く出した粒から入れ替わる。
TEST(ParticleCalcTest, OldestParticleIsReplacedWhenFull)
{
	ParticleCalc calc;
	for (int i = 0; i < ParticleCalc::MAX_PARTICLES; ++i) calc.Spawn(MakeParticle(1.0f));
	calc.Step(0.5f);
	calc.Spawn(MakeParticle(1.0f));
	EXPECT_EQ(ParticleCalc::MAX_PARTICLES, calc.GetAliveCount());
	EXPECT_NEAR(0.0f, calc.Get(0).Age, 1.0e-4f);
	EXPECT_NEAR(0.5f, calc.Get(1).Age, 1.0e-4f);
}
