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
		// スティックの値：0 が -1、255 が +1、真ん中（127・128）はほぼ 0
		TEST_METHOD(StickValue_EndsAndCenter)
		{
			Assert::AreEqual(-1.0f, PadCalcStickValue(0), PAD_EPSILON);
			Assert::AreEqual(1.0f, PadCalcStickValue(255), PAD_EPSILON);
			Assert::IsTrue(fabsf(PadCalcStickValue(127)) < 0.01f);
			Assert::IsTrue(fabsf(PadCalcStickValue(128)) < 0.01f);
		}

		// 遊びの中（手を離したときのずれ）は 0
		TEST_METHOD(DeadZone_InsideIsZero)
		{
			ACTION_AXIS axis = PadCalcApplyDeadZone({ 0.05f, -0.05f }, 0.1f);
			Assert::AreEqual(0.0f, axis.X, PAD_EPSILON);
			Assert::AreEqual(0.0f, axis.Y, PAD_EPSILON);
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

		// スティック：上に倒すと Y がプラス（レポートでは上が 0）、右に倒すと X がプラス
		TEST_METHOD(Parse_SticksUpIsPlus)
		{
			unsigned char report[64];
			MakeNeutralReport(report);
			report[2] = 0;		// 左スティックを上の端へ
			report[3] = 255;	// 右スティックを右の端へ
			report[4] = 255;	// 右スティックを下の端へ
			PAD_STATE state{};
			Assert::IsTrue(PadCalcParseDs4UsbReport(report, 64, state));
			Assert::AreEqual(1.0f, state.LeftStick.Y, PAD_EPSILON);
			Assert::AreEqual(1.0f, state.RightStick.X, PAD_EPSILON);
			Assert::AreEqual(-1.0f, state.RightStick.Y, PAD_EPSILON);
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

		// 残りのボタン（○・△・L2・R2・SHARE・L3・R3・PS）のビット
		TEST_METHOD(Parse_OtherButtons)
		{
			unsigned char report[64];
			MakeNeutralReport(report);
			report[5] = 0x08 | 0x40 | 0x80;	// ○ と △
			report[6] = 0x04 | 0x08 | 0x10 | 0x40 | 0x80;	// L2・R2・SHARE・L3・R3
			report[7] = 0x01;	// PS
			PAD_STATE state{};
			Assert::IsTrue(PadCalcParseDs4UsbReport(report, 64, state));
			Assert::AreEqual((unsigned int)(PAD_BUTTON_CIRCLE | PAD_BUTTON_TRIANGLE | PAD_BUTTON_L2 | PAD_BUTTON_R2
				| PAD_BUTTON_SHARE | PAD_BUTTON_L3 | PAD_BUTTON_R3 | PAD_BUTTON_PS), state.Buttons);
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

		// 8〜15 は方向を押していない。直前の方向が残らず、ほかのボタンは読める。
		TEST_METHOD(Parse_DpadNeutralAndReservedValues)
		{
			unsigned char report[64];
			MakeNeutralReport(report);
			PAD_STATE state{};
			for (int direction = 8; direction < 16; ++direction) {
				report[5] = 0x07;
				Assert::IsTrue(PadCalcParseDs4UsbReport(report, 64, state));
				report[5] = (unsigned char)direction;
				Assert::IsTrue(PadCalcParseDs4UsbReport(report, 64, state));
				Assert::AreEqual(0u, state.Buttons);
				report[5] |= 0xF0;
				Assert::IsTrue(PadCalcParseDs4UsbReport(report, 64, state));
				Assert::AreEqual((unsigned int)(PAD_BUTTON_SQUARE | PAD_BUTTON_CROSS
					| PAD_BUTTON_CIRCLE | PAD_BUTTON_TRIANGLE), state.Buttons);
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

		// 0 と 1 がそれぞれ 0 と 255、半端な値は四捨五入される。
		TEST_METHOD(Output_EndpointsAndRounding)
		{
			unsigned char report[32];
			Assert::IsTrue(PadCalcBuildDs4UsbOutput(report, 32, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f));
			Assert::AreEqual(0, (int)report[4]);
			Assert::AreEqual(255, (int)report[5]);
			Assert::AreEqual(255, (int)report[6]);
			Assert::AreEqual(0, (int)report[7]);
			Assert::AreEqual(255, (int)report[8]);
			Assert::IsTrue(PadCalcBuildDs4UsbOutput(report, 32, 0.5f, 0.1f, 0.9f, 0.25f, 0.75f));
			const int expected[5] = { 128, 26, 230, 64, 191 };
			for (int i = 0; i < 5; ++i) Assert::AreEqual(expected[i], (int)report[i + 4]);
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

		// HID が返した長さだけ書き、長いレポートの末尾も 0 にする（前後の番兵は保つ）。
		TEST_METHOD(Output_UsesGivenLength)
		{
			unsigned char storage[66];
			for (unsigned char& byte : storage) byte = 0xAA;
			Assert::IsTrue(PadCalcBuildDs4UsbOutput(storage + 1, 64, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f));
			Assert::AreEqual(0xAA, (int)storage[0]);
			Assert::AreEqual(0xAA, (int)storage[65]);
			for (int i = 3; i <= 64; ++i) Assert::AreEqual(0, (int)storage[i]);
			unsigned char minimum[9];
			Assert::IsTrue(PadCalcBuildDs4UsbOutput(minimum, 9, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f));
			Assert::AreEqual(255, (int)minimum[8]);
		}

		// 短い出力先や null は拒否し、渡されたバッファを書き換えない。
		TEST_METHOD(Output_RejectsInvalidBuffer)
		{
			unsigned char report[32];
			for (unsigned char& byte : report) byte = 0xAA;
			Assert::IsFalse(PadCalcBuildDs4UsbOutput(nullptr, 32, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f));
			Assert::IsFalse(PadCalcBuildDs4UsbOutput(report, 8, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f));
			Assert::IsFalse(PadCalcBuildDs4UsbOutput(report, 0, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f));
			Assert::IsFalse(PadCalcBuildDs4UsbOutput(report, -1, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f));
			for (unsigned char byte : report) Assert::AreEqual(0xAA, (int)byte);
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

		// 強く短い要求は時間を縮めず、弱く長い要求は強さを下げずに時間を延ばす。
		TEST_METHOD(Rumble_StrengthAndTimeAreIndependent)
		{
			PAD_RUMBLE rumble{ 0.2f, 0.3f, 0.4f };
			PadCalcRumble(rumble, 0.8f, 0.7f, 0.1f);
			Assert::AreEqual(0.8f, rumble.SmallMotor, PAD_EPSILON);
			Assert::AreEqual(0.7f, rumble.LargeMotor, PAD_EPSILON);
			Assert::AreEqual(0.4f, rumble.RemainingSeconds, PAD_EPSILON);
			PadCalcUpdateRumble(rumble, 0.3f);
			PadCalcRumble(rumble, 0.1f, 0.2f, 0.25f);
			Assert::AreEqual(0.8f, rumble.SmallMotor, PAD_EPSILON);
			Assert::AreEqual(0.7f, rumble.LargeMotor, PAD_EPSILON);
			Assert::AreEqual(0.25f, rumble.RemainingSeconds, PAD_EPSILON);
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

		// 時間を進めないときに、残り時間が増えたり強さが変わったりしない。
		TEST_METHOD(Rumble_ZeroAndNegativeDeltaDoNotAdvance)
		{
			PAD_RUMBLE rumble{ 0.4f, 0.8f, 0.5f };
			PadCalcUpdateRumble(rumble, 0.0f);
			PadCalcUpdateRumble(rumble, -1.0f);
			Assert::AreEqual(0.4f, rumble.SmallMotor, PAD_EPSILON);
			Assert::AreEqual(0.8f, rumble.LargeMotor, PAD_EPSILON);
			Assert::AreEqual(0.5f, rumble.RemainingSeconds, PAD_EPSILON);
		}

		// やり直し・時間切れ用の明示的な停止は、強い振動が残っていても止める。
		TEST_METHOD(Rumble_NonPositiveDurationStops)
		{
			PAD_RUMBLE rumble{ 1.0f, 1.0f, 0.4f };
			PadCalcRumble(rumble, 0.0f, 0.0f, 0.0f);
			Assert::AreEqual(0.0f, rumble.SmallMotor, PAD_EPSILON);
			Assert::AreEqual(0.0f, rumble.LargeMotor, PAD_EPSILON);
			Assert::AreEqual(0.0f, rumble.RemainingSeconds, PAD_EPSILON);
			PadCalcRumble(rumble, 0.6f, 0.6f, 0.15f);
			PadCalcRumble(rumble, 1.0f, 1.0f, -1.0f);
			Assert::AreEqual(0.0f, rumble.SmallMotor, PAD_EPSILON);
			Assert::AreEqual(0.0f, rumble.LargeMotor, PAD_EPSILON);
			Assert::AreEqual(0.0f, rumble.RemainingSeconds, PAD_EPSILON);
		}

		// 時間が終わった状態の古い強さを、新しい振動へ持ち越さない。
		TEST_METHOD(Rumble_DiscardsExpiredStrength)
		{
			PAD_RUMBLE rumble{ 1.0f, 1.0f, 0.0f };
			PadCalcRumble(rumble, 0.4f, 0.0f, 0.1f);
			Assert::AreEqual(0.4f, rumble.SmallMotor, PAD_EPSILON);
			Assert::AreEqual(0.0f, rumble.LargeMotor, PAD_EPSILON);
			Assert::AreEqual(0.1f, rumble.RemainingSeconds, PAD_EPSILON);
			rumble = { 1.0f, 1.0f, -1.0f };
			PadCalcUpdateRumble(rumble, 0.0f);
			Assert::AreEqual(0.0f, rumble.SmallMotor, PAD_EPSILON);
			Assert::AreEqual(0.0f, rumble.LargeMotor, PAD_EPSILON);
			Assert::AreEqual(0.0f, rumble.RemainingSeconds, PAD_EPSILON);
		}
	};
}
