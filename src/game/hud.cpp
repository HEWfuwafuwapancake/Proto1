#include "game/hud.h"
#include "core/dx11.h"
#include "core/gametime.h"
#include "core/shader.h"
#include "game/camera.h"
#include "game/fightermanager.h"
#include "game/match.h"
#include "game/netsession.h"
#include "input/action.h"
#include "physics/scenecalc.h"
#include "render/cubeflipcalc.h"
#include "render/number2d.h"
#include "render/text2d.h"
#include <cmath>


//------------------------------------------------------------------------------
//	調整用の値（位置・大きさはピクセル。画面は SCREEN_WIDTH × SCREEN_HEIGHT）
//------------------------------------------------------------------------------
static constexpr XMFLOAT4 TEXT_COLOR = { 1.0f, 1.0f, 1.0f, 1.0f };		// 文字の色（白）
static constexpr XMFLOAT4 COVER_COLOR = { 0.0f, 0.0f, 0.0f, 0.6f };		// シーンのおおい（半透明の黒）

static constexpr float SECONDS_DIGIT_WIDTH = 40.0f;	// 残り秒数の桁の幅（高さはこの2倍）
static constexpr float SECONDS_Y = 20.0f;			// 残り秒数の上端（横は画面の真ん中）

static constexpr float SCORE_DIGIT_WIDTH = 28.0f;	// 左上のスコアの桁の幅
static constexpr float SCORE_X = 30.0f;				// 左上のスコアの左端
static constexpr float SCORE_Y = 56.0f;				// KOの文字の下に置く
static constexpr float HUD_LABEL_Y = 20.0f;
static constexpr float SELF_PERCENT_WIDTH = 22.0f;
static constexpr float SELF_PERCENT_BOTTOM = 140.0f;
static constexpr float SELF_PERCENT_X = 110.0f;
static constexpr float SELF_TAG_RIGHT = 75.0f;
static constexpr float PLAYER_TAG_WIDTH = 14.0f;
static constexpr float PLAYER_TAG_GAP = 4.0f;
static constexpr float PLAYER_TAG_LINE = 34.0f;
static constexpr float ARM_LABEL_MARGIN = 30.0f;
static constexpr float ARM_LABEL_WIDTH = 290.0f;
static constexpr float ARM_LABEL_HEIGHT = 38.0f;
static constexpr float ARM_LABEL_PADDING = 12.0f;
static constexpr float ARM_LABEL_BORDER = 2.0f;
static constexpr float ARM_HAND_DOT = 4.0f;
static constexpr float ARM_NAME_OFFSET = 46.0f;
static constexpr float ARM_FLIP_SECONDS = 0.2f;
static constexpr float ARM_EDGE_BRIGHTNESS = 0.65f;

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
static constexpr float TITLE_PLAYERS_Y = 200.0f;
static constexpr float TITLE_PLAYER_LINE = 48.0f;
static constexpr float TITLE_PLAYER_X = 440.0f;
static constexpr float TITLE_DEVICE_X = 540.0f;
static constexpr float TITLE_DEVICE_NUMBER_X = 640.0f;
static constexpr float TITLE_CHARACTER_X = 700.0f;
static constexpr float TITLE_JOIN_Y = 415.0f;
static constexpr float TITLE_START_Y = 455.0f;
static constexpr float TITLE_EQUIP_Y = 497.0f;
static constexpr float MODE_EQUIP_Y = 540.0f;
static constexpr char TITLE_EQUIP_HELP[] = "EQUIP  DPAD LEFT RIGHT HAND / UP DOWN ARM";
static constexpr float TITLE_MOVE_Y = 540.0f;
static constexpr float TITLE_ARMS_Y = 580.0f;
static constexpr float TITLE_JUMP_Y = 620.0f;
static constexpr float RESULT_DOT = 10.0f;			// 「RESULT」のドットの大きさ
static constexpr float RESULT_Y = 70.0f;
static constexpr float RESULT_HEADER_Y = 175.0f;
static constexpr float RESULT_ROWS_Y = 230.0f;
static constexpr float RESULT_ROW_GAP = 48.0f;
static constexpr float RESULT_RANK_X = 390.0f;
static constexpr float RESULT_PLAYER_X = 640.0f;
static constexpr float RESULT_KO_X = 890.0f;
static constexpr float RESULT_DIGIT_WIDTH = 16.0f;
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
static constexpr float MODE_TITLE_Y = 45.0f;
static constexpr float MODE_FIRST_Y = 160.0f;
static constexpr float MODE_LINE = 72.0f;
static constexpr float MENU_BACK_Y = 670.0f;
static constexpr float ADDRESS_Y = 280.0f;
static constexpr float ADDRESS_LEFT = 300.0f;
static constexpr float ADDRESS_PART_WIDTH = 170.0f;
static constexpr float ADDRESS_DOT = 7.0f;
static constexpr float ADDRESS_HELP_Y = 440.0f;
static constexpr float HOST_ADDRESS_Y = 105.0f;
static constexpr float HOST_ADDRESS_LINE = 42.0f;
static constexpr float HOST_ADDRESS_DOT = 5.0f;
static constexpr float HOST_PLAYER_Y = 275.0f;
static constexpr float HOST_PLAYER_LINE = 32.0f;

