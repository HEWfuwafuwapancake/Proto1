#include "game/hud.h"
#include "core/dx11.h"
#include "game/camera.h"
#include "game/fightermanager.h"
#include "game/match.h"
#include "physics/scenecalc.h"
#include "render/number2d.h"


//------------------------------------------------------------------------------
//	調整用の値（位置・大きさはピクセル。画面は SCREEN_WIDTH × SCREEN_HEIGHT）
//------------------------------------------------------------------------------
static constexpr XMFLOAT4 TEXT_COLOR = { 1.0f, 1.0f, 1.0f, 1.0f };		// 文字の色（白）
static constexpr XMFLOAT4 COVER_COLOR = { 0.0f, 0.0f, 0.0f, 0.6f };		// シーンのおおい（半透明の黒）

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
static constexpr float NEXT_DOT = 5.0f;				// 「NEXT O / ENTER」のドットの大きさ
static constexpr float NEXT_Y = 470.0f;				// 結果への案内の上端

static constexpr float TITLE_DOT = 12.0f;			// 「PROTO」のドットの大きさ
static constexpr float TITLE_Y = 80.0f;
static constexpr float TITLE_START_Y = 220.0f;
static constexpr float TITLE_SELECT_Y = 330.0f;
static constexpr float TITLE_DECIDE_Y = 375.0f;
static constexpr float TITLE_MOVE_Y = 470.0f;
static constexpr float TITLE_ARMS_Y = 515.0f;
static constexpr float TITLE_JUMP_Y = 560.0f;
static constexpr float RESULT_DOT = 10.0f;			// 「RESULT」のドットの大きさ
static constexpr float RESULT_Y = 150.0f;
static constexpr float RESULT_MENU_Y = 440.0f;
static constexpr float RESULT_SELECT_Y = 610.0f;
static constexpr float RESULT_DECIDE_Y = 655.0f;
static constexpr float MENU_DOT = 7.0f;
static constexpr float MENU_LINE_GAP = 75.0f;		// メニューの上端どうしの間隔
static constexpr float MENU_HELP_DOT = 4.0f;
static constexpr float MENU_HOVER_MARGIN_X = 20.0f;
static constexpr float MENU_HOVER_MARGIN_Y = 8.0f;
static constexpr XMFLOAT4 MENU_HOVER_COLOR = { 1.0f, 1.0f, 1.0f, 0.2f };
static constexpr XMFLOAT4 MENU_SELECTED_COLOR = { 1.0f, 0.9f, 0.2f, 1.0f };
static constexpr XMFLOAT4 MENU_TEXT_COLOR = { 0.7f, 0.7f, 0.7f, 1.0f };


// ダミーの頭の上に「12%」を出す（復活待ち・カメラの後ろのときは出さない）
void Hud::DrawFighterPercent(int index) const
{
	if (index == m_Slot || !m_Fighters->IsActive(index)) return;

	// 頭のてっぺん（縦のカプセルの上の端 ＋ 半径）の少し上
	const CAPSULE body = m_Fighters->GetTarget(index, m_Slot).Body;
	const float top = (body.Start.Y > body.End.Y ? body.Start.Y : body.End.Y) + body.Radius;
	const XMFLOAT3 head = { body.Start.X, top + PERCENT_HEAD_OFFSET, body.Start.Z };
	XMFLOAT2 screen;
	if (!m_Camera->WorldToScreen(head, &screen)) return;

	// 数字と「%」を合わせた幅の真ん中を頭に合わせ、下の端を頭の上の点にそろえる
	const int percent = GlyphCalcDisplayPercent(m_Fighters->GetPercent(index));
	const float height = PERCENT_DIGIT_WIDTH * 2.0f;
	const float dot = height / 7.0f;	// 「%」は数字と同じ高さ（7 ドット）
	const float numberWidth = GlyphCalcNumberWidth(percent, 1, PERCENT_DIGIT_WIDTH);
	const float totalWidth = numberWidth + PERCENT_SIGN_GAP + GlyphCalcTextWidth("%", dot);
	const float left = screen.x - totalWidth * 0.5f;
	const float y = screen.y - height;
	Number2DDrawNumber(percent, 1, left, y, PERCENT_DIGIT_WIDTH, GLYPH_ALIGN_LEFT, TEXT_COLOR);
	Number2DDrawText("%", left + numberWidth + PERCENT_SIGN_GAP, y, dot, GLYPH_ALIGN_LEFT, TEXT_COLOR);
}


void Hud::Reset(void)
{

}

void Hud::Finalize(void)
{

}

void Hud::Update(void)
{

}

void Hud::DrawMenuItem(const char* text, float y, bool selected) const
{
	const float centerX = (float)SCREEN_WIDTH * 0.5f;
	if (selected) {
		const float width = GlyphCalcTextWidth(text, MENU_DOT);
		Number2DDrawBox({ centerX - width * 0.5f - MENU_HOVER_MARGIN_X, y - MENU_HOVER_MARGIN_Y,
			width + MENU_HOVER_MARGIN_X * 2.0f, MENU_DOT * 7.0f + MENU_HOVER_MARGIN_Y * 2.0f }, MENU_HOVER_COLOR);
	}
	Number2DDrawText(text, centerX, y, MENU_DOT, GLYPH_ALIGN_CENTER, selected ? MENU_SELECTED_COLOR : MENU_TEXT_COLOR);
}

