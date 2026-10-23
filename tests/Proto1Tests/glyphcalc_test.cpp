// render/glyphcalc.h（数字と文字を四角の一覧に変える計算）の単体テスト
#include "CppUnitTest.h"
#include "render/glyphcalc.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace Proto1Tests
{
	static constexpr float GLYPH_EPSILON = 0.001f;	// 許す誤差

	// 7 セグの棒の番号
	static constexpr int SEG_A = 0, SEG_B = 1, SEG_C = 2, SEG_D = 3, SEG_G = 6;

	// 点いている棒の数
	static int CountBits(unsigned char bits)
	{
		int count = 0;
		for (int i = 0; i < 8; i++) {
			if (bits & (1 << i)) count++;
		}
		return count;
	}

	TEST_CLASS(GlyphCalcTest)
	{
	public:
		// 桁に分ける：0 → [0]、90 → [9,0]、7 を2桁 → [0,7]
		TEST_METHOD(SplitDigits_Basic)
		{
			int digits[10] = {};
			Assert::AreEqual(1, GlyphCalcSplitDigits(0, 1, digits, 10));
			Assert::AreEqual(0, digits[0]);

			Assert::AreEqual(2, GlyphCalcSplitDigits(90, 1, digits, 10));
			Assert::AreEqual(9, digits[0]);
			Assert::AreEqual(0, digits[1]);

			Assert::AreEqual(2, GlyphCalcSplitDigits(7, 2, digits, 10));
			Assert::AreEqual(0, digits[0]);
			Assert::AreEqual(7, digits[1]);
		}

		// 負は 0、入りきらなければ全部 9
		TEST_METHOD(SplitDigits_NegativeAndOverflow)
		{
			int digits[10] = {};
			Assert::AreEqual(1, GlyphCalcSplitDigits(-5, 1, digits, 10));
			Assert::AreEqual(0, digits[0]);

			Assert::AreEqual(3, GlyphCalcSplitDigits(1234, 1, digits, 3));
			Assert::AreEqual(9, digits[0]);
			Assert::AreEqual(9, digits[1]);
			Assert::AreEqual(9, digits[2]);
		}

		// 7 セグ：8 は7本、1 は b・c、7 は a・b・c、0 は g が消えている、範囲外は 0
		TEST_METHOD(Segments_Digits)
		{
			Assert::AreEqual(7, CountBits(GlyphCalcSegments(8)));
			Assert::AreEqual((int)((1 << SEG_B) | (1 << SEG_C)), (int)GlyphCalcSegments(1));
			Assert::AreEqual((int)((1 << SEG_A) | (1 << SEG_B) | (1 << SEG_C)), (int)GlyphCalcSegments(7));
			Assert::AreEqual(6, CountBits(GlyphCalcSegments(0)));
			Assert::IsTrue((GlyphCalcSegments(0) & (1 << SEG_G)) == 0);
			Assert::AreEqual(0, (int)GlyphCalcSegments(-1));
			Assert::AreEqual(0, (int)GlyphCalcSegments(10));
		}

		// 棒の位置：a は一番上で横長、g は縦の真ん中、d の下の端は 2W
		TEST_METHOD(SegmentRect_Positions)
		{
			const float W = 10.0f;
			const RECT2D a = GlyphCalcSegmentRect(SEG_A, 100.0f, 50.0f, W);
			Assert::AreEqual(100.0f, a.X, GLYPH_EPSILON);
			Assert::AreEqual(50.0f, a.Y, GLYPH_EPSILON);
			Assert::IsTrue(a.Width > a.Height);

			const RECT2D g = GlyphCalcSegmentRect(SEG_G, 100.0f, 50.0f, W);
			Assert::AreEqual(50.0f + W, g.Y + g.Height * 0.5f, GLYPH_EPSILON);

			const RECT2D d = GlyphCalcSegmentRect(SEG_D, 100.0f, 50.0f, W);
			Assert::AreEqual(50.0f + W * 2.0f, d.Y + d.Height, GLYPH_EPSILON);
		}

		// 90 は 12 個の四角（9 が6本・0 が6本）
		TEST_METHOD(NumberRects_Count)
		{
			RECT2D rects[64];
			Assert::AreEqual(12, GlyphCalcNumberRects(90, 1, 0.0f, 0.0f, 10.0f, GLYPH_ALIGN_LEFT, rects, 64));
		}

		// 右寄せは一番右の四角の右端が x、中央寄せは全体の幅の半分だけ左
		TEST_METHOD(NumberRects_Align)
		{
			RECT2D rects[64];
			const float W = 10.0f;
			const float width = GlyphCalcNumberWidth(90, 1, W);

			int count = GlyphCalcNumberRects(90, 1, 300.0f, 0.0f, W, GLYPH_ALIGN_RIGHT, rects, 64);
			float right = 0.0f;
			for (int i = 0; i < count; i++) {
				if (rects[i].X + rects[i].Width > right) right = rects[i].X + rects[i].Width;
			}
			Assert::AreEqual(300.0f, right, GLYPH_EPSILON);

			count = GlyphCalcNumberRects(90, 1, 300.0f, 0.0f, W, GLYPH_ALIGN_CENTER, rects, 64);
			float left = 1.0e9f;
			for (int i = 0; i < count; i++) {
				if (rects[i].X < left) left = rects[i].X;
			}
			Assert::AreEqual(300.0f - width * 0.5f, left, GLYPH_EPSILON);
		}

		// maxRects を超えて書かない
		TEST_METHOD(NumberRects_MaxRects)
		{
			RECT2D rects[8];
			rects[5] = { -1.0f, -1.0f, -1.0f, -1.0f };
			Assert::AreEqual(5, GlyphCalcNumberRects(88, 1, 0.0f, 0.0f, 10.0f, GLYPH_ALIGN_LEFT, rects, 5));
			Assert::AreEqual(-1.0f, rects[5].X);
		}

		// 文字：T の1行目は 0b11111、知らない文字は 0
		TEST_METHOD(LetterRow_Basic)
		{
			Assert::AreEqual(0x1F, (int)GlyphCalcLetterRow('T', 0));
			Assert::AreEqual(0, (int)GlyphCalcLetterRow('Z', 0));
			Assert::AreEqual(0, (int)GlyphCalcLetterRow(' ', 3));
			Assert::AreEqual(0, (int)GlyphCalcLetterRow('T', 7));
		}

		// "TIME UP" の四角の数は、ドットの数の合計と同じ
		TEST_METHOD(TextRects_CountMatchesDots)
		{
			const char* text = "TIME UP";
			int dots = 0;
			for (int i = 0; text[i] != '\0'; i++) {
				for (int row = 0; row < 7; row++) {
					dots += CountBits(GlyphCalcLetterRow(text[i], row));
				}
			}
			RECT2D rects[512];
			Assert::IsTrue(dots > 0);
			Assert::AreEqual(dots, GlyphCalcTextRects(text, 0.0f, 0.0f, 4.0f, GLYPH_ALIGN_LEFT, rects, 512));
		}

		// %：四捨五入して 0〜999
		TEST_METHOD(DisplayPercent_RoundAndClamp)
		{
			Assert::AreEqual(12, GlyphCalcDisplayPercent(12.4f));
			Assert::AreEqual(13, GlyphCalcDisplayPercent(12.5f));
			Assert::AreEqual(0, GlyphCalcDisplayPercent(-1.0f));
			Assert::AreEqual(999, GlyphCalcDisplayPercent(5000.0f));
		}
	};
}
