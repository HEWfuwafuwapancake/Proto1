// input/actioncalc.h（キーから移動の向きを作る・複数の機器の入力をまとめる）の単体テスト
#include <math.h>
#include "CppUnitTest.h"
#include "input/actioncalc.h"

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
		// 何も押していなければ 0
		TEST_METHOD(MoveFromKeys_NoKeysIsZero)
		{
			ACTION_AXIS axis = ActionCalcMoveFromKeys(false, false, false, false);
			Assert::AreEqual(0.0f, axis.X, ACTION_EPSILON);
			Assert::AreEqual(0.0f, axis.Y, ACTION_EPSILON);
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

		// 長さ 1 より短ければ、そのまま（スティックを少しだけ倒したとき）
		TEST_METHOD(ClampLength_KeepsShortAxis)
		{
			ACTION_AXIS axis = ActionCalcClampLength({ 0.3f, -0.4f });
			Assert::AreEqual(0.3f, axis.X, ACTION_EPSILON);
			Assert::AreEqual(-0.4f, axis.Y, ACTION_EPSILON);
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

		// 片方が入力なし（0）なら、もう片方をそのまま使う
		TEST_METHOD(CombineAxis_ZeroUsesOther)
		{
			ACTION_AXIS pad = { -0.2f, 0.1f };
			ACTION_AXIS result = ActionCalcCombineAxis({ 0.0f, 0.0f }, pad);
			Assert::AreEqual(-0.2f, result.X, ACTION_EPSILON);
			Assert::AreEqual(0.1f, result.Y, ACTION_EPSILON);
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

		// モードキーと数字キーを同じフレームで押しても、その手を選ぶ
		TEST_METHOD(EquipKey_ModeKeyAndNumberSameFrame)
		{
			int mode = -1;
			const ACTION_EQUIP_KEY key = ActionCalcEquipKey(mode, true, false, 3, true);
			Assert::AreEqual(0, key.Hand);
			Assert::AreEqual(3, key.Number);
			Assert::AreEqual(-1, mode);
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
