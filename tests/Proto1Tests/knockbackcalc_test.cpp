// physics/knockbackcalc.h（ダメージ%と吹っ飛び）の単体テスト
#include <math.h>
#include "CppUnitTest.h"
#include "physics/knockbackcalc.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

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
		PLAYER_BODY body{};
		body.OnGround = true;
		const VEC3 v = KnockbackCalcVelocity({ 1.0f, 0.0f, 0.0f }, KnockbackCalcSpeed(percent, 1.0f, params), params);
		body.VelocityX = v.X;
		body.VelocityY = v.Y;
		body.VelocityZ = v.Z;
		body.OnGround = false;
		for (int i = 0; i < 600; i++) {
			if (!KnockbackCalcStep(body, KB_DT, params, stage)) {
				return body.X;
			}
			if (StageCalcIsOut(stage, body.Y)) {
				return -1.0f;
			}
		}
		return -2.0f;	// 10 秒たっても終わらない（おかしい）
	}

	TEST_CLASS(KnockbackCalcTest)
	{
	public:
		// ダメージ%は足していく。負のダメージは 0
		TEST_METHOD(AddDamage_Accumulates)
		{
			float percent = 0.0f;
			percent = KnockbackCalcAddDamage(percent, 12.0f);
			percent = KnockbackCalcAddDamage(percent, 8.5f);
			Assert::AreEqual(20.5f, percent, KB_EPSILON);
			Assert::AreEqual(20.5f, KnockbackCalcAddDamage(percent, -5.0f), KB_EPSILON);
		}

		// 吹っ飛ぶ速さ：% が高いほど速く、攻撃の強さを掛ける
		TEST_METHOD(Speed_GrowsWithPercentAndPower)
		{
			const KNOCKBACK_PARAMS params = MakeKnockbackParams();
			Assert::AreEqual(4.0f, KnockbackCalcSpeed(0.0f, 1.0f, params), KB_EPSILON);
			Assert::AreEqual(16.0f, KnockbackCalcSpeed(100.0f, 1.0f, params), KB_EPSILON);
			Assert::AreEqual(32.0f, KnockbackCalcSpeed(100.0f, 2.0f, params), KB_EPSILON);
			Assert::IsTrue(KnockbackCalcSpeed(51.0f, 1.0f, params) > KnockbackCalcSpeed(50.0f, 1.0f, params));
		}

		// 速度：攻撃の向き（横）に 30° 上向き。長さは速さと同じ。攻撃の向きの上下と長さは使わない
		TEST_METHOD(Velocity_AngleUpFromAttackDirection)
		{
			const KNOCKBACK_PARAMS params = MakeKnockbackParams();
			const VEC3 v = KnockbackCalcVelocity({ 0.0f, -3.0f, 5.0f }, 10.0f, params);
			Assert::AreEqual(0.0f, v.X, KB_EPSILON);
			Assert::AreEqual(5.0f, v.Y, KB_EPSILON);				// 10 × sin30°
			Assert::AreEqual(10.0f * cosf(KB_PI / 6.0f), v.Z, KB_EPSILON);
			Assert::AreEqual(10.0f, Vec3Length(v), KB_EPSILON);
		}

		// 攻撃の向きが横に向いていない（真上・0）なら、真上に飛ぶ
		TEST_METHOD(Velocity_NoHorizontalGoesStraightUp)
		{
			const VEC3 v = KnockbackCalcVelocity({ 0.0f, 0.0f, 0.0f }, 7.0f, MakeKnockbackParams());
			Assert::AreEqual(0.0f, v.X, KB_EPSILON);
			Assert::AreEqual(7.0f, v.Y, KB_EPSILON);
			Assert::AreEqual(0.0f, v.Z, KB_EPSILON);
		}

		// 空中では、重力で下向きになり、空気の抵抗で横が遅くなる
		TEST_METHOD(Step_AirGravityAndDrag)
		{
			PLAYER_BODY body{};
			body.Y = 5.0f;
			body.VelocityX = 10.0f;
			Assert::IsTrue(KnockbackCalcStep(body, KB_DT, MakeKnockbackParams(), MakeKnockbackStage()));
			Assert::IsTrue(body.VelocityY < 0.0f);
			Assert::IsTrue(body.VelocityX < 10.0f && body.VelocityX > 9.9f);
		}

		// 浮島の上に落ちてきたら着地して、すべって止まる
		TEST_METHOD(Step_LandSlideAndStop)
		{
			PLAYER_BODY body{};
			body.Y = 1.0f;
			body.VelocityX = 6.0f;
			const KNOCKBACK_PARAMS params = MakeKnockbackParams();
			bool flying = true;
			bool landed = false;
			for (int i = 0; i < 300 && flying; i++) {
				flying = KnockbackCalcStep(body, KB_DT, params, MakeKnockbackStage());
				if (body.OnGround) landed = true;
			}
			Assert::IsTrue(landed);
			Assert::IsFalse(flying);
			Assert::AreEqual(0.0f, body.Y, KB_EPSILON);
			Assert::AreEqual(0.0f, body.VelocityX, KB_EPSILON);
			Assert::IsTrue(body.X > 0.0f && body.X < 15.0f);
		}

		// % が低いと浮島の上で止まり、高いと場外まで飛ぶ（真ん中から +X へ）
		TEST_METHOD(Launch_LowStaysHighFliesOut)
		{
			const float low = LaunchAndRun(0.0f);
			Assert::IsTrue(low > 0.0f && low < 15.0f);	// 浮島の上で止まる

			const float middle = LaunchAndRun(50.0f);
			Assert::IsTrue(middle > low);				// % が高いほど遠くで止まる

			Assert::AreEqual(-1.0f, LaunchAndRun(200.0f), KB_EPSILON);	// 場外まで飛ぶ
		}

		// すべっている途中で浮島の端から出たら、止まらずに落ちる
		TEST_METHOD(Step_SlideOffEdgeFalls)
		{
			PLAYER_BODY body{};
			body.X = 14.0f;
			body.OnGround = true;
			body.VelocityX = 10.0f;
			const KNOCKBACK_PARAMS params = MakeKnockbackParams();
			for (int i = 0; i < 30; i++) {
				Assert::IsTrue(KnockbackCalcStep(body, KB_DT, params, MakeKnockbackStage()));
			}
			Assert::IsFalse(body.OnGround);
			Assert::IsTrue(body.Y < 0.0f);
		}
	};
}
