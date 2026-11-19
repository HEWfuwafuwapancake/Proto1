// physics/cpucalc.h（CPU が状況から入力を作る計算）の単体テスト
#include <gtest/gtest.h>
#include "physics/cpucalc.h"

namespace Proto1Tests
{
	static constexpr float CPU_DT = 0.05f;
	static constexpr int CPU_STEPS = 30; // 1.5 秒分（最初に考えるまでの待ちを含む）

	// 中心 (0, 0)、横と奥行きが ±20 m、上の面の高さ 0 の浮島。
	static CPU_VIEW MakeView()
	{
		CPU_VIEW view;
		view.Dt = CPU_DT;
		view.OnGround = true;
		view.CanAct = true;
		view.Stage.HalfWidth = 20.0f;
		view.Stage.HalfDepth = 20.0f;
		view.Stage.Thickness = 2.0f;
		view.Stage.OutY = -30.0f;
		view.Arms[0] = { true, 2.0f, 0.0f, 0.0f, false, CPU_RECOVERY_NONE };
		return view;
	}

	static bool Pressed(const PLAYER_INPUT& input, ACTION action)
	{
		return ActionCalcPlayerPress(input, action);
	}

	// 遠くの相手へ向かって歩く（カメラが +Z を向いていれば前へ倒す）。腕が届くまでは振らない。
	TEST(CpuCalcTest, Chase_MovesTowardFarOpponentWithoutAttacking)
	{
		CPU_VIEW view = MakeView();
		view.Opponents[1] = { true, { 0.0f, 0.0f, 10.0f }, 0.0f };
		CpuCalc cpu;
		cpu.Reset(1, 0, VERSUS_CPU_NORMAL);
		PLAYER_INPUT input{};
		bool attacked = false;
		for (int i = 0; i < CPU_STEPS; ++i) {
			input = cpu.Step(view);
			attacked = attacked || Pressed(input, ACTION_ARM_LEFT);
		}
		EXPECT_EQ(1, cpu.GetTarget());
		EXPECT_GT(input.MoveY, 30000);
		EXPECT_NEAR(0.0, input.MoveX, 100.0);
		EXPECT_FALSE(attacked);
	}

	// 腕の届く距離に相手がいれば、腕のボタンを押す。
	TEST(CpuCalcTest, Attack_PressesArmWhenOpponentInRange)
	{
		CPU_VIEW view = MakeView();
		view.Opponents[2] = { true, { 1.0f, 0.0f, 0.0f }, 0.0f };
		CpuCalc cpu;
		cpu.Reset(2, 0, VERSUS_CPU_HARD);
		bool attacked = false;
		for (int i = 0; i < CPU_STEPS; ++i) attacked = attacked || Pressed(cpu.Step(view), ACTION_ARM_LEFT);
		EXPECT_TRUE(attacked);
	}

	// 浮島の外で落ちていれば、浮島の中心へ向かいながら空中ジャンプする。
	TEST(CpuCalcTest, Recovery_JumpsTowardStageCenterWhenOffStage)
	{
		CPU_VIEW view = MakeView();
		view.Position = { 25.0f, -2.0f, 0.0f };
		view.OnGround = false;
		view.Opponents[1] = { true, { 0.0f, 0.0f, 0.0f }, 0.0f };
		CpuCalc cpu;
		cpu.Reset(3, 0, VERSUS_CPU_NORMAL);
		const PLAYER_INPUT input = cpu.Step(view);
		EXPECT_LT(input.MoveX, -30000); // カメラが +Z のとき、-X（中心）は左
		EXPECT_TRUE(Pressed(input, ACTION_JUMP));
		EXPECT_FALSE(Pressed(input, ACTION_ARM_LEFT));
	}

	// 相手が場外の向こうにいても、浮島のふちから自分で歩き出さない。
	TEST(CpuCalcTest, Edge_StopsInsteadOfWalkingOff)
	{
		CPU_VIEW view = MakeView();
		view.Position = { 19.5f, 0.0f, 0.0f };
		view.Opponents[1] = { true, { 30.0f, 0.0f, 0.0f }, 0.0f };
		CpuCalc cpu;
		cpu.Reset(4, 0, VERSUS_CPU_NORMAL);
		PLAYER_INPUT input{};
		for (int i = 0; i < CPU_STEPS; ++i) input = cpu.Step(view);
		EXPECT_EQ(1, cpu.GetTarget());
		EXPECT_EQ(0, input.MoveX);
		EXPECT_EQ(0, input.MoveY);
	}

