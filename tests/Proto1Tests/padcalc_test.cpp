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

		// 方向キーはゲームのボタンにしない（下位4ビットが何でも、□×○△ にならない）
		TEST_METHOD(Parse_DpadIsIgnored)
		{
			unsigned char report[64];
			MakeNeutralReport(report);
			report[5] = 0x07;	// 方向キーの左上
			PAD_STATE state{};
			Assert::IsTrue(PadCalcParseDs4UsbReport(report, 64, state));
			Assert::AreEqual(0u, state.Buttons);
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
	};
}
