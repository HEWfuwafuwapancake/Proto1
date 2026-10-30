#include "render/glyphcalc.h"
#include <math.h>


//------------------------------------------------------------------------------
//	調整用の値（桁の幅 W に対する比）
//------------------------------------------------------------------------------
static constexpr float SEGMENT_THICKNESS = 0.2f;	// 棒の太さ
static constexpr float DIGIT_GAP = 0.3f;			// 桁の間
static constexpr int MAX_DIGITS = 10;				// int の最大の桁数

static constexpr int LETTER_COLUMNS = 5;			// 文字の横のドット数
static constexpr int LETTER_ROWS = 7;				// 文字の縦のドット数
static constexpr int LETTER_ADVANCE = 6;			// 1文字分の幅（ドット。5 ＋ 文字の間 1）
static constexpr int MAX_PERCENT = 999;				// 出す % のいちばん大きい値


// 0〜9 で点ける棒（bit0=a 〜 bit6=g）
static constexpr unsigned char DIGIT_SEGMENTS[10] = {
	0x3F,	// 0：a b c d e f
	0x06,	// 1：b c
	0x5B,	// 2：a b d e g
	0x4F,	// 3：a b c d g
	0x66,	// 4：b c f g
	0x6D,	// 5：a c d f g
	0x7D,	// 6：a c d e f g
	0x07,	// 7：a b c
	0x7F,	// 8：全部
	0x6F,	// 9：a b c d f g
};

// 5×7 ドットの文字（1行が下位5ビット、bit4 が左端）
struct LETTER
{
	char Code;
	unsigned char Rows[LETTER_ROWS];
};

static constexpr LETTER LETTERS[] = {
	{ 'A', { 0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11 } },
	{ 'B', { 0x1E, 0x11, 0x11, 0x1E, 0x11, 0x11, 0x1E } },
	{ 'C', { 0x0E, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0E } },
	{ 'D', { 0x1E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1E } },
	{ 'E', { 0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F } },
	{ 'G', { 0x0E, 0x11, 0x10, 0x17, 0x11, 0x11, 0x0E } },
	{ 'H', { 0x11, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11 } },
	{ 'I', { 0x0E, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0E } },
	{ 'J', { 0x07, 0x02, 0x02, 0x02, 0x12, 0x12, 0x0C } },
	{ 'K', { 0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11 } },
	{ 'L', { 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F } },
	{ 'M', { 0x11, 0x1B, 0x15, 0x15, 0x11, 0x11, 0x11 } },
	{ 'N', { 0x11, 0x19, 0x15, 0x13, 0x11, 0x11, 0x11 } },
	{ 'O', { 0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E } },
	{ 'P', { 0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10 } },
	{ 'R', { 0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11 } },
	{ 'S', { 0x0F, 0x10, 0x10, 0x0E, 0x01, 0x01, 0x1E } },
	{ 'T', { 0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04 } },
	{ 'U', { 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E } },
	{ 'V', { 0x11, 0x11, 0x11, 0x11, 0x11, 0x0A, 0x04 } },
	{ 'W', { 0x11, 0x11, 0x11, 0x15, 0x15, 0x1B, 0x11 } },
	{ 'X', { 0x11, 0x11, 0x0A, 0x04, 0x0A, 0x11, 0x11 } },
	{ 'Y', { 0x11, 0x11, 0x0A, 0x04, 0x04, 0x04, 0x04 } },
	{ '1', { 0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E } },
	{ '0', { 0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E } },
	{ '2', { 0x0E, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1F } },
	{ '3', { 0x1E, 0x01, 0x01, 0x0E, 0x01, 0x01, 0x1E } },
	{ '4', { 0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02 } },
	{ '5', { 0x1F, 0x10, 0x10, 0x1E, 0x01, 0x01, 0x1E } },
	{ '6', { 0x0E, 0x10, 0x10, 0x1E, 0x11, 0x11, 0x0E } },
	{ '7', { 0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08 } },
	{ '8', { 0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E } },
	{ '9', { 0x0E, 0x11, 0x11, 0x0F, 0x01, 0x01, 0x0E } },
	{ '.', { 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x0C } },
	{ '%', { 0x18, 0x19, 0x02, 0x04, 0x08, 0x13, 0x03 } },
	{ '/', { 0x01, 0x01, 0x02, 0x04, 0x08, 0x10, 0x10 } },
};


// 幅 width のものを x に align で合わせたときの左端
static float AlignLeft(float x, float width, GLYPH_ALIGN align)
{
	if (align == GLYPH_ALIGN_CENTER) return x - width * 0.5f;
	if (align == GLYPH_ALIGN_RIGHT) return x - width;
	return x;
}


