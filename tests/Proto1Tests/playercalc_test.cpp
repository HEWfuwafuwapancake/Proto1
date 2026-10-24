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
	static PLAYER_BODY MakeStandingBody(float x, float z)
	{
		PLAYER_BODY body{};
		body.X = x;
		body.Z = z;
		body.OnGround = true;
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

		TEST_METHOD(Facing_CrossesPiByShortestPathInBothDirections)
		{
			const float pi = 3.14159265358979323846f;
			const float target = -pi + 0.2f;
			Assert::AreEqual(-pi + 0.05f,
				PlayerCalcFacing(pi - 0.05f, sinf(target), cosf(target), 0, false, 0.1f, 1), PLAYER_EPSILON);
			Assert::AreEqual(pi - 0.05f,
				PlayerCalcFacing(-pi + 0.05f, sinf(-target), cosf(-target), 0, false, 0.1f, 1), PLAYER_EPSILON);
		}

		// dt=0 では歩く方向へ回らない。腕の入力による即時の向きは反映する
		TEST_METHOD(Facing_ZeroDtKeepsWalkingYawButAppliesArmYaw)
		{
			Assert::AreEqual(0.3f, PlayerCalcFacing(0.3f, 1, 0, 1, false, 10, 0), PLAYER_EPSILON);
			Assert::AreEqual(1.0f, PlayerCalcFacing(0.3f, 1, 0, 1, true, 10, 0), PLAYER_EPSILON);
		}

		TEST_METHOD(Facing_ReturnsToWalkingAfterArmEnds)
		{
			const float yaw = PlayerCalcFacing(0, 1, 0, 0.5f, true, 1, 0.1f);
			Assert::AreEqual(0.6f, PlayerCalcFacing(yaw, 1, 0, -1, false, 1, 0.1f), PLAYER_EPSILON);
		}

		// 何もしなければ、立ったまま動かない
		TEST_METHOD(Step_StandingStill)
		{
			PLAYER_BODY body = MakeStandingBody(1.0f, 2.0f);
			PlayerCalcStep(body, 0.0f, 0.0f, false, PLAYER_DT, MakeParams(), MakePlayerStage());
			Assert::IsTrue(body.OnGround);
			Assert::AreEqual(1.0f, body.X, PLAYER_EPSILON);
			Assert::AreEqual(0.0f, body.Y, PLAYER_EPSILON);
			Assert::AreEqual(2.0f, body.Z, PLAYER_EPSILON);
		}

		// 1秒歩くと、歩く速さの分だけ進む
		TEST_METHOD(Step_WalksAtMoveSpeed)
		{
			PLAYER_BODY body = MakeStandingBody(0.0f, 0.0f);
			for (int i = 0; i < 60; i++) {
				PlayerCalcStep(body, 0.0f, 1.0f, false, PLAYER_DT, MakeParams(), MakePlayerStage());
			}
			Assert::AreEqual(6.0f, body.Z, 1.0e-3f);
			Assert::AreEqual(0.0f, body.X, PLAYER_EPSILON);
			Assert::IsTrue(body.OnGround);
		}

		// 斜め（長さが 1 を超える向き）でも速くならない
		TEST_METHOD(Step_DiagonalIsNotFaster)
		{
			PLAYER_BODY body = MakeStandingBody(0.0f, 0.0f);
			PlayerCalcStep(body, 1.0f, 1.0f, false, PLAYER_DT, MakeParams(), MakePlayerStage());
			const float speed = sqrtf(body.VelocityX * body.VelocityX + body.VelocityZ * body.VelocityZ);
			Assert::AreEqual(6.0f, speed, PLAYER_EPSILON);
		}

		// 立っているときにジャンプを押すと、上に飛ぶ
		TEST_METHOD(Step_JumpWhenOnGround)
		{
			PLAYER_BODY body = MakeStandingBody(0.0f, 0.0f);
			PlayerCalcStep(body, 0.0f, 0.0f, true, PLAYER_DT, MakeParams(), MakePlayerStage());
			Assert::IsFalse(body.OnGround);
			Assert::IsTrue(body.Y > 0.0f);
			Assert::IsTrue(body.VelocityY > 0.0f);
		}

		// 空中ではジャンプできない（二段ジャンプしない）
		TEST_METHOD(Step_NoJumpInAir)
		{
			PLAYER_BODY body{};
			body.Y = 3.0f;
			body.VelocityY = -1.0f;
			PlayerCalcStep(body, 0.0f, 0.0f, true, PLAYER_DT, MakeParams(), MakePlayerStage());
			Assert::IsTrue(body.VelocityY < -1.0f);	// 重力でさらに下向きに
		}

		// ジャンプすると、そのうち浮島の上に着地して止まる
		TEST_METHOD(Step_JumpAndLand)
		{
			PLAYER_BODY body = MakeStandingBody(0.0f, 0.0f);
			PlayerCalcStep(body, 0.0f, 0.0f, true, PLAYER_DT, MakeParams(), MakePlayerStage());
			float highest = body.Y;
			int frames = 1;
			while (!body.OnGround && frames < 600) {
				PlayerCalcStep(body, 0.0f, 0.0f, false, PLAYER_DT, MakeParams(), MakePlayerStage());
				if (body.Y > highest) highest = body.Y;
				frames++;
			}
			Assert::IsTrue(body.OnGround);
			Assert::AreEqual(0.0f, body.Y, PLAYER_EPSILON);
			Assert::AreEqual(0.0f, body.VelocityY, PLAYER_EPSILON);
			// 一番高いところは、速さ 9・重力 25 なら 9² / (2 × 25) = 1.62 くらい
			Assert::IsTrue(highest > 1.4f && highest < 1.8f);
		}

		// 浮島の端から歩き出ると、止まらずに落ちていく
		TEST_METHOD(Step_WalkOffEdgeAndFall)
		{
			PLAYER_BODY body = MakeStandingBody(14.95f, 0.0f);
			for (int i = 0; i < 60; i++) {
				PlayerCalcStep(body, 1.0f, 0.0f, false, PLAYER_DT, MakeParams(), MakePlayerStage());
			}
			Assert::IsFalse(body.OnGround);
			Assert::IsTrue(body.Y < -1.0f);
			Assert::IsTrue(body.X > 15.0f);
		}

		// 浮島の外の上から落ちてきても、着地しない（下の線まで落ちていく）
		TEST_METHOD(Step_FallOutsidePlatformDoesNotLand)
		{
			PLAYER_BODY body{};
			body.X = 20.0f;
			body.Y = 1.0f;
			for (int i = 0; i < 120; i++) {
				PlayerCalcStep(body, 0.0f, 0.0f, false, PLAYER_DT, MakeParams(), MakePlayerStage());
			}
			Assert::IsFalse(body.OnGround);
			Assert::IsTrue(StageCalcIsOut(MakePlayerStage(), body.Y));
		}

		// 浮島より下から上がってきても、上の面に引っかからない
		TEST_METHOD(Step_RisingFromBelowDoesNotLand)
		{
			PLAYER_BODY body{};
			body.Y = -0.5f;
			body.VelocityY = 20.0f;
			PlayerCalcStep(body, 0.0f, 0.0f, false, PLAYER_DT, MakeParams(), MakePlayerStage());
			Assert::IsFalse(body.OnGround);
			Assert::IsTrue(body.Y > -0.5f);
		}
	};
}