static constexpr float WARNING_X = 12.0f; // 全画面の通信警告の左端
static constexpr float WARNING_Y = 12.0f;
static constexpr float WARNING_DOT = 3.0f;
static constexpr float WARNING_LINE = 30.0f;
static constexpr float WARNING_PADDING = 6.0f;
static constexpr XMFLOAT4 WARNING_COLOR = { 1.0f, 0.2f, 0.2f, 1.0f };
static constexpr XMFLOAT4 WARNING_BACKGROUND = { 0.0f, 0.0f, 0.0f, 0.85f };

void Hud::DrawCornerWarning(const NetSession& net) const
{
	const std::string warning = net.GetCornerWarningText();
	float y = WARNING_Y;
	size_t begin = 0;
	while (begin < warning.size()) {
		const size_t end = warning.find('\n', begin);
		const std::string line = warning.substr(begin, end == std::string::npos ? end : end - begin);
		Number2DDrawBox({ WARNING_X - WARNING_PADDING, y - WARNING_PADDING,
			GlyphCalcTextWidth(line.c_str(), WARNING_DOT) + WARNING_PADDING * 2.0f,
			WARNING_DOT * 7.0f + WARNING_PADDING * 2.0f }, WARNING_BACKGROUND);
		Number2DDrawText(line.c_str(), WARNING_X, y, WARNING_DOT, GLYPH_ALIGN_LEFT, WARNING_COLOR);
		if (end == std::string::npos) break;
		begin = end + 1;
		y += WARNING_LINE;
	}
}


// 人の番号は7セグの数字とPを組み合わせる（文字の2〜4を新しく作らない）。
void Hud::DrawPlayerTag(int slot, float centerX, float y, float digitWidth) const
{
	const float dot = digitWidth * 2.0f / 7.0f;
	const float numberWidth = GlyphCalcNumberWidth(slot + 1, 1, digitWidth);
	const float width = numberWidth + PLAYER_TAG_GAP + GlyphCalcTextWidth("P", dot);
	const float left = centerX - width * 0.5f;
	const XMFLOAT4 color = Fighter::GetPlayerColor(slot);
	Number2DDrawNumber(slot + 1, 1, left, y, digitWidth, GLYPH_ALIGN_LEFT, color);
	Number2DDrawText("P", left + numberWidth + PLAYER_TAG_GAP, y, dot, GLYPH_ALIGN_LEFT, color);
}

void Hud::DrawPercent(float percent, float centerX, float y, float digitWidth, const XMFLOAT4& color) const
{
	const int value = GlyphCalcDisplayPercent(percent);
	const float dot = digitWidth * 2.0f / 7.0f;
	const float numberWidth = GlyphCalcNumberWidth(value, 1, digitWidth);
	const float width = numberWidth + PERCENT_SIGN_GAP + GlyphCalcTextWidth("%", dot);
	const float left = centerX - width * 0.5f;
	Number2DDrawNumber(value, 1, left, y, digitWidth, GLYPH_ALIGN_LEFT, color);
	Number2DDrawText("%", left + numberWidth + PERCENT_SIGN_GAP, y, dot, GLYPH_ALIGN_LEFT, color);
}