void Hud::DrawMenuHelp(float selectY, float decideY) const
{
	const float centerX = (float)SCREEN_WIDTH * 0.5f;
	Number2DDrawText("SELECT  STICK / W S", centerX, selectY, MENU_HELP_DOT, GLYPH_ALIGN_CENTER, TEXT_COLOR);
	Number2DDrawText("OK  O / ENTER", centerX, decideY, MENU_HELP_DOT, GLYPH_ALIGN_CENTER, TEXT_COLOR);
}

void Hud::Draw(void) const
{
	const float centerX = (float)SCREEN_WIDTH * 0.5f;

	if (m_Scene->GetScene() == SCENE_TITLE) {
		Number2DDrawBox({ 0.0f, 0.0f, (float)SCREEN_WIDTH, (float)SCREEN_HEIGHT }, COVER_COLOR);
		Number2DDrawText("PROTO", centerX, TITLE_Y, TITLE_DOT, GLYPH_ALIGN_CENTER, TEXT_COLOR);
		DrawMenuItem("START", TITLE_START_Y, true);
		DrawMenuHelp(TITLE_SELECT_Y, TITLE_DECIDE_Y);
		Number2DDrawText("MOVE  WASD / L STICK", centerX, TITLE_MOVE_Y, MENU_HELP_DOT, GLYPH_ALIGN_CENTER, TEXT_COLOR);
		Number2DDrawText("ARMS  L1 R1 / CLICK", centerX, TITLE_ARMS_Y, MENU_HELP_DOT, GLYPH_ALIGN_CENTER, TEXT_COLOR);
		Number2DDrawText("JUMP  SPACE / X", centerX, TITLE_JUMP_Y, MENU_HELP_DOT, GLYPH_ALIGN_CENTER, TEXT_COLOR);
		return;
	}
	if (m_Scene->GetScene() == SCENE_RESULT) {
		Number2DDrawBox({ 0.0f, 0.0f, (float)SCREEN_WIDTH, (float)SCREEN_HEIGHT }, COVER_COLOR);
		Number2DDrawText("RESULT", centerX, RESULT_Y, RESULT_DOT, GLYPH_ALIGN_CENTER, TEXT_COLOR);
		Number2DDrawNumber(m_Match->GetScore(), 1, centerX, FINAL_SCORE_Y, FINAL_SCORE_DIGIT_WIDTH, GLYPH_ALIGN_CENTER, TEXT_COLOR);
		DrawMenuItem("RETRY", RESULT_MENU_Y, m_Scene->GetCursor() == RESULT_ITEM_RETRY);
		DrawMenuItem("TITLE", RESULT_MENU_Y + MENU_LINE_GAP, m_Scene->GetCursor() == RESULT_ITEM_TITLE);
		DrawMenuHelp(RESULT_SELECT_Y, RESULT_DECIDE_Y);
		return;
	}

	for (int i = 0; i < FIGHTER_COUNT; i++) {
		DrawFighterPercent(i);
	}

	if (m_Match->GetState() == MATCH_STATE_FINISHED) {
		// 時間切れ：おおいの上に TIME UP・スコア・結果への案内（左上のスコアは出さない）
		Number2DDrawBox({ 0.0f, 0.0f, (float)SCREEN_WIDTH, (float)SCREEN_HEIGHT }, COVER_COLOR);
		Number2DDrawText("TIME UP", centerX, TIME_UP_Y, TIME_UP_DOT, GLYPH_ALIGN_CENTER, TEXT_COLOR);
		Number2DDrawNumber(m_Match->GetScore(), 1, centerX, FINAL_SCORE_Y, FINAL_SCORE_DIGIT_WIDTH, GLYPH_ALIGN_CENTER, TEXT_COLOR);
		Number2DDrawText("NEXT  O / ENTER", centerX, NEXT_Y, NEXT_DOT, GLYPH_ALIGN_CENTER, TEXT_COLOR);
	}
	else {
		Number2DDrawNumber(m_Match->GetScore(), 1, SCORE_X, SCORE_Y, SCORE_DIGIT_WIDTH, GLYPH_ALIGN_LEFT, TEXT_COLOR);
	}

	// 残り秒数（時間切れでも 0 のまま、おおいの上に出す）
	Number2DDrawNumber(m_Match->GetRemainingSeconds(), 1, centerX, SECONDS_Y, SECONDS_DIGIT_WIDTH, GLYPH_ALIGN_CENTER, TEXT_COLOR);
}

void Hud::Initialize(const Camera& camera, const FighterManager& fighters, int slot, const Match& match, const SceneCalc& scene)
{
	m_Camera = &camera;
	m_Fighters = &fighters;
	m_Slot = slot;
	m_Match = &match;
	m_Scene = &scene;
	Reset();
}
