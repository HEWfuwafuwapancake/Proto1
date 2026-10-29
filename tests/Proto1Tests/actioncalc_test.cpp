// input/actioncalc.h（キーから移動の向きを作る・複数の機器の入力をまとめる）の単体テスト
#include <math.h>
#include "CppUnitTest.h"
#include "input/actioncalc.h"
#include "input/keyboardcalc.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace Proto1Tests
{
	static constexpr float ACTION_EPSILON = 1.0e-5f;	// float の比較で許す誤差

	static float Length(ACTION_AXIS axis)
	{
		return sqrtf(axis.X * axis.X + axis.Y * axis.Y);
	}

	TEST_CLASS(ActionCalcTest)
	{
	public:
		// ローカルでも通信でも同じ整数を使い、移動とカメラを同じ精度で復元する。
		TEST_METHOD(PlayerInput_QuantizesMoveAndLook)
		{
			const auto input = ActionCalcPlayerInput({ 0.6f, -0.8f }, { 0.025f, -0.05f }, 1u << ACTION_JUMP);
			Assert::AreEqual(19660, static_cast<int>(input.MoveX));
			Assert::AreEqual(-26214, static_cast<int>(input.MoveY));
			Assert::AreEqual(102, static_cast<int>(input.LookX));
			Assert::AreEqual(-205, static_cast<int>(input.LookY));
			const auto move = ActionCalcPlayerMove(input);
			const auto look = ActionCalcPlayerLook(input);
			Assert::AreEqual(0.6f, move.X, 0.00002f);
			Assert::AreEqual(-0.8f, move.Y, 0.00002f);
			Assert::AreEqual(102.0f / 4096.0f, look.X, ACTION_EPSILON);
			Assert::AreEqual(-205.0f / 4096.0f, look.Y, ACTION_EPSILON);
			Assert::AreEqual(-1, input.Equip.Hand);
		}

		// 押し始め・長押し・離す・押し直し。ターゲットのイベントは連続でも落とさない。
		TEST_METHOD(PlayerInput_TriggersFromPreviousFrame)
		{
			PLAYER_INPUT released;
			PLAYER_INPUT held;
			held.Buttons = (1u << ACTION_JUMP) | (1u << ACTION_DECIDE) | (1u << ACTION_TARGET_RIGHT);
			Assert::IsTrue(ActionCalcPlayerTrigger(held, released, ACTION_JUMP));
			Assert::IsTrue(ActionCalcPlayerTrigger(held, released, ACTION_DECIDE));
			Assert::IsFalse(ActionCalcPlayerTrigger(held, held, ACTION_JUMP));
			Assert::IsFalse(ActionCalcPlayerTrigger(released, held, ACTION_JUMP));
			Assert::IsTrue(ActionCalcPlayerTrigger(held, released, ACTION_JUMP));
			Assert::IsTrue(ActionCalcPlayerTrigger(held, held, ACTION_TARGET_RIGHT));
			Assert::IsFalse(ActionCalcPlayerTrigger(released, held, ACTION_TARGET_RIGHT));
		}

		// Q と 5 の短い押下を記憶し、両手の盾を選べる。Shift は押下時の値を使う。
		TEST_METHOD(EquipKey_ShortPressIsRememberedUntilUpdate)
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
				Assert::AreEqual(hand, key.Hand);
				Assert::AreEqual(4, key.Number);
				numberKey.Update();
				Assert::IsFalse(numberKey.IsTriggered());
			}
			KeyboardTriggerCalc numberKey;
			numberKey.Press(false, true);
			numberKey.Update();
			int mode = -1;
			const auto key = ActionCalcEquipKey(mode, false, false, 4, numberKey.HadShift());
			Assert::AreEqual(1, key.Hand);
		}

		// 長押しでモードを連続反転させず、非選択中の押下も復帰時に持ち越さない。
		TEST_METHOD(EquipKey_RepeatAndFocusLossDoNotLeaveTriggers)
		{
			KeyboardTriggerCalc key;
			key.Press(false, false);
			key.Update();
			Assert::IsTrue(key.IsTriggered());
			key.Press(true, false);
			key.Update();
			Assert::IsFalse(key.IsTriggered());
			key.Press(false, true);
			key.Reset();
			key.Update();
			Assert::IsFalse(key.IsTriggered());
			Assert::IsFalse(key.HadShift());
		}

		// 前・後ろ・左・右は、それぞれの向きに長さ 1
		TEST_METHOD(MoveFromKeys_SingleKeys)
		{
			ACTION_AXIS forward = ActionCalcMoveFromKeys(true, false, false, false);
			Assert::AreEqual(0.0f, forward.X, ACTION_EPSILON);
			Assert::AreEqual(1.0f, forward.Y, ACTION_EPSILON);

			ACTION_AXIS back = ActionCalcMoveFromKeys(false, true, false, false);
			Assert::AreEqual(-1.0f, back.Y, ACTION_EPSILON);

			ACTION_AXIS left = ActionCalcMoveFromKeys(false, false, true, false);
			Assert::AreEqual(-1.0f, left.X, ACTION_EPSILON);
			Assert::AreEqual(0.0f, left.Y, ACTION_EPSILON);

			ACTION_AXIS right = ActionCalcMoveFromKeys(false, false, false, true);
			Assert::AreEqual(1.0f, right.X, ACTION_EPSILON);
		}

		// 斜め（前＋右）は長さ 1 で、45 度の向き
		TEST_METHOD(MoveFromKeys_DiagonalHasLengthOne)
		{
			ACTION_AXIS axis = ActionCalcMoveFromKeys(true, false, false, true);
			Assert::AreEqual(1.0f, Length(axis), ACTION_EPSILON);
			Assert::AreEqual(axis.X, axis.Y, ACTION_EPSILON);
			Assert::IsTrue(axis.X > 0.0f);
		}

		// 反対のキーを同時に押すと、その向きは 0
		TEST_METHOD(MoveFromKeys_OppositeKeysCancel)
		{
			ACTION_AXIS axis = ActionCalcMoveFromKeys(true, true, true, false);
			Assert::AreEqual(-1.0f, axis.X, ACTION_EPSILON);
			Assert::AreEqual(0.0f, axis.Y, ACTION_EPSILON);

			ACTION_AXIS all = ActionCalcMoveFromKeys(true, true, true, true);
			Assert::AreEqual(0.0f, Length(all), ACTION_EPSILON);
		}

		// 長さ 1 より長ければ、向きを変えずに長さ 1 にする
		TEST_METHOD(ClampLength_ShortensLongAxis)
		{
			ACTION_AXIS axis = ActionCalcClampLength({ 3.0f, 4.0f });
			Assert::AreEqual(0.6f, axis.X, ACTION_EPSILON);
			Assert::AreEqual(0.8f, axis.Y, ACTION_EPSILON);
		}

		// 2つの機器は、長いほう（強く倒しているほう）が勝つ
		TEST_METHOD(CombineAxis_LongerWins)
		{
			ACTION_AXIS keyboard = { 0.0f, 1.0f };
			ACTION_AXIS pad = { 0.5f, 0.0f };
			ACTION_AXIS result = ActionCalcCombineAxis(keyboard, pad);
			Assert::AreEqual(0.0f, result.X, ACTION_EPSILON);
			Assert::AreEqual(1.0f, result.Y, ACTION_EPSILON);

			result = ActionCalcCombineAxis(pad, keyboard);
			Assert::AreEqual(0.0f, result.X, ACTION_EPSILON);
			Assert::AreEqual(1.0f, result.Y, ACTION_EPSILON);
		}

		// 切り替えモードのキーだけでは選ばず、モードに入る
		TEST_METHOD(EquipKey_ModeKeyEntersModeWithoutSelecting)
		{
			int mode = -1;
			ACTION_EQUIP_KEY key = ActionCalcEquipKey(mode, false, true, -1, false);
			Assert::AreEqual(1, mode);
			Assert::AreEqual(-1, key.Hand);
			Assert::AreEqual(-1, key.Number);
			mode = -1;
			ActionCalcEquipKey(mode, true, false, -1, false);
			Assert::AreEqual(0, mode);
		}

		// モード中の数字キーはその手を選び、モードを抜ける（Shift は見ない）
		TEST_METHOD(EquipKey_NumberInModeSelectsThatHandAndLeavesMode)
		{
			int mode = 1;
			ACTION_EQUIP_KEY key = ActionCalcEquipKey(mode, false, false, 2, false);
			Assert::AreEqual(1, key.Hand);
			Assert::AreEqual(2, key.Number);
			Assert::AreEqual(-1, mode);
			mode = 0;
			key = ActionCalcEquipKey(mode, false, false, 1, true);
			Assert::AreEqual(0, key.Hand);
			Assert::AreEqual(1, key.Number);
			Assert::AreEqual(-1, mode);
		}

		// 同じモードキーをもう一度押すと抜け、別のキーなら乗りかえる。両方押したら右手
		TEST_METHOD(EquipKey_ModeKeysToggleAndSwitch)
		{
			int mode = 0;
			ActionCalcEquipKey(mode, true, false, -1, false);
			Assert::AreEqual(-1, mode);
			mode = 0;
			ActionCalcEquipKey(mode, false, true, -1, false);
			Assert::AreEqual(1, mode);
			const int starts[] = { -1, 0, 1 };
			for (int start : starts) {
				mode = start;
				ActionCalcEquipKey(mode, true, true, -1, false);
				Assert::AreEqual(1, mode);
			}
		}

		// モードでないときは今までどおり（Shift なしで左手・ありで右手）
		TEST_METHOD(EquipKey_NoModeUsesShift)
		{
			int mode = -1;
			ACTION_EQUIP_KEY key = ActionCalcEquipKey(mode, false, false, 0, false);
			Assert::AreEqual(0, key.Hand);
			Assert::AreEqual(0, key.Number);
			key = ActionCalcEquipKey(mode, false, false, 5, true);
			Assert::AreEqual(1, key.Hand);
			Assert::AreEqual(5, key.Number);
			Assert::AreEqual(-1, mode);
			key = ActionCalcEquipKey(mode, false, false, -1, true);
			Assert::AreEqual(-1, key.Hand);
			Assert::AreEqual(-1, key.Number);
		}
	};
}