// 相手の頭の上に%を出す。人には1P〜4Pも表示する。
void Hud::DrawFighterPercent(int index) const
{
	if (index == m_Slot || !m_Fighters->IsActive(index) || m_Fighters->IsHidden(index)) return;

	// 頭のてっぺん（縦のカプセルの上の端 ＋ 半径）の少し上
	const CAPSULE body = m_Fighters->GetTarget(index, m_Slot).Body;
	const float top = (body.Start.Y > body.End.Y ? body.Start.Y : body.End.Y) + body.Radius;
	const XMFLOAT3 head = { body.Start.X, top + PERCENT_HEAD_OFFSET, body.Start.Z };
	XMFLOAT2 screen;
	if (!m_Camera->WorldToScreen(head, &screen)) return;
	screen.x /= m_Scale;
	screen.y /= m_Scale;

	const float y = screen.y - PERCENT_DIGIT_WIDTH * 2.0f;
	DrawPercent(m_Fighters->GetPercent(index), screen.x, y, PERCENT_DIGIT_WIDTH, TEXT_COLOR);
	if (m_Fighters->GetFighter(index).IsHuman()) {
		DrawPlayerTag(index, screen.x, y - PLAYER_TAG_LINE, PLAYER_TAG_WIDTH);
	}
}


void Hud::Reset(void)
{
	for (int hand = 0; hand < 2; ++hand) {
		const ARM_TYPE type = m_Arms->GetType(hand);
		m_ArmFlips[hand] = { type, type, ARM_FLIP_SECONDS, 1 };
	}
}

void Hud::Update(void)
{
	const float dt = GameTimeGetDeltaTime();
	for (int hand = 0; hand < 2; ++hand) {
		ARM_FLIP& flip = m_ArmFlips[hand];
		const ARM_TYPE type = m_Arms->GetType(hand);
		if (type != flip.Current) {
			// 途中でも、最後に装備していた腕から次の回転を始める。
			flip = { flip.Current, type, 0.0f, m_Arms->GetChangeDirection(hand) };
		}
		else {
			flip.Elapsed = fminf(ARM_FLIP_SECONDS, flip.Elapsed + dt);
		}
	}
}

void Hud::DrawArmName(int hand) const
{
	const bool selected = m_Arms->GetSelectedHand() == hand;
	const float x = hand == 0 ? ARM_LABEL_MARGIN : m_Width - ARM_LABEL_MARGIN - ARM_LABEL_WIDTH;
	const float y = m_Height - ARM_LABEL_MARGIN - ARM_LABEL_PADDING - ARM_LABEL_HEIGHT;
	const XMFLOAT4 color = selected ? MENU_SELECTED_COLOR : TEXT_COLOR;
	if (selected) {
		const float top = y - ARM_LABEL_PADDING;
		const float height = ARM_LABEL_HEIGHT + ARM_LABEL_PADDING * 2.0f;
		Number2DDrawBox({ x, top, ARM_LABEL_WIDTH, ARM_LABEL_BORDER }, MENU_HOVER_COLOR);
		Number2DDrawBox({ x, top + height - ARM_LABEL_BORDER, ARM_LABEL_WIDTH, ARM_LABEL_BORDER }, MENU_HOVER_COLOR);
		Number2DDrawBox({ x, top + ARM_LABEL_BORDER, ARM_LABEL_BORDER, height - ARM_LABEL_BORDER * 2.0f }, MENU_HOVER_COLOR);
		Number2DDrawBox({ x + ARM_LABEL_WIDTH - ARM_LABEL_BORDER, top + ARM_LABEL_BORDER,
			ARM_LABEL_BORDER, height - ARM_LABEL_BORDER * 2.0f }, MENU_HOVER_COLOR);
	}

	Number2DDrawText(hand == 0 ? "L" : "R", x + ARM_LABEL_PADDING, y + (ARM_LABEL_HEIGHT - ARM_HAND_DOT * 7.0f) * 0.5f,
		ARM_HAND_DOT, GLYPH_ALIGN_LEFT, color);
	const ARM_FLIP& flip = m_ArmFlips[hand];
	const CUBE_FLIP faces = CubeFlipCalc(flip.Elapsed / ARM_FLIP_SECONDS, ARM_LABEL_HEIGHT, flip.Direction);
	const auto drawFace = [&](ARM_TYPE type, const CUBE_FLIP_FACE& face) {
		const float brightness = ARM_EDGE_BRIGHTNESS + (1.0f - ARM_EDGE_BRIGHTNESS) * face.Facing;
		Text2DDraw(m_ArmNames[type], x + ARM_NAME_OFFSET, y + face.Y, face.Height, GLYPH_ALIGN_LEFT,
			{ color.x * brightness, color.y * brightness, color.z * brightness, color.w }, ARM_LABEL_HEIGHT);
	};
	drawFace(flip.Previous, faces.Previous);
	drawFace(flip.Current, faces.Next);
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
	Number2DDrawText("1P SELECT  STICK / W S", centerX, selectY, MENU_HELP_DOT, GLYPH_ALIGN_CENTER, TEXT_COLOR);
	Number2DDrawText("1P OK  O / ENTER", centerX, decideY, MENU_HELP_DOT, GLYPH_ALIGN_CENTER, TEXT_COLOR);
}

