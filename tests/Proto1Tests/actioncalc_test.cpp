// input/actioncalc.h（キーから移動の向きを作る・複数の機器の入力をまとめる）の単体テスト
#include <math.h>
#include <gtest/gtest.h>
#include "input/actioncalc.h"
#include "input/keyboardcalc.h"

namespace Proto1Tests
{
	static constexpr float ACTION_EPSILON = 1.0e-5f;	// float の比較で許す誤差

	static float Length(ACTION_AXIS axis)
	{
		return sqrtf(axis.X * axis.X + axis.Y * axis.Y);
	}

	// ローカルでも通信でも同じ整数を使い、移動とカメラを同じ精度で復元する。
	TEST(ActionCalcTest, PlayerInput_QuantizesMoveAndLook)
	{
		const auto input = ActionCalcPlayerInput({ 0.6f, -0.8f }, { 0.025f, -0.05f }, 1u << ACTION_JUMP);
		EXPECT_EQ(19660, static_cast<int>(input.MoveX));
		EXPECT_EQ(-26214, static_cast<int>(input.MoveY));
		EXPECT_EQ(102, static_cast<int>(input.LookX));
		EXPECT_EQ(-205, static_cast<int>(input.LookY));
		const auto move = ActionCalcPlayerMove(input);
		const auto look = ActionCalcPlayerLook(input);
		EXPECT_NEAR(0.6f, move.X, 0.00002f);
		EXPECT_NEAR(-0.8f, move.Y, 0.00002f);
		EXPECT_NEAR(102.0f / 4096.0f, look.X, ACTION_EPSILON);
		EXPECT_NEAR(-205.0f / 4096.0f, look.Y, ACTION_EPSILON);
		EXPECT_EQ(-1, input.Equip.Hand);
	}

	// 腕の入力に使う押下状態は、前フレームの状態とは関係なく読める。
	TEST(ActionCalcTest, PlayerInput_ReadsHeldArmButton)
	{
		PLAYER_INPUT input;
		input.Buttons = 1u << ACTION_ARM_RIGHT;
		EXPECT_TRUE(ActionCalcPlayerPress(input, ACTION_ARM_RIGHT));
		EXPECT_FALSE(ActionCalcPlayerPress(input, ACTION_ARM_LEFT));
		EXPECT_FALSE(ActionCalcPlayerTrigger(input, input, ACTION_ARM_RIGHT));
		EXPECT_FALSE(ActionCalcPlayerPress(PLAYER_INPUT{}, ACTION_ARM_RIGHT));
	}

	// 押し始め・長押し・離す・押し直し。ターゲットのイベントは連続でも落とさない。
	TEST(ActionCalcTest, PlayerInput_TriggersFromPreviousFrame)
	{
		PLAYER_INPUT released;
		PLAYER_INPUT held;
		held.Buttons = (1u << ACTION_JUMP) | (1u << ACTION_DECIDE) | (1u << ACTION_TARGET_RIGHT);
		EXPECT_TRUE(ActionCalcPlayerTrigger(held, released, ACTION_JUMP));
		EXPECT_TRUE(ActionCalcPlayerTrigger(held, released, ACTION_DECIDE));
		EXPECT_FALSE(ActionCalcPlayerTrigger(held, held, ACTION_JUMP));
		EXPECT_FALSE(ActionCalcPlayerTrigger(released, held, ACTION_JUMP));
		EXPECT_TRUE(ActionCalcPlayerTrigger(held, released, ACTION_JUMP));
		EXPECT_TRUE(ActionCalcPlayerTrigger(held, held, ACTION_TARGET_RIGHT));
		EXPECT_FALSE(ActionCalcPlayerTrigger(released, held, ACTION_TARGET_RIGHT));
	}

