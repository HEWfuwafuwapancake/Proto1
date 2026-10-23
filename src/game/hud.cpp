#include "game/hud.h"
#include "core/dx11.h"
#include "game/camera.h"
#include "game/dummy.h"
#include "game/match.h"
#include "render/number2d.h"


//------------------------------------------------------------------------------
//	調整用の値（位置・大きさはピクセル。画面は SCREEN_WIDTH × SCREEN_HEIGHT）
//------------------------------------------------------------------------------
static constexpr XMFLOAT4 TEXT_COLOR = { 1.0f, 1.0f, 1.0f, 1.0f };		// 文字の色（白）
static constexpr XMFLOAT4 COVER_COLOR = { 0.0f, 0.0f, 0.0f, 0.6f };		// 時間切れのおおい（半透明の黒）

static constexpr float SECONDS_DIGIT_WIDTH = 40.0f;	// 残り秒数の桁の幅（高さはこの2倍）
static constexpr float SECONDS_Y = 20.0f;			// 残り秒数の上端（横は画面の真ん中）

static constexpr float SCORE_DIGIT_WIDTH = 28.0f;	// 左上のスコアの桁の幅
static constexpr float SCORE_X = 30.0f;				// 左上のスコアの左端
static constexpr float SCORE_Y = 20.0f;				// 左上のスコアの上端

static constexpr float PERCENT_DIGIT_WIDTH = 14.0f;	// 頭の上の % の桁の幅
static constexpr float PERCENT_HEAD_OFFSET = 0.4f;	// 頭のてっぺんから、% を出す点までの高さ（ワールド）
static constexpr float PERCENT_SIGN_GAP = 4.0f;		// 数字と「%」の間

static constexpr float TIME_UP_DOT = 10.0f;				// 「TIME UP」のドットの大きさ
static constexpr float TIME_UP_Y = 190.0f;				// 「TIME UP」の上端
static constexpr float FINAL_SCORE_DIGIT_WIDTH = 50.0f;	// 時間切れのスコアの桁の幅
static constexpr float FINAL_SCORE_Y = 300.0f;			// 時間切れのスコアの上端
static constexpr float RESTART_DOT = 5.0f;				// 「PRESS R / OPTIONS」のドットの大きさ
static constexpr float RESTART_Y = 470.0f;				// 「PRESS R / OPTIONS」の上端


// ダミーの頭の上に「12%」を出す（復活待ち・カメラの後ろのときは出さない）
static void DrawDummyPercent(int index)
{
	if (!DummyIsActive(index)) return;

	// 頭のてっぺん（縦のカプセルの上の端 ＋ 半径）の少し上
	const CAPSULE body = DummyGetTarget(index).Body;
	const float top = (body.Start.Y > body.End.Y ? body.Start.Y : body.End.Y) + body.Radius;
	const XMFLOAT3 head = { body.Start.X, top + PERCENT_HEAD_OFFSET, body.Start.Z };
	XMFLOAT2 screen;
	if (!CameraWorldToScreen(head, &screen)) return;

	// 数字と「%」を合わせた幅の真ん中を頭に合わせ、下の端を頭の上の点にそろえる
	const int percent = GlyphCalcDisplayPercent(DummyGetPercent(index));
	const float height = PERCENT_DIGIT_WIDTH * 2.0f;
	const float dot = height / 7.0f;	// 「%」は数字と同じ高さ（7 ドット）
	const float numberWidth = GlyphCalcNumberWidth(percent, 1, PERCENT_DIGIT_WIDTH);
	const float totalWidth = numberWidth + PERCENT_SIGN_GAP + GlyphCalcTextWidth("%", dot);
	const float left = screen.x - totalWidth * 0.5f;
	const float y = screen.y - height;
	Number2DDrawNumber(percent, 1, left, y, PERCENT_DIGIT_WIDTH, GLYPH_ALIGN_LEFT, TEXT_COLOR);
	Number2DDrawText("%", left + numberWidth + PERCENT_SIGN_GAP, y, dot, GLYPH_ALIGN_LEFT, TEXT_COLOR);
}


void HudInitialize(void)
{

}

void HudFinalize(void)
{

}

void HudUpdate(void)
{

}

void HudDraw(void)
{
	const float centerX = (float)SCREEN_WIDTH * 0.5f;

	for (int i = 0; i < DUMMY_COUNT; i++) {
		DrawDummyPercent(i);
	}

	if (MatchGetState() == MATCH_STATE_FINISHED) {
		// 時間切れ：おおいの上に TIME UP・スコア・やり直しの案内（左上のスコアは出さない）
		Number2DDrawBox({ 0.0f, 0.0f, (float)SCREEN_WIDTH, (float)SCREEN_HEIGHT }, COVER_COLOR);
		Number2DDrawText("TIME UP", centerX, TIME_UP_Y, TIME_UP_DOT, GLYPH_ALIGN_CENTER, TEXT_COLOR);
		Number2DDrawNumber(MatchGetScore(), 1, centerX, FINAL_SCORE_Y, FINAL_SCORE_DIGIT_WIDTH, GLYPH_ALIGN_CENTER, TEXT_COLOR);
		Number2DDrawText("PRESS R / OPTIONS", centerX, RESTART_Y, RESTART_DOT, GLYPH_ALIGN_CENTER, TEXT_COLOR);
	}
	else {
		Number2DDrawNumber(MatchGetScore(), 1, SCORE_X, SCORE_Y, SCORE_DIGIT_WIDTH, GLYPH_ALIGN_LEFT, TEXT_COLOR);
	}

	// 残り秒数（時間切れでも 0 のまま、おおいの上に出す）
	Number2DDrawNumber(MatchGetRemainingSeconds(), 1, centerX, SECONDS_Y, SECONDS_DIGIT_WIDTH, GLYPH_ALIGN_CENTER, TEXT_COLOR);
}