void Hud::SetViewSize(float width, float height)
{
	m_Scale = fminf(width / (float)SCREEN_WIDTH, height / (float)SCREEN_HEIGHT);
	m_Width = width / m_Scale;
	m_Height = height / m_Scale;
}

void Hud::DrawScene(const VersusCalc& participants, const NetSession& net, const SessionAddressCalc& address,
	const std::array<CHARACTER_TYPE, FIGHTER_COUNT>& characters) const
{
	const float centerX = (float)SCREEN_WIDTH * 0.5f;
	const SCENE scene = m_Scene->GetScene();
	if (scene == SCENE_GAME_START || scene == SCENE_CHARACTER_SELECT
		|| scene == SCENE_MODE_SELECT || scene == SCENE_JOIN_ADDRESS) {
		Number2DDrawBox({ 0.0f, 0.0f, (float)SCREEN_WIDTH, (float)SCREEN_HEIGHT }, COVER_COLOR);
		if (scene == SCENE_GAME_START) {
			Number2DDrawText("PROTO", centerX, TITLE_Y, TITLE_DOT, GLYPH_ALIGN_CENTER, TEXT_COLOR);
			DrawMenuItem("GAME START", ADDRESS_Y, true);
			Number2DDrawText("O / ENTER", centerX, ADDRESS_HELP_Y, MENU_HELP_DOT, GLYPH_ALIGN_CENTER, TEXT_COLOR);
			return;
		}
		if (scene == SCENE_CHARACTER_SELECT) {
			Number2DDrawText("CHARACTER SELECT", centerX, MODE_TITLE_Y, MENU_DOT, GLYPH_ALIGN_CENTER, TEXT_COLOR);
			DrawMenuItem(CHARACTER_DATA::Get(m_Scene->GetCharacter()).Name, ADDRESS_Y, true);
			Number2DDrawText("SELECT  LEFT RIGHT / A D / DPAD", centerX, TITLE_ARMS_Y, MENU_HELP_DOT, GLYPH_ALIGN_CENTER, TEXT_COLOR);
			Number2DDrawText("OK  O / ENTER", centerX, TITLE_JUMP_Y, MENU_HELP_DOT, GLYPH_ALIGN_CENTER, TEXT_COLOR);
		}
		else if (scene == SCENE_MODE_SELECT) {
			Number2DDrawText("MODE SELECT", centerX, MODE_TITLE_Y, MENU_DOT, GLYPH_ALIGN_CENTER, TEXT_COLOR);
			const char* items[MODE_ITEM_COUNT] = { "SOLO", "LOCAL VS", "ONLINE HOST", "ONLINE JOIN", "BACK" };
			for (int item = 0; item < MODE_ITEM_COUNT; ++item) {
				DrawMenuItem(items[item], MODE_FIRST_Y + item * MODE_LINE, m_Scene->GetCursor() == item);
			}
			// 下の「BACK」と重ならないよう、タイトルの説明と同じ高さに置く。
			Number2DDrawText(TITLE_EQUIP_HELP, centerX, MODE_EQUIP_Y, MENU_HELP_DOT, GLYPH_ALIGN_CENTER, TEXT_COLOR);
			Number2DDrawText("SELECT  UP DOWN / W S / DPAD", centerX, TITLE_ARMS_Y, MENU_HELP_DOT, GLYPH_ALIGN_CENTER, TEXT_COLOR);
			Number2DDrawText("OK  O / ENTER", centerX, TITLE_JUMP_Y, MENU_HELP_DOT, GLYPH_ALIGN_CENTER, TEXT_COLOR);
		}
		else {
			Number2DDrawText("ONLINE JOIN", centerX, MODE_TITLE_Y, MENU_DOT, GLYPH_ALIGN_CENTER, TEXT_COLOR);
			Number2DDrawText("HOST IP", centerX, MODE_FIRST_Y, MENU_HELP_DOT, GLYPH_ALIGN_CENTER, TEXT_COLOR);
			for (int part = 0; part < 4; ++part) {
				const float x = ADDRESS_LEFT + part * ADDRESS_PART_WIDTH;
				const XMFLOAT4 color = part == address.GetCursor() ? MENU_SELECTED_COLOR : TEXT_COLOR;
				if (part == address.GetCursor()) {
					Number2DDrawBox({ x - MENU_HOVER_MARGIN_X, ADDRESS_Y - MENU_HOVER_MARGIN_Y,
						ADDRESS_PART_WIDTH - MENU_HOVER_MARGIN_X, ADDRESS_DOT * 7.0f + MENU_HOVER_MARGIN_Y * 2.0f }, MENU_HOVER_COLOR);
				}
				Number2DDrawText(address.GetPart(part).c_str(), x, ADDRESS_Y, ADDRESS_DOT, GLYPH_ALIGN_LEFT, color);
				if (part < 3) Number2DDrawText(".", x + ADDRESS_PART_WIDTH - MENU_HOVER_MARGIN_X,
					ADDRESS_Y, ADDRESS_DOT, GLYPH_ALIGN_LEFT, TEXT_COLOR);
			}
			Number2DDrawText("KEY  0 1 2 3 4 5 6 7 8 9 .", centerX, ADDRESS_HELP_Y, MENU_HELP_DOT, GLYPH_ALIGN_CENTER, TEXT_COLOR);
			Number2DDrawText("DPAD  LEFT RIGHT / UP DOWN", centerX, ADDRESS_HELP_Y + TITLE_PLAYER_LINE,
				MENU_HELP_DOT, GLYPH_ALIGN_CENTER, TEXT_COLOR);
			Number2DDrawText("BACKSPACE  DELETE / EMPTY BACK", centerX, TITLE_MOVE_Y, MENU_HELP_DOT, GLYPH_ALIGN_CENTER, TEXT_COLOR);
			Number2DDrawText(address.IsValid() ? "JOIN  O / ENTER" : "ENTER IP  0 TO 255", centerX, RESULT_SELECT_Y,
				MENU_HELP_DOT, GLYPH_ALIGN_CENTER, address.IsValid() ? MENU_SELECTED_COLOR : MENU_TEXT_COLOR);
		}
		Number2DDrawText(scene == SCENE_JOIN_ADDRESS ? "BACK  X" : "BACK  X / BACKSPACE",
			centerX, MENU_BACK_Y, MENU_HELP_DOT, GLYPH_ALIGN_CENTER, TEXT_COLOR);
		return;
	}

	if (m_Scene->GetScene() == SCENE_TITLE) {
		Number2DDrawBox({ 0.0f, 0.0f, (float)SCREEN_WIDTH, (float)SCREEN_HEIGHT }, COVER_COLOR);
		Number2DDrawText(net.IsOnline() ? (net.IsHost() ? "ONLINE HOST" : "ONLINE JOIN") : "LOCAL VS",
			centerX, MODE_TITLE_Y, MENU_DOT, GLYPH_ALIGN_CENTER, TEXT_COLOR);
		if (net.IsHost()) {
			// 複数の IPv4 は2列で大きく出す。LAN・仮想アダプターのどれを使うか選べる。
			const std::string& addresses = net.GetAddresses();
			size_t begin = 0;
			int index = 0;
			while (begin < addresses.size()) {
				const size_t end = addresses.find(" / ", begin);
				const std::string ip = addresses.substr(begin, end == std::string::npos ? end : end - begin);
				Number2DDrawText(ip.c_str(), (float)SCREEN_WIDTH * (index % 2 == 0 ? 0.25f : 0.75f),
					HOST_ADDRESS_Y + (index / 2) * HOST_ADDRESS_LINE, HOST_ADDRESS_DOT, GLYPH_ALIGN_CENTER, MENU_SELECTED_COLOR);
				++index;
				if (end == std::string::npos) break;
				begin = end + 3;
			}
		}
		for (int slot = 0; slot < FIGHTER_COUNT; ++slot) {
			const float y = net.IsHost() ? HOST_PLAYER_Y + slot * HOST_PLAYER_LINE : TITLE_PLAYERS_Y + slot * TITLE_PLAYER_LINE;
			DrawPlayerTag(slot, TITLE_PLAYER_X, y, PLAYER_TAG_WIDTH);
			const int device = participants.GetDevice(slot);
			const XMFLOAT4 color = participants.IsHuman(slot) ? Fighter::GetPlayerColor(slot) : MENU_TEXT_COLOR;
			const char* label = !participants.IsHuman(slot) ? "WAIT"
				: (device == VERSUS_DEVICE_REMOTE ? "NET" : (device == ACTION_DEVICE_KEYBOARD ? "KEY" : "PAD"));
			Number2DDrawText(label, TITLE_DEVICE_X, y, MENU_HELP_DOT, GLYPH_ALIGN_LEFT, color);
			if (device >= 0 && device < ACTION_DEVICE_KEYBOARD) {
				Number2DDrawNumber(device + 1, 1, TITLE_DEVICE_NUMBER_X, y, PLAYER_TAG_WIDTH, GLYPH_ALIGN_LEFT, color);
			}
			if (participants.IsHuman(slot) || (net.IsOnline() && (net.GetMask() & (1u << slot)))) {
				Number2DDrawText(CHARACTER_DATA::Get(characters[slot]).Name, TITLE_CHARACTER_X, y,
					MENU_HELP_DOT, GLYPH_ALIGN_LEFT, color);
			}
		}
		Number2DDrawText("JOIN  O / ENTER", centerX, TITLE_JOIN_Y, MENU_HELP_DOT, GLYPH_ALIGN_CENTER, TEXT_COLOR);
		if (participants.GetHumanCount() > 0) {
			Number2DDrawText("1P START  O / ENTER", centerX, TITLE_START_Y, MENU_HELP_DOT, GLYPH_ALIGN_CENTER, TEXT_COLOR);
		}
		Number2DDrawText(TITLE_EQUIP_HELP, centerX, TITLE_EQUIP_Y, MENU_HELP_DOT, GLYPH_ALIGN_CENTER, TEXT_COLOR);
		Number2DDrawText(net.IsOnline() ? "MOVE  WASD / L STICK" : "CHARACTER  A D / DPAD LEFT RIGHT",
			centerX, TITLE_MOVE_Y, MENU_HELP_DOT, GLYPH_ALIGN_CENTER, TEXT_COLOR);
		Number2DDrawText("ARMS  L1 R1 / CLICK", centerX, TITLE_ARMS_Y, MENU_HELP_DOT, GLYPH_ALIGN_CENTER, TEXT_COLOR);
		Number2DDrawText("JUMP  SPACE / X", centerX, TITLE_JUMP_Y, MENU_HELP_DOT, GLYPH_ALIGN_CENTER, TEXT_COLOR);
		Number2DDrawText("BACK  X / BACKSPACE", centerX, MENU_BACK_Y, MENU_HELP_DOT, GLYPH_ALIGN_CENTER, TEXT_COLOR);
		return;
	}
	if (m_Scene->GetScene() == SCENE_RESULT) {
		Number2DDrawBox({ 0.0f, 0.0f, (float)SCREEN_WIDTH, (float)SCREEN_HEIGHT }, COVER_COLOR);
		Number2DDrawText("RESULT", centerX, RESULT_Y, RESULT_DOT, GLYPH_ALIGN_CENTER, TEXT_COLOR);
		Number2DDrawText("RANK", RESULT_RANK_X, RESULT_HEADER_Y, MENU_HELP_DOT, GLYPH_ALIGN_CENTER, TEXT_COLOR);
		Number2DDrawText("PLAYER", RESULT_PLAYER_X, RESULT_HEADER_Y, MENU_HELP_DOT, GLYPH_ALIGN_CENTER, TEXT_COLOR);
		Number2DDrawText("KO", RESULT_KO_X, RESULT_HEADER_Y, MENU_HELP_DOT, GLYPH_ALIGN_CENTER, TEXT_COLOR);
		const VersusCalc& versus = m_Fighters->GetVersus();
		for (int order = 0; order < versus.GetHumanCount(); ++order) {
			const int slot = versus.GetRankedSlot(order);
			const float y = RESULT_ROWS_Y + order * RESULT_ROW_GAP;
			const XMFLOAT4 color = Fighter::GetPlayerColor(slot);
			Number2DDrawNumber(versus.GetRank(slot), 1, RESULT_RANK_X, y, RESULT_DIGIT_WIDTH, GLYPH_ALIGN_CENTER, color);
			DrawPlayerTag(slot, RESULT_PLAYER_X, y, RESULT_DIGIT_WIDTH);
			Number2DDrawNumber(versus.GetScore(slot), 1, RESULT_KO_X, y, RESULT_DIGIT_WIDTH, GLYPH_ALIGN_CENTER, color);
		}
		DrawMenuItem("RETRY", RESULT_MENU_Y, m_Scene->GetCursor() == RESULT_ITEM_RETRY);
		DrawMenuItem("TITLE", RESULT_MENU_Y + MENU_LINE_GAP, m_Scene->GetCursor() == RESULT_ITEM_TITLE);
		DrawMenuHelp(RESULT_SELECT_Y, RESULT_DECIDE_Y);
	}
}