int GlyphCalcSplitDigits(int value, int minDigits, int digits[], int maxDigits)
{
	if (maxDigits <= 0) return 0;
	if (value < 0) value = 0;

	// 下の桁から数える
	int reversed[MAX_DIGITS];
	int count = 0;
	do {
		reversed[count++] = value % 10;
		value /= 10;
	} while (value > 0 && count < MAX_DIGITS);
	while (count < minDigits && count < MAX_DIGITS) {
		reversed[count++] = 0;
	}

	// 入りきらなければ全部 9
	if (count > maxDigits) {
		for (int i = 0; i < maxDigits; i++) {
			digits[i] = 9;
		}
		return maxDigits;
	}
	for (int i = 0; i < count; i++) {
		digits[i] = reversed[count - 1 - i];
	}
	return count;
}

unsigned char GlyphCalcSegments(int digit)
{
	if (digit < 0 || digit > 9) return 0;
	return DIGIT_SEGMENTS[digit];
}

RECT2D GlyphCalcSegmentRect(int segment, float x, float y, float digitWidth)
{
	const float w = digitWidth;
	const float t = digitWidth * SEGMENT_THICKNESS;
	switch (segment) {
	case 0: return { x,         y,                  w, t };	// a 上
	case 1: return { x + w - t, y,                  t, w };	// b 右上
	case 2: return { x + w - t, y + w,              t, w };	// c 右下
	case 3: return { x,         y + w * 2.0f - t,   w, t };	// d 下
	case 4: return { x,         y + w,              t, w };	// e 左下
	case 5: return { x,         y,                  t, w };	// f 左上
	case 6: return { x,         y + w - t * 0.5f,   w, t };	// g 真ん中
	default: return { x, y, 0.0f, 0.0f };
	}
}

float GlyphCalcNumberWidth(int value, int minDigits, float digitWidth)
{
	int digits[MAX_DIGITS];
	const int count = GlyphCalcSplitDigits(value, minDigits, digits, MAX_DIGITS);
	return digitWidth * (float)count + digitWidth * DIGIT_GAP * (float)(count - 1);
}

int GlyphCalcNumberRects(int value, int minDigits, float x, float y, float digitWidth,
	GLYPH_ALIGN align, RECT2D out[], int maxRects)
{
	int digits[MAX_DIGITS];
	const int count = GlyphCalcSplitDigits(value, minDigits, digits, MAX_DIGITS);
	const float left = AlignLeft(x, GlyphCalcNumberWidth(value, minDigits, digitWidth), align);
	const float advance = digitWidth * (1.0f + DIGIT_GAP);

	int written = 0;
	for (int i = 0; i < count; i++) {
		const unsigned char segments = GlyphCalcSegments(digits[i]);
		for (int s = 0; s < 7; s++) {
			if ((segments & (1 << s)) == 0) continue;
			if (written >= maxRects) return written;
			out[written++] = GlyphCalcSegmentRect(s, left + advance * (float)i, y, digitWidth);
		}
	}
	return written;
}

unsigned char GlyphCalcLetterRow(char c, int row)
{
	if (row < 0 || row >= LETTER_ROWS) return 0;
	for (const LETTER& letter : LETTERS) {
		if (letter.Code == c) return letter.Rows[row];
	}
	return 0;	// 空白・知らない文字
}

float GlyphCalcTextWidth(const char* text, float dotSize)
{
	int length = 0;
	while (text[length] != '\0') length++;
	if (length == 0) return 0.0f;
	return dotSize * (float)(length * LETTER_ADVANCE - (LETTER_ADVANCE - LETTER_COLUMNS));
}

int GlyphCalcTextRects(const char* text, float x, float y, float dotSize,
	GLYPH_ALIGN align, RECT2D out[], int maxRects)
{
	const float left = AlignLeft(x, GlyphCalcTextWidth(text, dotSize), align);

	int written = 0;
	for (int i = 0; text[i] != '\0'; i++) {
		const float charLeft = left + dotSize * (float)(i * LETTER_ADVANCE);
		for (int row = 0; row < LETTER_ROWS; row++) {
			const unsigned char bits = GlyphCalcLetterRow(text[i], row);
			for (int col = 0; col < LETTER_COLUMNS; col++) {
				if ((bits & (1 << (LETTER_COLUMNS - 1 - col))) == 0) continue;
				if (written >= maxRects) return written;
				out[written++] = { charLeft + dotSize * (float)col, y + dotSize * (float)row, dotSize, dotSize };
			}
		}
	}
	return written;
}

int GlyphCalcDisplayPercent(float percent)
{
	// int に変える前に範囲に収める（とても大きい値をそのまま int にするとあふれる）
	const float rounded = floorf(percent + 0.5f);
	if (rounded < 0.0f) return 0;
	if (rounded > (float)MAX_PERCENT) return MAX_PERCENT;
	return (int)rounded;
}