	// Q と 5 の短い押下を記憶し、両手の盾を選べる。Shift は押下時の値を使う。
	TEST(ActionCalcTest, EquipKey_ShortPressIsRememberedUntilUpdate)
	{
		for (int hand = 0; hand < 2; ++hand) {
			KeyboardTriggerCalc modeKey, numberKey;
			int mode = -1;
			modeKey.Press(false, false);
			modeKey.Update();
			ActionCalcEquipKey(mode, hand == 0 && modeKey.IsTriggered(),
				hand == 1 && modeKey.IsTriggered(), -1, false);
			numberKey.Press(false, false);
			numberKey.Update(); // 更新前に離していても押下が残る
			const auto key = ActionCalcEquipKey(mode, false, false,
				numberKey.IsTriggered() ? 4 : -1, numberKey.HadShift());
			EXPECT_EQ(hand, key.Hand);
			EXPECT_EQ(4, key.Number);
			numberKey.Update();
			EXPECT_FALSE(numberKey.IsTriggered());
		}
		KeyboardTriggerCalc numberKey;
		numberKey.Press(false, true);
		numberKey.Update();
		int mode = -1;
		const auto key = ActionCalcEquipKey(mode, false, false, 4, numberKey.HadShift());
		EXPECT_EQ(1, key.Hand);
	}

	// Q を2回押すと左手の2段目、3回目で3段目、4回目で抜ける。数字は「段×10＋番号」。
	TEST(ActionCalcTest, EquipKey_ModeKeyCyclesPages)
	{
		int mode = -1;
		ActionCalcEquipKey(mode, true, false, -1, false);
		ActionCalcEquipKey(mode, true, false, -1, false);
		const auto second = ActionCalcEquipKey(mode, false, false, 2, false);
		EXPECT_EQ(0, second.Hand);
		EXPECT_EQ(12, second.Number);
		for (int i = 0; i < 3; ++i) ActionCalcEquipKey(mode, false, i == 2, -1, false);
		EXPECT_EQ(1, ActionCalcEquipModeHand(mode));
		EXPECT_EQ(0, ActionCalcEquipModePage(mode));
		ActionCalcEquipKey(mode, false, true, -1, false);
		ActionCalcEquipKey(mode, false, true, -1, false);
		EXPECT_EQ(2, ActionCalcEquipModePage(mode));
		ActionCalcEquipKey(mode, false, true, -1, false);
		EXPECT_EQ(-1, mode);
	}

	// 長押しでモードを連続反転させず、非選択中の押下も復帰時に持ち越さない。
	TEST(ActionCalcTest, EquipKey_RepeatAndFocusLossDoNotLeaveTriggers)
	{
		KeyboardTriggerCalc key;
		key.Press(false, false);
		key.Update();
		EXPECT_TRUE(key.IsTriggered());
		key.Press(true, false);
		key.Update();
		EXPECT_FALSE(key.IsTriggered());
		key.Press(false, true);
		key.Reset();
		key.Update();
		EXPECT_FALSE(key.IsTriggered());
		EXPECT_FALSE(key.HadShift());
	}

	// 前・後ろ・左・右は、それぞれの向きに長さ 1
	TEST(ActionCalcTest, MoveFromKeys_SingleKeys)
	{
		ACTION_AXIS forward = ActionCalcMoveFromKeys(true, false, false, false);
		EXPECT_NEAR(0.0f, forward.X, ACTION_EPSILON);
		EXPECT_NEAR(1.0f, forward.Y, ACTION_EPSILON);

		ACTION_AXIS back = ActionCalcMoveFromKeys(false, true, false, false);
		EXPECT_NEAR(-1.0f, back.Y, ACTION_EPSILON);

		ACTION_AXIS left = ActionCalcMoveFromKeys(false, false, true, false);
		EXPECT_NEAR(-1.0f, left.X, ACTION_EPSILON);
		EXPECT_NEAR(0.0f, left.Y, ACTION_EPSILON);

		ACTION_AXIS right = ActionCalcMoveFromKeys(false, false, false, true);
		EXPECT_NEAR(1.0f, right.X, ACTION_EPSILON);
	}

	// 斜め（前＋右）は長さ 1 で、45 度の向き
	TEST(ActionCalcTest, MoveFromKeys_DiagonalHasLengthOne)
	{
		ACTION_AXIS axis = ActionCalcMoveFromKeys(true, false, false, true);
		EXPECT_NEAR(1.0f, Length(axis), ACTION_EPSILON);
		EXPECT_NEAR(axis.X, axis.Y, ACTION_EPSILON);
		EXPECT_TRUE(axis.X > 0.0f);
	}

	// 反対のキーを同時に押すと、その向きは 0
	TEST(ActionCalcTest, MoveFromKeys_OppositeKeysCancel)
	{
		ACTION_AXIS axis = ActionCalcMoveFromKeys(true, true, true, false);
		EXPECT_NEAR(-1.0f, axis.X, ACTION_EPSILON);
		EXPECT_NEAR(0.0f, axis.Y, ACTION_EPSILON);

		ACTION_AXIS all = ActionCalcMoveFromKeys(true, true, true, true);
		EXPECT_NEAR(0.0f, Length(all), ACTION_EPSILON);
	}