void Hud::Draw(void) const
{
	if (m_Scene->GetScene() != SCENE_MATCH) return;
	// 自分の画面に 1280×720 の座標を縦横同じ倍率で当てはめ、描き終わったら画面全体に戻す。
	Shader_Set2DSize(m_Width, m_Height);
	const float centerX = m_Width * 0.5f;
	const int score = m_Fighters->GetScore(m_Slot);

	for (int i = 0; i < FIGHTER_COUNT; i++) {
		DrawFighterPercent(i);
	}

	if (m_Match->GetState() == MATCH_STATE_FINISHED) {
		// 時間切れ：おおいの上に TIME UP・自分の KO 数・結果への案内（左上の KO 数は出さない）
		Number2DDrawBox({ 0.0f, 0.0f, m_Width, m_Height }, COVER_COLOR);
		Number2DDrawText("TIME UP", centerX, TIME_UP_Y, TIME_UP_DOT, GLYPH_ALIGN_CENTER, TEXT_COLOR);
		Number2DDrawNumber(score, 1, centerX, FINAL_SCORE_Y, FINAL_SCORE_DIGIT_WIDTH, GLYPH_ALIGN_CENTER, TEXT_COLOR);
		Number2DDrawText("1P NEXT  O / ENTER", centerX, NEXT_Y, NEXT_DOT, GLYPH_ALIGN_CENTER, TEXT_COLOR);
	}
	else {
		Number2DDrawText("KO", SCORE_X, HUD_LABEL_Y, MENU_HELP_DOT, GLYPH_ALIGN_LEFT, TEXT_COLOR);
		Number2DDrawNumber(score, 1, SCORE_X, SCORE_Y, SCORE_DIGIT_WIDTH, GLYPH_ALIGN_LEFT, TEXT_COLOR);
	}
	DrawPlayerTag(m_Slot, m_Width - SELF_TAG_RIGHT, HUD_LABEL_Y, SELF_PERCENT_WIDTH);
	DrawPercent(m_Fighters->GetPercent(m_Slot), SELF_PERCENT_X, m_Height - SELF_PERCENT_BOTTOM,
		SELF_PERCENT_WIDTH, Fighter::GetPlayerColor(m_Slot));

	DrawArmName(0);
	DrawArmName(1);

	// 残り秒数（時間切れでも 0 のまま、おおいの上に出す）
	Number2DDrawNumber(m_Match->GetRemainingSeconds(), 1, centerX, SECONDS_Y, SECONDS_DIGIT_WIDTH, GLYPH_ALIGN_CENTER, TEXT_COLOR);
	Shader_Set2DSize((float)SCREEN_WIDTH, (float)SCREEN_HEIGHT);
}

void Hud::Initialize(const Camera& camera, const FighterManager& fighters, int slot, const Match& match, const SceneCalc& scene, const ArmManager& arms)
{
	m_Camera = &camera;
	m_Fighters = &fighters;
	m_Slot = slot;
	m_Match = &match;
	m_Scene = &scene;
	m_Arms = &arms;
	for (int type = 0; type < ARM_TYPE_COUNT; ++type) {
		m_ArmNames[type] = Text2DCreate(ArmGetName(static_cast<ARM_TYPE>(type)));
	}
	Reset();
}