	// 同じ種と同じ状況なら、毎フレーム同じ入力になる。
	TEST(CpuCalcTest, Determinism_SameSeedGivesSameInputs)
	{
		CPU_VIEW view = MakeView();
		view.Opponents[1] = { true, { 1.5f, 0.0f, 1.0f }, 40.0f };
		view.Opponents[3] = { true, { -6.0f, 0.0f, 2.0f }, 0.0f };
		view.Arms[1] = { true, 3.0f, 0.8f, 0.5f, true, CPU_RECOVERY_NONE };
		CpuCalc a, b;
		a.Reset(99, 0, VERSUS_CPU_HARD);
		b.Reset(99, 0, VERSUS_CPU_HARD);
		for (int i = 0; i < CPU_STEPS * 2; ++i) {
			const PLAYER_INPUT x = a.Step(view), y = b.Step(view);
			EXPECT_EQ(x.Buttons, y.Buttons);
			EXPECT_EQ(x.MoveX, y.MoveX);
			EXPECT_EQ(x.MoveY, y.MoveY);
		}
	}

	// 同じくらいの近さの相手が2人いても、考え直すたびに狙う相手を入れ替えない。
	TEST(CpuCalcTest, Target_KeepsCurrentWhenOpponentsAreSimilar)
	{
		CPU_VIEW view = MakeView();
		CpuCalc cpu;
		cpu.Reset(5, 0, VERSUS_CPU_HARD);
		int first = -1;
		bool switched = false;
		for (int i = 0; i < CPU_STEPS * 4; ++i) {
			// 2人の近さが 0.3 m ずつ交互に入れ替わる。
			const bool odd = (i / 4) % 2 == 1;
			view.Opponents[1] = { true, { 0.0f, 0.0f, odd ? 5.0f : 5.3f }, 0.0f };
			view.Opponents[2] = { true, { 0.0f, 0.0f, odd ? -5.3f : -5.0f }, 0.0f };
			cpu.Step(view);
			if (first < 0) first = cpu.GetTarget();
			else switched = switched || cpu.GetTarget() != first;
		}
		EXPECT_GE(first, 1);
		EXPECT_FALSE(switched);
	}

	// 止まる距離まで近づいて止まったら、相手が少し揺れても歩き出さない。
	TEST(CpuCalcTest, Chase_StaysStoppedWhenOpponentWobblesAtRange)
	{
		CPU_VIEW view = MakeView();
		view.Arms[0] = { false, 0.0f, 0.0f, 0.0f, false, CPU_RECOVERY_NONE }; // 腕なし：2 m で止まる
		view.Opponents[1] = { true, { 0.0f, 0.0f, 6.0f }, 0.0f };
		CpuCalc cpu;
		cpu.Reset(6, 0, VERSUS_CPU_NORMAL);
		for (int i = 0; i < CPU_STEPS; ++i) cpu.Step(view);
		bool moved = false;
		for (int i = 0; i < CPU_STEPS; ++i) {
			view.Opponents[1].Position.Z = i % 2 == 0 ? 1.9f : 2.3f;
			const PLAYER_INPUT input = cpu.Step(view);
			if (i > 0) moved = moved || input.MoveX != 0 || input.MoveY != 0;
		}
		EXPECT_FALSE(moved);
	}

	// ロックオンを合わせるときは、狙う相手のいる側へ切り替える。カメラがロックしていなければ押さない。
	TEST(CpuCalcTest, TargetSwitch_PressesTowardWantedSide)
	{
		CPU_VIEW view = MakeView();
		view.Opponents[1] = { true, { 0.0f, 0.0f, 3.0f }, 0.0f };
		view.Opponents[2] = { false, { 3.0f, 0.0f, 0.0f }, 0.0f }; // ロックしているが狙えない相手（右）
		CpuCalc cpu;
		cpu.Reset(7, 0, VERSUS_CPU_NORMAL);
		view.CameraTarget = -1;
		bool pressedWithoutLock = false;
		for (int i = 0; i < CPU_STEPS; ++i) {
			const PLAYER_INPUT input = cpu.Step(view);
			pressedWithoutLock = pressedWithoutLock || Pressed(input, ACTION_TARGET_LEFT) || Pressed(input, ACTION_TARGET_RIGHT);
		}
		EXPECT_EQ(1, cpu.GetTarget());
		EXPECT_FALSE(pressedWithoutLock);
		view.CameraTarget = 2;
		const PLAYER_INPUT input = cpu.Step(view);
		EXPECT_TRUE(Pressed(input, ACTION_TARGET_LEFT)); // 正面（+Z）の相手は、右（+X）の相手から見て左
		EXPECT_FALSE(Pressed(input, ACTION_TARGET_RIGHT));
	}
}
