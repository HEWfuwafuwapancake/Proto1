// physics/knockbackcalc.h（ダメージ%と吹っ飛び）の単体テスト
#include <math.h>
#include <gtest/gtest.h>
#include "physics/knockbackcalc.h"

namespace Proto1Tests
{
	static constexpr float KB_EPSILON = 1.0e-4f;	// float の比較で許す誤差
	static constexpr float KB_DT = 1.0f / 60.0f;	// 1フレームの秒数（ゲームと同じ）
	static constexpr float KB_PI = 3.14159265f;

	static KNOCKBACK_PARAMS MakeKnockbackParams(void)
	{
		KNOCKBACK_PARAMS params{};
		params.BaseSpeed = 4.0f;
		params.Growth = 0.12f;
		params.LaunchAngle = KB_PI / 6.0f;	// 30°
		params.AirDrag = 0.5f;
		params.GroundFriction = 20.0f;
		params.StopSpeed = 0.5f;
		params.Gravity = 25.0f;
		return params;
	}

	// ゲームと同じ形の浮島（30 × 30、上の面 y = 0、下の線 y = -15）
	static STAGE MakeKnockbackStage(void)
	{
		STAGE stage{};
		stage.HalfWidth = 15.0f;
		stage.HalfDepth = 15.0f;
		stage.TopY = 0.0f;
		stage.Thickness = 2.0f;
		stage.OutY = -15.0f;
		return stage;
	}

	// 浮島の真ん中に立っている体を、+X 向きに percent% で吹っ飛ばし、止まるか場外に落ちるまで動かす
	// 戻り値：止まったら止まった X、場外に落ちたら -1
	static float LaunchAndRun(float percent)
	{
		const KNOCKBACK_PARAMS params = MakeKnockbackParams();
		const STAGE stage = MakeKnockbackStage();
		PlayerBodyCalc body{};
		body.SetOnGround(true);
		const VEC3 v = KnockbackCalcVelocity({ 1.0f, 0.0f, 0.0f }, KnockbackCalcSpeed(percent, 1.0f, params), params);
		body.SetVelocity({ v.X, v.Y, v.Z });
		body.SetOnGround(false);
		for (int i = 0; i < 600; i++) {
			if (!KnockbackCalcStep(body, KB_DT, params, stage)) {
				return body.GetX();
			}
			if (StageCalcIsOut(stage, body.GetY())) {
				return -1.0f;
			}
		}
		return -2.0f;	// 10 秒たっても終わらない（おかしい）
	}

	// ダメージ%は足していく。負のダメージは 0
	// リペアで減らし、0 より下にはしない。
	TEST(KnockbackCalcTest, Heal_ReducesButNotBelowZero)
	{
		EXPECT_EQ(15.0f, KnockbackCalcHeal(40.0f, 25.0f));
		EXPECT_EQ(0.0f, KnockbackCalcHeal(10.0f, 25.0f));
	}

	TEST(KnockbackCalcTest, AddDamage_Accumulates)
	{
		float percent = 0.0f;
		percent = KnockbackCalcAddDamage(percent, 12.0f);
		percent = KnockbackCalcAddDamage(percent, 8.5f);
		EXPECT_NEAR(20.5f, percent, KB_EPSILON);
		EXPECT_NEAR(20.5f, KnockbackCalcAddDamage(percent, -5.0f), KB_EPSILON);
	}

	// 吹っ飛ぶ速さ：% が高いほど速く、攻撃の強さを掛ける
	TEST(KnockbackCalcTest, Speed_GrowsWithPercentAndPower)
	{
		const KNOCKBACK_PARAMS params = MakeKnockbackParams();
		EXPECT_NEAR(4.0f, KnockbackCalcSpeed(0.0f, 1.0f, params), KB_EPSILON);
		EXPECT_NEAR(16.0f, KnockbackCalcSpeed(100.0f, 1.0f, params), KB_EPSILON);
		EXPECT_NEAR(32.0f, KnockbackCalcSpeed(100.0f, 2.0f, params), KB_EPSILON);
		EXPECT_TRUE(KnockbackCalcSpeed(51.0f, 1.0f, params) > KnockbackCalcSpeed(50.0f, 1.0f, params));
	}

	// 速度：攻撃の向き（横）に 30° 上向き。長さは速さと同じ。攻撃の向きの上下と長さは使わない
	TEST(KnockbackCalcTest, Velocity_AngleUpFromAttackDirection)
	{
		const KNOCKBACK_PARAMS params = MakeKnockbackParams();
		const VEC3 v = KnockbackCalcVelocity({ 0.0f, -3.0f, 5.0f }, 10.0f, params);
		EXPECT_NEAR(0.0f, v.X, KB_EPSILON);
		EXPECT_NEAR(5.0f, v.Y, KB_EPSILON);				// 10 × sin30°
		EXPECT_NEAR(10.0f * cosf(KB_PI / 6.0f), v.Z, KB_EPSILON);
		EXPECT_NEAR(10.0f, Vec3Length(v), KB_EPSILON);
	}

