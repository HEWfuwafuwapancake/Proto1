// physics/playercalc.h（移動・ジャンプ・重力・浮島への着地）の単体テスト
#include <math.h>
#include "CppUnitTest.h"
#include "physics/playercalc.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace Proto1Tests
{
	static constexpr float PLAYER_EPSILON = 1.0e-4f;	// float の比較で許す誤差
	static constexpr float PLAYER_DT = 1.0f / 60.0f;	// 1フレームの秒数（ゲームと同じ）

	// ゲームと同じ形の浮島（30 × 30、上の面 y = 0、下の線 y = -15）
	static STAGE MakePlayerStage(void)
	{
		STAGE stage{};
		stage.HalfWidth = 15.0f;
		stage.HalfDepth = 15.0f;
		stage.TopY = 0.0f;
		stage.Thickness = 2.0f;
		stage.OutY = -15.0f;
		return stage;
	}

	static PLAYER_PARAMS MakeParams(void)
	{
		PLAYER_PARAMS params{};
		params.MoveSpeed = 6.0f;
		params.JumpSpeed = 9.0f;
		params.Gravity = 25.0f;
		return params;
	}

	// 浮島の上の (x, z) に立っている体
	static PlayerBodyCalc MakeStandingBody(float x, float z)
	{
		PlayerBodyCalc body{};
		body.SetPosition({ x, body.GetY(), body.GetZ() });
		body.SetPosition({ body.GetX(), body.GetY(), z });
		body.SetOnGround(true);
		return body;
	}

	TEST_CLASS(PlayerCalcTest)
	{
	public:
		// ワールドの移動方向へ向く（右・左・前・斜め）。入力の長さにはよらない
		TEST_METHOD(Facing_TurnsToWorldMoveDirection)
		{
			const float pi = 3.14159265358979323846f;
			Assert::AreEqual(pi * 0.5f, PlayerCalcFacing(0, 1, 0, -1, false, 10, 1), PLAYER_EPSILON);
			Assert::AreEqual(-pi * 0.5f, PlayerCalcFacing(0, -1, 0, 1, false, 10, 1), PLAYER_EPSILON);
			Assert::AreEqual(0.0f, PlayerCalcFacing(1, 0, 1, 2, false, 10, 1), PLAYER_EPSILON);
			Assert::AreEqual(pi * 0.25f, PlayerCalcFacing(0, 0.2f, 0.2f, 0, false, 10, 1), PLAYER_EPSILON);
		}

		// 止まっている間は、カメラが回っても最後の向きを保つ
		TEST_METHOD(Facing_StandingStillKeepsLastYaw)
		{
			Assert::AreEqual(0.7f, PlayerCalcFacing(0.7f, 0, 0, -1, false, 10, 1), PLAYER_EPSILON);
		}

		// ボタンを押した瞬間と、伸長・保持・収縮中は、歩く方向より画面の正面を優先する
		TEST_METHOD(Facing_ArmImmediatelyFacesCameraAndFollowsIt)
		{
			float yaw = PlayerCalcFacing(-1, 1, 0, 0.6f, true, 0.1f, PLAYER_DT);
			Assert::AreEqual(0.6f, yaw, PLAYER_EPSILON);
			yaw = PlayerCalcFacing(yaw, -1, 0, -0.8f, true, 0.1f, PLAYER_DT);
			Assert::AreEqual(-0.8f, yaw, PLAYER_EPSILON);
			yaw = PlayerCalcFacing(yaw, 0, 0, 1.2f, true, 0.1f, PLAYER_DT);
			Assert::AreEqual(1.2f, yaw, PLAYER_EPSILON);
		}

		TEST_METHOD(Facing_LimitsTurnPerSecondAndStopsAtTarget)
		{
			Assert::AreEqual(0.1f, PlayerCalcFacing(0, 1, 0, 0, false, 0.2f, 0.5f), PLAYER_EPSILON);
			Assert::AreEqual(-0.1f, PlayerCalcFacing(0, -1, 0, 0, false, 0.2f, 0.5f), PLAYER_EPSILON);
			Assert::AreEqual(0.0f, PlayerCalcFacing(0.05f, 0, 1, 1, false, 0.2f, 0.5f), PLAYER_EPSILON);
		}

		TEST_METHOD(Facing_ReturnsToWalkingAfterArmEnds)
		{
			const float yaw = PlayerCalcFacing(0, 1, 0, 0.5f, true, 1, 0.1f);
			Assert::AreEqual(0.6f, PlayerCalcFacing(yaw, 1, 0, -1, false, 1, 0.1f), PLAYER_EPSILON);
		}

		// ターゲットは横歩き・後ろ歩き・停止中も優先し、解除後は歩く向きへ戻る。
		TEST_METHOD(Facing_TargetOverridesWalkingAndStanding)
		{
			Assert::AreEqual(0.8f, PlayerCalcFacing(0, -1, 0, -1, false, 10, 1, true, 0.8f), PLAYER_EPSILON);
			Assert::AreEqual(0.8f, PlayerCalcFacing(0, 0, -1, -1, false, 10, 1, true, 0.8f), PLAYER_EPSILON);
			Assert::AreEqual(0.8f, PlayerCalcFacing(0, 0, 0, -1, false, 10, 1, true, 0.8f), PLAYER_EPSILON);
			Assert::AreEqual(0.0f, PlayerCalcFacing(0.8f, 0, 1, -1, false, 10, 1, false, 0.8f), PLAYER_EPSILON);
		}

		TEST_METHOD(Facing_ArmOverridesTargetThenReturnsToTarget)
		{
			const float yaw = PlayerCalcFacing(0, 1, 0, -0.7f, true, 0.1f, PLAYER_DT, true, 1);
			Assert::AreEqual(-0.7f, yaw, PLAYER_EPSILON);
			Assert::AreEqual(1.0f, PlayerCalcFacing(yaw, 1, 0, -0.7f, false, 10, 1, true, 1), PLAYER_EPSILON);
		}

		// 1秒歩くと、歩く速さの分だけ進む
		TEST_METHOD(Step_WalksAtMoveSpeed)
		{
			PlayerBodyCalc body = MakeStandingBody(0.0f, 0.0f);
			for (int i = 0; i < 60; i++) {
				body.Step(0.0f, 1.0f, false, PLAYER_DT, MakeParams(), MakePlayerStage());
			}
			Assert::AreEqual(6.0f, body.GetZ(), 1.0e-3f);
			Assert::AreEqual(0.0f, body.GetX(), PLAYER_EPSILON);
			Assert::IsTrue(body.GetOnGround());
		}

		// 斜め（長さが 1 を超える向き）でも速くならない
		TEST_METHOD(Step_DiagonalIsNotFaster)
		{
			PlayerBodyCalc body = MakeStandingBody(0.0f, 0.0f);
			body.Step(1.0f, 1.0f, false, PLAYER_DT, MakeParams(), MakePlayerStage());
			const float speed = sqrtf(body.GetVelocityX() * body.GetVelocityX() + body.GetVelocityZ() * body.GetVelocityZ());
			Assert::AreEqual(6.0f, speed, PLAYER_EPSILON);
		}

		// 立っているときにジャンプを押すと、上に飛ぶ
		TEST_METHOD(Step_JumpWhenOnGround)
		{
			PlayerBodyCalc body = MakeStandingBody(0.0f, 0.0f);
			body.Step(0.0f, 0.0f, true, PLAYER_DT, MakeParams(), MakePlayerStage());
			Assert::IsFalse(body.GetOnGround());
			Assert::IsTrue(body.GetY() > 0.0f);
			Assert::IsTrue(body.GetVelocityY() > 0.0f);
		}

		// 空中ではジャンプできない（二段ジャンプしない）
		TEST_METHOD(Step_NoJumpInAir)
		{
			PlayerBodyCalc body{};
			body.SetPosition({ body.GetX(), 3.0f, body.GetZ() });
			body.SetVelocity({ body.GetVelocityX(), -1.0f, body.GetVelocityZ() });
			body.Step(0.0f, 0.0f, true, PLAYER_DT, MakeParams(), MakePlayerStage());
			Assert::IsTrue(body.GetVelocityY() < -1.0f);	// 重力でさらに下向きに
		}

		// ジャンプすると、そのうち浮島の上に着地して止まる
		TEST_METHOD(Step_JumpAndLand)
		{
			PlayerBodyCalc body = MakeStandingBody(0.0f, 0.0f);
			body.Step(0.0f, 0.0f, true, PLAYER_DT, MakeParams(), MakePlayerStage());
			float highest = body.GetY();
			int frames = 1;
			while (!body.GetOnGround() && frames < 600) {
				body.Step(0.0f, 0.0f, false, PLAYER_DT, MakeParams(), MakePlayerStage());
				if (body.GetY() > highest) highest = body.GetY();
				frames++;
			}
			Assert::IsTrue(body.GetOnGround());
			Assert::AreEqual(0.0f, body.GetY(), PLAYER_EPSILON);
			Assert::AreEqual(0.0f, body.GetVelocityY(), PLAYER_EPSILON);
			// 一番高いところは、速さ 9・重力 25 なら 9² / (2 × 25) = 1.62 くらい
			Assert::IsTrue(highest > 1.4f && highest < 1.8f);
		}

		// 浮島の端から歩き出ると、止まらずに落ちていく
		TEST_METHOD(Step_WalkOffEdgeAndFall)
		{
			PlayerBodyCalc body = MakeStandingBody(14.95f, 0.0f);
			for (int i = 0; i < 60; i++) {
				body.Step(1.0f, 0.0f, false, PLAYER_DT, MakeParams(), MakePlayerStage());
			}
			Assert::IsFalse(body.GetOnGround());
			Assert::IsTrue(body.GetY() < -1.0f);
			Assert::IsTrue(body.GetX() > 15.0f);
		}

		// 浮島の外の上から落ちてきても、着地しない（下の線まで落ちていく）
		TEST_METHOD(Step_FallOutsidePlatformDoesNotLand)
		{
			PlayerBodyCalc body{};
			body.SetPosition({ 20.0f, body.GetY(), body.GetZ() });
			body.SetPosition({ body.GetX(), 1.0f, body.GetZ() });
			for (int i = 0; i < 120; i++) {
				body.Step(0.0f, 0.0f, false, PLAYER_DT, MakeParams(), MakePlayerStage());
			}
			Assert::IsFalse(body.GetOnGround());
			Assert::IsTrue(StageCalcIsOut(MakePlayerStage(), body.GetY()));
		}

		// 浮島より下から上がってきても、上の面に引っかからない
		TEST_METHOD(Step_RisingFromBelowDoesNotLand)
		{
			PlayerBodyCalc body{};
			body.SetPosition({ body.GetX(), -0.5f, body.GetZ() });
			body.SetVelocity({ body.GetVelocityX(), 20.0f, body.GetVelocityZ() });
			body.Step(0.0f, 0.0f, false, PLAYER_DT, MakeParams(), MakePlayerStage());
			Assert::IsFalse(body.GetOnGround());
			Assert::IsTrue(body.GetY() > -0.5f);
		}
	};
}
