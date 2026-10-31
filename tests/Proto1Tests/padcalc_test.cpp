// input/padcalc.h（DUALSHOCK 4 の USB のレポートの読み方・スティックの遊び）の単体テスト
#include <math.h>
#include "CppUnitTest.h"
#include "input/padcalc.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace Proto1Tests
{
	static constexpr float PAD_EPSILON = 1.0e-4f;	// float の比較で許す誤差

	// 何も触っていないときの USB のレポート（スティックは真ん中、方向キーは 8＝押していない）
	static void MakeNeutralReport(unsigned char (&report)[64])
	{
		for (int i = 0; i < 64; i++) {
			report[i] = 0;
		}
		report[0] = 0x01;
		report[1] = 128;
		report[2] = 128;
		report[3] = 128;
		report[4] = 128;
		report[5] = 0x08;
	}

	TEST_CLASS(PadCalcTest)
	{
	public:
		TEST_METHOD(XInput_PositionButtonsTriggerAndStick)
		{
			PAD_STATE state{};
			PadCalcParseXInput(0x1000, 255, 0, 0, 32767, 0, 0, state);
			Assert::AreEqual((unsigned int)(PAD_BUTTON_CROSS | PAD_BUTTON_L2), state.Buttons);
			Assert::AreEqual(1.0f, state.LeftStick.Y, PAD_EPSILON);
			PadCalcParseXInput(0x2000, 0, 0, 0, 0, 0, 0, state);
			Assert::AreEqual((unsigned int)PAD_BUTTON_CIRCLE, state.Buttons);
		}

		TEST_METHOD(SwitchPro_FullPositionButtonsAndStick)
		{
			Assert::AreEqual((int)PAD_MODEL_SWITCH_PRO, (int)PadCalcIdentifyModel(0x057E, 0x2009));
			unsigned char report[64] = { 0x30 };
			report[3] = 0x04; // 下の B
			report[5] = 0x80; // ZL
			report[6] = 0; report[7] = 0x08; report[8] = 0xE4; // X=2048、Y=3648
			report[9] = 0; report[10] = 0x08; report[11] = 0x80;
			PAD_STATE state{};
			Assert::IsTrue(PadCalcParseSwitchProReport(report, 64, state));
			Assert::AreEqual((unsigned int)(PAD_BUTTON_CROSS | PAD_BUTTON_L2), state.Buttons);
			Assert::AreEqual(0.0f, state.LeftStick.X, PAD_EPSILON);
			Assert::AreEqual(1.0f, state.LeftStick.Y, PAD_EPSILON);
			report[3] = 0x08; report[5] = 0; // 右の A
			Assert::IsTrue(PadCalcParseSwitchProReport(report, 64, state));
			Assert::AreEqual((unsigned int)PAD_BUTTON_CIRCLE, state.Buttons);
		}

		TEST_METHOD(SwitchPro_SimplePositionButtonsAndDpad)
		{
			unsigned char report[12] = { 0x3F, 0x01, 0, 1, 0, 0x80, 0, 0x80, 0, 0x80, 0, 0x80 };
			PAD_STATE state{};
			Assert::IsTrue(PadCalcParseSwitchProReport(report, 12, state));
			Assert::AreEqual((unsigned int)(PAD_BUTTON_CROSS | PAD_BUTTON_DPAD_UP | PAD_BUTTON_DPAD_RIGHT), state.Buttons);
			report[1] = 0x02; report[3] = 8;
			Assert::IsTrue(PadCalcParseSwitchProReport(report, 12, state));
			Assert::AreEqual((unsigned int)PAD_BUTTON_CIRCLE, state.Buttons);
		}

		TEST_METHOD(SwitchPro_InitializationBytes)
		{
			unsigned char report[64]{};
			Assert::IsTrue(PadCalcBuildSwitchUsbCommand(report, 64, 0x02));
			Assert::AreEqual(0x80, (int)report[0]);
			Assert::AreEqual(0x02, (int)report[1]);
			Assert::IsTrue(PadCalcBuildSwitchUsbCommand(report, 64, 0x04));
			Assert::AreEqual(0x04, (int)report[1]);
			Assert::IsTrue(PadCalcBuildSwitchInputMode(report, 64, 7));
			const unsigned char expected[] = { 0x01, 7, 0, 1, 0x40, 0x40, 0, 1, 0x40, 0x40, 3, 0x30 };
			for (int i = 0; i < 64; ++i) {
				Assert::AreEqual(i < 12 ? (int)expected[i] : 0, (int)report[i]);
			}
		}

		TEST_METHOD(Identify_SonyModels)
		{
			Assert::AreEqual((int)PAD_MODEL_DS4, (int)PadCalcIdentifyModel(0x054C, 0x05C4));
			Assert::AreEqual((int)PAD_MODEL_DS4, (int)PadCalcIdentifyModel(0x054C, 0x09CC));
			Assert::AreEqual((int)PAD_MODEL_DUALSENSE, (int)PadCalcIdentifyModel(0x054C, 0x0CE6));
			Assert::AreEqual((int)PAD_MODEL_NONE, (int)PadCalcIdentifyModel(0x1234, 0x0CE6));
			Assert::AreEqual((int)PAD_MODEL_NONE, (int)PadCalcIdentifyModel(0x054C, 0x1234));
		}

		// PS5 のトリガー・カウンターはボタン扱いにせず、両スティックの向きと中立を読む。
		TEST_METHOD(DualSense_SticksAndNeutral)
		{
			unsigned char report[64] = { 0x01, 128, 128, 128, 128, 255, 255, 255, 0x08 };
			PAD_STATE state{};
			Assert::IsTrue(PadCalcParseDualSenseUsbReport(report, 64, state));
			Assert::AreEqual(0u, state.Buttons);
			Assert::IsTrue(fabsf(state.LeftStick.X) < 0.01f && fabsf(state.LeftStick.Y) < 0.01f);
			Assert::IsTrue(fabsf(state.RightStick.X) < 0.01f && fabsf(state.RightStick.Y) < 0.01f);
			report[1] = 0; report[2] = 255; report[3] = 255; report[4] = 0;
			Assert::IsTrue(PadCalcParseDualSenseUsbReport(report, 64, state));
			Assert::AreEqual(-1.0f, state.LeftStick.X, PAD_EPSILON);
			Assert::AreEqual(-1.0f, state.LeftStick.Y, PAD_EPSILON);
			Assert::AreEqual(1.0f, state.RightStick.X, PAD_EPSILON);
			Assert::AreEqual(1.0f, state.RightStick.Y, PAD_EPSILON);
		}

		// ボタンを1個ずつ確かめる（複数ビットの取り違えを見逃さない）。
		TEST_METHOD(DualSense_EachButtonAndDpad)
		{
			unsigned char report[64] = { 0x01, 128, 128, 128, 128, 0, 0, 0, 0x08 };
			const unsigned int expected[] = {
				PAD_BUTTON_SQUARE, PAD_BUTTON_CROSS, PAD_BUTTON_CIRCLE, PAD_BUTTON_TRIANGLE,
				PAD_BUTTON_L1, PAD_BUTTON_R1, PAD_BUTTON_L2, PAD_BUTTON_R2,
				PAD_BUTTON_SHARE, PAD_BUTTON_OPTIONS, PAD_BUTTON_L3, PAD_BUTTON_R3, PAD_BUTTON_PS
			};
			PAD_STATE state{};
			for (int i = 0; i < 13; ++i) {
				report[8] = i < 4 ? (unsigned char)(0x08 | (0x10 << i)) : 0x08;
				report[9] = i >= 4 && i < 12 ? (unsigned char)(1 << (i - 4)) : 0;
				report[10] = i == 12 ? 1 : 0;
				Assert::IsTrue(PadCalcParseDualSenseUsbReport(report, 64, state));
				Assert::AreEqual(expected[i], state.Buttons);
			}
			const unsigned int directions[] = {
				PAD_BUTTON_DPAD_UP, PAD_BUTTON_DPAD_UP | PAD_BUTTON_DPAD_RIGHT,
				PAD_BUTTON_DPAD_RIGHT, PAD_BUTTON_DPAD_RIGHT | PAD_BUTTON_DPAD_DOWN,
				PAD_BUTTON_DPAD_DOWN, PAD_BUTTON_DPAD_DOWN | PAD_BUTTON_DPAD_LEFT,
				PAD_BUTTON_DPAD_LEFT, PAD_BUTTON_DPAD_LEFT | PAD_BUTTON_DPAD_UP
			};
			report[9] = 0; report[10] = 0;
			for (int i = 0; i < 8; ++i) {
				report[8] = (unsigned char)(0x40 | i); // ○と十字キーを同時に押せる
				Assert::IsTrue(PadCalcParseDualSenseUsbReport(report, 64, state));
				Assert::AreEqual(directions[i] | PAD_BUTTON_CIRCLE, state.Buttons);
			}
		}

		// Bluetooth の簡易0x01・通常0x31や、欠けたUSB入力を誤って読まない。
		TEST_METHOD(DualSense_RejectsNonUsbReport)
		{
			unsigned char report[78] = { 0x01 };
			PAD_STATE state{ { 0.25f, -0.5f }, { -0.75f, 1.0f }, PAD_BUTTON_CIRCLE };
			Assert::IsFalse(PadCalcParseDualSenseUsbReport(report, 10, state));
			Assert::IsFalse(PadCalcParseDualSenseUsbReport(report, 63, state));
			report[0] = 0x31;
			Assert::IsFalse(PadCalcParseDualSenseUsbReport(report, 78, state));
			Assert::IsFalse(PadCalcParseDualSenseUsbReport(nullptr, 64, state));
			Assert::AreEqual((unsigned int)PAD_BUTTON_CIRCLE, state.Buttons);
			Assert::AreEqual(0.25f, state.LeftStick.X, PAD_EPSILON);
			Assert::AreEqual(-0.5f, state.LeftStick.Y, PAD_EPSILON);
			Assert::AreEqual(-0.75f, state.RightStick.X, PAD_EPSILON);
			Assert::AreEqual(1.0f, state.RightStick.Y, PAD_EPSILON);
		}

		// USB出力は機種固有の位置へ書き、停止時はモーターを0にして色を保つ。
		TEST_METHOD(DualSense_OutputAndStop)
		{
			unsigned char report[63];
			for (unsigned char& byte : report) byte = 0xAA;
			Assert::IsTrue(PadCalcBuildDualSenseUsbOutput(report, 63, 0.4f, 0.6f, 0.95f, 0.45f, 0.35f));
			unsigned char expected[63] = { 0x02, 0x03, 0x04, 102, 153 };
			expected[45] = 242; expected[46] = 115; expected[47] = 89;
			for (int i = 0; i < 63; ++i) Assert::AreEqual((int)expected[i], (int)report[i]);
			Assert::IsTrue(PadCalcBuildDualSenseUsbOutput(report, 63, 0, 0, 0.95f, 0.45f, 0.35f));
			expected[3] = 0; expected[4] = 0;
			for (int i = 0; i < 63; ++i) Assert::AreEqual((int)expected[i], (int)report[i]);
		}

		// 端まで倒せば長さ 1、遊びのすぐ外は 0 に近い（遊びの境目で急に飛ばない）
		TEST_METHOD(DeadZone_RescalesOutside)
		{
			ACTION_AXIS full = PadCalcApplyDeadZone({ 1.0f, 0.0f }, 0.1f);
			Assert::AreEqual(1.0f, full.X, PAD_EPSILON);
			Assert::AreEqual(0.0f, full.Y, PAD_EPSILON);

			ACTION_AXIS half = PadCalcApplyDeadZone({ 0.0f, -0.55f }, 0.1f);
			Assert::AreEqual(0.0f, half.X, PAD_EPSILON);
			Assert::AreEqual(-0.5f, half.Y, PAD_EPSILON);

			ACTION_AXIS edge = PadCalcApplyDeadZone({ 0.11f, 0.0f }, 0.1f);
			Assert::IsTrue(edge.X > 0.0f && edge.X < 0.02f);
		}

		// 斜めの端（長さが 1 を超える）でも、長さは 1 まで
		TEST_METHOD(DeadZone_DiagonalLengthIsAtMostOne)
		{
			ACTION_AXIS axis = PadCalcApplyDeadZone({ 1.0f, 1.0f }, 0.1f);
			Assert::AreEqual(1.0f, sqrtf(axis.X * axis.X + axis.Y * axis.Y), PAD_EPSILON);
			Assert::AreEqual(axis.X, axis.Y, PAD_EPSILON);
		}

		// 何も触っていないレポート：ボタンなし、スティックはほぼ 0
		TEST_METHOD(Parse_NeutralReport)
		{
			unsigned char report[64];
			MakeNeutralReport(report);
			PAD_STATE state{};
			Assert::IsTrue(PadCalcParseDs4UsbReport(report, 64, state));
			Assert::AreEqual(0u, state.Buttons);
			Assert::IsTrue(fabsf(state.LeftStick.X) < 0.01f && fabsf(state.LeftStick.Y) < 0.01f);
			Assert::IsTrue(fabsf(state.RightStick.X) < 0.01f && fabsf(state.RightStick.Y) < 0.01f);
		}

		// ゲームで使うボタン（×・□・L1・R1・OPTIONS）のビット
		TEST_METHOD(Parse_GameButtons)
		{
			unsigned char report[64];
			MakeNeutralReport(report);
			report[5] = 0x08 | 0x20 | 0x10;	// × と □（方向キーは押していない）
			report[6] = 0x01 | 0x02 | 0x20;	// L1・R1・OPTIONS
			PAD_STATE state{};
			Assert::IsTrue(PadCalcParseDs4UsbReport(report, 64, state));
			Assert::AreEqual((unsigned int)(PAD_BUTTON_CROSS | PAD_BUTTON_SQUARE | PAD_BUTTON_L1 | PAD_BUTTON_R1 | PAD_BUTTON_OPTIONS), state.Buttons);
		}

		// 十字キーの8方向。ななめは2つの方向の同時押しにする。
		TEST_METHOD(Parse_DpadEightDirections)
		{
			unsigned char report[64];
			MakeNeutralReport(report);
			const unsigned int expected[] = {
				PAD_BUTTON_DPAD_UP,
				PAD_BUTTON_DPAD_UP | PAD_BUTTON_DPAD_RIGHT,
				PAD_BUTTON_DPAD_RIGHT,
				PAD_BUTTON_DPAD_RIGHT | PAD_BUTTON_DPAD_DOWN,
				PAD_BUTTON_DPAD_DOWN,
				PAD_BUTTON_DPAD_DOWN | PAD_BUTTON_DPAD_LEFT,
				PAD_BUTTON_DPAD_LEFT,
				PAD_BUTTON_DPAD_LEFT | PAD_BUTTON_DPAD_UP,
			};
			PAD_STATE state{};
			for (int direction = 0; direction < 8; ++direction) {
				report[5] = (unsigned char)direction;
				Assert::IsTrue(PadCalcParseDs4UsbReport(report, 64, state));
				Assert::AreEqual(expected[direction], state.Buttons);
				// 下位4ビットだけを読む。□×○△ と同時でも方向が変わらない。
				report[5] |= 0xF0;
				Assert::IsTrue(PadCalcParseDs4UsbReport(report, 64, state));
				Assert::AreEqual(expected[direction] | PAD_BUTTON_SQUARE | PAD_BUTTON_CROSS
					| PAD_BUTTON_CIRCLE | PAD_BUTTON_TRIANGLE, state.Buttons);
			}
		}

		// 読めないレポート（短い・番号が違う・null）は false で、状態を書き換えない
		TEST_METHOD(Parse_RejectsBadReport)
		{
			unsigned char report[64];
			MakeNeutralReport(report);
			PAD_STATE state{};
			state.Buttons = 123u;

			Assert::IsFalse(PadCalcParseDs4UsbReport(report, 7, state));
			report[0] = 0x11;	// Bluetooth のときのレポート番号
			Assert::IsFalse(PadCalcParseDs4UsbReport(report, 64, state));
			Assert::IsFalse(PadCalcParseDs4UsbReport(nullptr, 64, state));
			Assert::AreEqual(123u, state.Buttons);
		}

		// USB 出力：モーターと色の位置、番号・変更の印、未使用バイトがすべて 0。
		TEST_METHOD(Output_BytePositionsAndUnusedBytes)
		{
			unsigned char report[32];
			for (unsigned char& byte : report) byte = 0xAA;
			Assert::IsTrue(PadCalcBuildDs4UsbOutput(report, 32, 0.4f, 0.6f, 0.95f, 0.45f, 0.35f));
			const unsigned char expected[32] = { 0x05, 0x07, 0, 0, 102, 153, 242, 115, 89 };
			for (int i = 0; i < 32; ++i) Assert::AreEqual((int)expected[i], (int)report[i]);
		}

		// 各モーター・RGB のすべてで範囲外を切る。
		TEST_METHOD(Output_ClampsEveryValue)
		{
			unsigned char report[32];
			Assert::IsTrue(PadCalcBuildDs4UsbOutput(report, 32, -1.0f, -2.0f, -0.1f, -10.0f, -0.5f));
			for (int i = 4; i <= 8; ++i) Assert::AreEqual(0, (int)report[i]);
			Assert::IsTrue(PadCalcBuildDs4UsbOutput(report, 32, 2.0f, 3.0f, 1.1f, 10.0f, 1.5f));
			for (int i = 4; i <= 8; ++i) Assert::AreEqual(255, (int)report[i]);
		}

		// 停止状態から開始し、両モーターの範囲外の強さを切る。
		TEST_METHOD(Rumble_StartAndClamp)
		{
			PAD_RUMBLE rumble{};
			PadCalcRumble(rumble, 0.4f, 0.6f, 0.15f);
			Assert::AreEqual(0.4f, rumble.SmallMotor, PAD_EPSILON);
			Assert::AreEqual(0.6f, rumble.LargeMotor, PAD_EPSILON);
			Assert::AreEqual(0.15f, rumble.RemainingSeconds, PAD_EPSILON);
			rumble = {};
			PadCalcRumble(rumble, -1.0f, 2.0f, 0.4f);
			Assert::AreEqual(0.0f, rumble.SmallMotor, PAD_EPSILON);
			Assert::AreEqual(1.0f, rumble.LargeMotor, PAD_EPSILON);
			rumble = {};
			PadCalcRumble(rumble, 2.0f, -1.0f, 0.4f);
			Assert::AreEqual(1.0f, rumble.SmallMotor, PAD_EPSILON);
			Assert::AreEqual(0.0f, rumble.LargeMotor, PAD_EPSILON);
		}

		// 各モーターの強さを別々に重ね、長い残り時間を保つ。
		TEST_METHOD(Rumble_MergesStrongestMotorsAndLongestTime)
		{
			PAD_RUMBLE rumble{};
			PadCalcRumble(rumble, 0.4f, 0.0f, 0.1f); // つかむ
			PadCalcRumble(rumble, 0.6f, 0.6f, 0.15f); // 投げる
			PadCalcRumble(rumble, 0.0f, 1.0f, 0.4f); // 得点
			Assert::AreEqual(0.6f, rumble.SmallMotor, PAD_EPSILON);
			Assert::AreEqual(1.0f, rumble.LargeMotor, PAD_EPSILON);
			Assert::AreEqual(0.4f, rumble.RemainingSeconds, PAD_EPSILON);
			PadCalcRumble(rumble, 0.4f, 0.0f, 0.1f); // 弱く短い振動で上書きしない
			Assert::AreEqual(0.6f, rumble.SmallMotor, PAD_EPSILON);
			Assert::AreEqual(1.0f, rumble.LargeMotor, PAD_EPSILON);
			Assert::AreEqual(0.4f, rumble.RemainingSeconds, PAD_EPSILON);
		}

		// 減算中は強さが変わらず、ちょうど時間切れ・時間超過で両方止まる。
		TEST_METHOD(Rumble_UpdateAndExpire)
		{
			PAD_RUMBLE rumble{ 0.4f, 0.8f, 0.5f };
			PadCalcUpdateRumble(rumble, 0.25f);
			Assert::AreEqual(0.4f, rumble.SmallMotor, PAD_EPSILON);
			Assert::AreEqual(0.8f, rumble.LargeMotor, PAD_EPSILON);
			Assert::AreEqual(0.25f, rumble.RemainingSeconds, PAD_EPSILON);
			PadCalcUpdateRumble(rumble, 0.25f);
			Assert::AreEqual(0.0f, rumble.SmallMotor, PAD_EPSILON);
			Assert::AreEqual(0.0f, rumble.LargeMotor, PAD_EPSILON);
			Assert::AreEqual(0.0f, rumble.RemainingSeconds, PAD_EPSILON);
			PadCalcRumble(rumble, 0.6f, 0.6f, 0.15f);
			PadCalcUpdateRumble(rumble, 1.0f);
			Assert::AreEqual(0.0f, rumble.SmallMotor, PAD_EPSILON);
			Assert::AreEqual(0.0f, rumble.LargeMotor, PAD_EPSILON);
			Assert::AreEqual(0.0f, rumble.RemainingSeconds, PAD_EPSILON);
		}

	};
}