	// 攻撃の向きが横に向いていない（真上・0）なら、真上に飛ぶ
	TEST(KnockbackCalcTest, Velocity_NoHorizontalGoesStraightUp)
	{
		const VEC3 v = KnockbackCalcVelocity({ 0.0f, 0.0f, 0.0f }, 7.0f, MakeKnockbackParams());
		EXPECT_NEAR(0.0f, v.X, KB_EPSILON);
		EXPECT_NEAR(7.0f, v.Y, KB_EPSILON);
		EXPECT_NEAR(0.0f, v.Z, KB_EPSILON);
	}

	// 空中では、重力で下向きになり、空気の抵抗で横が遅くなる
	TEST(KnockbackCalcTest, Step_AirGravityAndDrag)
	{
		PlayerBodyCalc body{};
		body.SetPosition({ body.GetX(), 5.0f, body.GetZ() });
		body.SetVelocity({ 10.0f, body.GetVelocityY(), body.GetVelocityZ() });
		EXPECT_TRUE(KnockbackCalcStep(body, KB_DT, MakeKnockbackParams(), MakeKnockbackStage()));
		EXPECT_TRUE(body.GetVelocityY() < 0.0f);
		EXPECT_TRUE(body.GetVelocityX() < 10.0f && body.GetVelocityX() > 9.9f);
	}

	// 浮島の上に落ちてきたら着地して、すべって止まる
	TEST(KnockbackCalcTest, Step_LandSlideAndStop)
	{
		PlayerBodyCalc body{};
		body.SetPosition({ body.GetX(), 1.0f, body.GetZ() });
		body.SetVelocity({ 6.0f, body.GetVelocityY(), body.GetVelocityZ() });
		const KNOCKBACK_PARAMS params = MakeKnockbackParams();
		bool flying = true;
		bool landed = false;
		for (int i = 0; i < 300 && flying; i++) {
			flying = KnockbackCalcStep(body, KB_DT, params, MakeKnockbackStage());
			if (body.GetOnGround()) landed = true;
		}
		EXPECT_TRUE(landed);
		EXPECT_FALSE(flying);
		EXPECT_NEAR(0.0f, body.GetY(), KB_EPSILON);
		EXPECT_NEAR(0.0f, body.GetVelocityX(), KB_EPSILON);
		EXPECT_TRUE(body.GetX() > 0.0f && body.GetX() < 15.0f);
	}

	// 吹っ飛び中も高台に着地し、勢いが残っていればふちから落ちる。
	TEST(KnockbackCalcTest, Step_LandsAndSlidesOffHighPlatform)
	{
		STAGE stage = MakeKnockbackStage();
		stage.Platforms[0] = { -8.0f, 8.0f, 3.0f, 3.0f, 4.0f, 0.6f };
		stage.PlatformCount = 1;
		const KNOCKBACK_PARAMS params = MakeKnockbackParams();
		PlayerBodyCalc body{};
		body.SetPosition({ -5.3f, 4.1f, 8.0f });
		body.SetVelocity({ 10.0f, -10.0f, 0.0f });

		EXPECT_TRUE(KnockbackCalcStep(body, KB_DT, params, stage));
		EXPECT_TRUE(body.GetOnGround());
		EXPECT_NEAR(stage.Platforms[0].TopY, body.GetY(), KB_EPSILON);
		EXPECT_GT(body.GetVelocityX(), 0.0f);

		for (int i = 0; i < 3; ++i) {
			EXPECT_TRUE(KnockbackCalcStep(body, KB_DT, params, stage));
		}
		EXPECT_FALSE(body.GetOnGround());
		EXPECT_LT(body.GetY(), stage.Platforms[0].TopY);
	}

	// % が低いと浮島の上で止まり、高いと場外まで飛ぶ（真ん中から +X へ）
	TEST(KnockbackCalcTest, Launch_LowStaysHighFliesOut)
	{
		const float low = LaunchAndRun(0.0f);
		EXPECT_TRUE(low > 0.0f && low < 15.0f);	// 浮島の上で止まる

		const float middle = LaunchAndRun(50.0f);
		EXPECT_TRUE(middle > low);				// % が高いほど遠くで止まる

		EXPECT_NEAR(-1.0f, LaunchAndRun(200.0f), KB_EPSILON);	// 場外まで飛ぶ
	}

	// すべっている途中で浮島の端から出たら、止まらずに落ちる
	TEST(KnockbackCalcTest, Step_SlideOffEdgeFalls)
	{
		PlayerBodyCalc body{};
		body.SetPosition({ 14.0f, body.GetY(), body.GetZ() });
		body.SetOnGround(true);
		body.SetVelocity({ 10.0f, body.GetVelocityY(), body.GetVelocityZ() });
		const KNOCKBACK_PARAMS params = MakeKnockbackParams();
		for (int i = 0; i < 30; i++) {
			EXPECT_TRUE(KnockbackCalcStep(body, KB_DT, params, MakeKnockbackStage()));
		}
		EXPECT_FALSE(body.GetOnGround());
		EXPECT_TRUE(body.GetY() < 0.0f);
	}
}