	// 長さ 1 より長ければ、向きを変えずに長さ 1 にする
	TEST(ActionCalcTest, ClampLength_ShortensLongAxis)
	{
		ACTION_AXIS axis = ActionCalcClampLength({ 3.0f, 4.0f });
		EXPECT_NEAR(0.6f, axis.X, ACTION_EPSILON);
		EXPECT_NEAR(0.8f, axis.Y, ACTION_EPSILON);
	}

	// 2つの機器は、長いほう（強く倒しているほう）が勝つ
	TEST(ActionCalcTest, CombineAxis_LongerWins)
	{
		ACTION_AXIS keyboard = { 0.0f, 1.0f };
		ACTION_AXIS pad = { 0.5f, 0.0f };
		ACTION_AXIS result = ActionCalcCombineAxis(keyboard, pad);
		EXPECT_NEAR(0.0f, result.X, ACTION_EPSILON);
		EXPECT_NEAR(1.0f, result.Y, ACTION_EPSILON);

		result = ActionCalcCombineAxis(pad, keyboard);
		EXPECT_NEAR(0.0f, result.X, ACTION_EPSILON);
		EXPECT_NEAR(1.0f, result.Y, ACTION_EPSILON);
	}

	// 切り替えモードのキーだけでは選ばず、モードに入る
	TEST(ActionCalcTest, EquipKey_ModeKeyEntersModeWithoutSelecting)
	{
		int mode = -1;
		ACTION_EQUIP_KEY key = ActionCalcEquipKey(mode, false, true, -1, false);
		EXPECT_EQ(1, mode);
		EXPECT_EQ(-1, key.Hand);
		EXPECT_EQ(-1, key.Number);
		mode = -1;
		ActionCalcEquipKey(mode, true, false, -1, false);
		EXPECT_EQ(0, mode);
	}

	// モード中の数字キーはその手を選び、モードを抜ける（Shift は見ない）
	TEST(ActionCalcTest, EquipKey_NumberInModeSelectsThatHandAndLeavesMode)
	{
		int mode = 1;
		ACTION_EQUIP_KEY key = ActionCalcEquipKey(mode, false, false, 2, false);
		EXPECT_EQ(1, key.Hand);
		EXPECT_EQ(2, key.Number);
		EXPECT_EQ(-1, mode);
		mode = 0;
		key = ActionCalcEquipKey(mode, false, false, 1, true);
		EXPECT_EQ(0, key.Hand);
		EXPECT_EQ(1, key.Number);
		EXPECT_EQ(-1, mode);
	}

	// 同じモードキーは最後の段の次で抜け、別のキーなら乗りかえる。両方押したら右手
	TEST(ActionCalcTest, EquipKey_ModeKeysToggleAndSwitch)
	{
		int mode = (ACTION_EQUIP_PAGE_COUNT - 1) * 2;
		ActionCalcEquipKey(mode, true, false, -1, false);
		EXPECT_EQ(-1, mode);
		mode = 0;
		ActionCalcEquipKey(mode, false, true, -1, false);
		EXPECT_EQ(1, mode);
		const int starts[] = { -1, 0, 1 };
		for (int start : starts) {
			mode = start;
			ActionCalcEquipKey(mode, true, true, -1, false);
			EXPECT_EQ(1, mode);
		}
	}

	// モードでないときは今までどおり（Shift なしで左手・ありで右手）
	TEST(ActionCalcTest, EquipKey_NoModeUsesShift)
	{
		int mode = -1;
		ACTION_EQUIP_KEY key = ActionCalcEquipKey(mode, false, false, 0, false);
		EXPECT_EQ(0, key.Hand);
		EXPECT_EQ(0, key.Number);
		key = ActionCalcEquipKey(mode, false, false, 5, true);
		EXPECT_EQ(1, key.Hand);
		EXPECT_EQ(5, key.Number);
		EXPECT_EQ(-1, mode);
		key = ActionCalcEquipKey(mode, false, false, -1, true);
		EXPECT_EQ(-1, key.Hand);
		EXPECT_EQ(-1, key.Number);
	}
}
