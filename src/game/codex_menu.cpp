#include "game/codex_menu.h"
#include "game/netsession.h"
#include "game/character.h"
#include "game/fighter.h"
#include "core/shader.h"
#include "input/action.h"
#include "render/backdrop.h"
#include "render/menucalc.h"
#include "render/number2d.h"
#include "render/quadcalc.h"
#include "render/text2d.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

// 調整用の値（1280×720 の共通座標。色はピンク・黄緑・紫と、ふち・影の濃い紫）
static constexpr XMFLOAT4 PINK{ 1.0f, 0.24f, 0.65f, 1.0f };
static constexpr XMFLOAT4 LIME{ 0.78f, 0.96f, 0.20f, 1.0f };
static constexpr XMFLOAT4 PURPLE{ 0.36f, 0.17f, 0.85f, 1.0f };
static constexpr XMFLOAT4 INK{ 0.10f, 0.04f, 0.18f, 1.0f };
static constexpr XMFLOAT4 WHITE{ 1.0f, 1.0f, 1.0f, 1.0f };
static constexpr XMFLOAT4 MUTED{ 0.42f, 0.34f, 0.52f, 1.0f }; // 白い地の上の薄い文字
static constexpr XMFLOAT4 BACKDROP_TINT{ 1.0f, 0.24f, 0.65f, 0.35f }; // ぼかした景色に重ねるピンク（濃いほど景色が見えにくい）
static constexpr float PI = 3.14159265f;
static constexpr float SLANT = 0.25f; // 斜めの帯の傾き（高さ1あたり上の辺が右へずれる量。約14度）
static constexpr float TEXT_SCALE = 1.35f; // 文字画像の上下の余白を見込み、文字を大きくする
static constexpr float POP_SECONDS = 0.22f; // 選んだボタンが弾む時間
static constexpr float POP_WIDTH = 60.0f; // 選んだボタンが右へ伸びる長さ
static constexpr float JUT_WIDTH = 10.0f; // 選んだボタンが右へ突き出す揺れの幅
static constexpr float JUT_SPEED = 4.5f; // 突き出す揺れの速さ
static constexpr float ORBIT_SECONDS = 70.0f; // 背景のカメラがステージを1周する秒数
static constexpr float ORBIT_RADIUS_SCALE = 1.7f; // 背景のカメラの水平の距離（浮島の大きい方の半分の何倍か）
static constexpr float ORBIT_HEIGHT_SCALE = 0.8f; // 背景のカメラの高さ（水平の距離の何倍か）
static constexpr float MARQUEE_SPEED = 70.0f; // タイトルの流れる文字の速さ（ピクセル/秒）
static constexpr float LOGO_STEP_SECONDS = 0.35f; // タイトルのロゴがカクッと揺れる間隔
static constexpr float LOGO_ANGLES[] = { -3.0f, -1.5f, -4.0f, -2.0f }; // ロゴの揺れの角度（度）
static constexpr float SHOWCASE_SECONDS = 4.0f; // タイトルのキャラを入れ替える間隔
static constexpr float PREVIEW_TURN_SECONDS = 12.0f; // キャラ選択で全身を見せる1回転の秒数
static constexpr float PREVIEW_BODY_HEIGHT = 2.0f; // キャラ共通の表示上の高さ
static constexpr float PREVIEW_VIEW_HEIGHT = 2.8f; // 上下と回転中の張り出しの余白を含む
static constexpr float FOOTER_Y = 670.0f; // 下の操作説明の帯
static constexpr float CARD_TURN = -3.0f * PI / 180.0f; // 右の説明のカードの傾き
static constexpr RECT2D INFO_CARD{ 760, 138, 450, 310 }; // 右の説明のカード
static constexpr float OPTION_TOP = 118.0f;
static constexpr float OPTION_GAP = 45.0f; // 11行が下の操作説明に重ならない間隔
static constexpr float OPTION_HEIGHT = 38.0f;
static constexpr float OPTION_VALUE_X = 456.0f; // 設定の値を書く位置
static constexpr float LOBBY_CARD_LEFT = 40.0f; // キャラ選択の 1P〜4P のカード
static constexpr float LOBBY_CARD_TOP = 118.0f;
static constexpr float LOBBY_CARD_WIDTH = 282.0f;
static constexpr float LOBBY_CARD_HEIGHT = 440.0f;
static constexpr float LOBBY_CARD_GAP = 14.0f;
static constexpr float LOBBY_STATUS_Y = 572.0f; // カードの下の「準備OK」などの行
static constexpr float LOBBY_BANNER_Y = 612.0f; // 「READY TO FIGHT」の帯
static constexpr float RESULT_PANEL_LEFT = 40.0f; // 結果の順位のパネル
static constexpr float RESULT_PANEL_TOP = 244.0f;
static constexpr float RESULT_PANEL_WIDTH = 205.0f;
static constexpr float RESULT_PANEL_HEIGHT = 384.0f;
static constexpr float RESULT_PANEL_GAP = 16.0f;
static constexpr float RESULT_WINNER_RISE = 28.0f; // 1位のパネルだけ上へ伸ばす高さ
static constexpr float RESULT_BUTTON_X = 950.0f; // 結果のメニューのボタン
static constexpr float RESULT_BUTTON_WIDTH = 230.0f;
static constexpr float RESULT_BUTTON_TOP = 300.0f;
static constexpr float RESULT_BUTTON_GAP = 78.0f;
static constexpr float COST_LEFT = 40.0f; // 耐久の減る量の表の左端
static constexpr float COST_TOP = 178.0f; // 表の1行目
static constexpr float COST_GAP = 49.0f; // 表の行の間隔
static constexpr float COST_LABEL_WIDTH = 170.0f; // 表の腕の名前の列の幅
static constexpr float COST_CELL_WIDTH = 86.0f; // 表の数字の欄の幅
static constexpr float COST_ROW_WIDTH = 610.0f; // 表の行の帯の幅
static constexpr float COST_SCROLL_WIDTH = 6.0f; // スクロールの位置の棒の幅
static constexpr const char* MARQUEE_TEXT = "つかむ！ なげる！ ぶっ飛ばす！";
static constexpr const char* OPTION_NAMES[] = {
	"突進・移動技の距離", "吹っ飛びの強さ", "ハンマーのしびれ時間", "ダメージの倍率",
	"マップの広さ", "カプセルの出現量", "腕の耐久の持ち"
};
static constexpr const char* OPTION_HELP[][3] = {
	{ "長押しで使う移動技の距離を変えます。", "大きくすると、遠くまで踏み込めます。", "場外からの復帰にも影響します。" },
	{ "攻撃が当たったときの飛ぶ強さです。", "大きくすると、場外へ飛ばしやすくなります。", "ダメージの増え方は変わりません。" },
	{ "ハンマーの長押しでしびれる時間です。", "0 秒なら、しびれは発生しません。", "動きを止める長さを調整できます。" },
	{ "攻撃で増えるダメージ％の倍率です。", "大きくすると、短時間で％が上がります。", "腕ごとの基本ダメージに掛かります。" },
	{ "ステージ全体の広さを変えます。", "広いと移動に余裕が生まれます。", "狭いと相手と出会いやすくなります。" },
	{ "最初の個数と、追加される間隔です。", "多くすると、腕を拾いやすくなります。", "GAME ルールで使う設定です。" },
	{ "腕が壊れるまでの持ちを変えます。", "倍率が大きいほど、長く使えます。", "無制限なら、耐久が減りません。" }
};
static constexpr const char* COST_COLUMNS[ARM_COST_ITEM_COUNT] = { "弱・当たり", "弱・外れ", "強・当たり", "強・外れ", "そのほか" };
static constexpr const char* COST_ROWS[ARM_COST_ROW_COUNT] = { "触手", "ハンマー", "ナックル", "レーザー", "刀", "モーニングスター", "槍", "ミサイル", "ガード" };
static constexpr const char* COST_NOTES[ARM_COST_ROW_COUNT][2] = {
	{ "弱：つかんだ・足場に付いた", "強：回転" }, { "強：衝撃波", "そのほか：空中の跳ね" }, { "強：突進（空中も同じ）", "" },
	{ "弱：単発のレーザー銃", "強：溜めきった砲（貫通）" },
	{ "強・外れ：飛んだ瞬間に減る", "当たれば強・当たりとの差を足す" }, { "弱：扇　強：投げた鉄球", "回している間は減らない" },
	{ "弱：突き（直線上の全員）", "強：投げた槍が刺さったか" },
	{ "1発ごとに16（持ちで割る）", "表では変えられない" },
	{ "パリィできずに防いだとき", "" }
};
static constexpr const char* CPU_LEVEL_NAMES[VERSUS_CPU_LEVEL_COUNT] = { "ダミー", "CPU よわい", "CPU ふつう", "CPU つよい" };
static constexpr const char* MODE_NAMES[] = {
	"ひとりで遊ぶ", "この PC で対戦", "オンラインを開催", "オンラインに参加", "オプション", "タイトルへ戻る"
};
static constexpr const char* MODE_SUBTITLES[] = {
	"CPU・ダミーと", "最大 4 人", "参加者を待つ", "IP を入力", "遊び心地", "最初の画面"
};
static constexpr const char* MODE_HELP[][3] = {
	{ "決定した機器を 1P にして遊びます。", "空いている枠は CPU（強さ3段）かダミー。", "キャラ選択で 1P が選べます。" },
	{ "キャラ選択でパッドやキーボードから参加。", "人数に合わせて画面を分割します。", "全員が準備OKなら 1P の決定で開始。" },
	{ "この PC をホストにして待機します。", "表示された IP を相手に伝えます。", "離れた相手とは Tailscale で接続。" },
	{ "ホストの IPv4 アドレスを入力します。", "接続後はロビーで参加を待ちます。", "ホストと同じ版のゲームを使います。" },
	{ "試合の遊び心地を調整できます。", "変更は次の試合から適用されます。", "オンラインはホストの設定を使用。" },
	{ "対戦方法は選び直せます。", "オプションは起動中そのまま保持。", "終了は Esc キーから行えます。" }
};

/// @brief 文字や四角を回す中心と角度（Angle が 0 なら回さない）
struct MENU_TURN
{
	float X; ///< 回す中心の横の位置
	float Y; ///< 回す中心の縦の位置
	float Angle; ///< 回す角度（ラジアン。プラスで画面の時計回り）
};
static constexpr MENU_TURN NO_TURN{ 0.0f, 0.0f, 0.0f };

static QUAD2D Turn(const QUAD2D& quad, const MENU_TURN& turn)
{
	return turn.Angle == 0.0f ? quad : QuadCalcRotate(quad, turn.X, turn.Y, turn.Angle);
}

static QUAD2D Move(QUAD2D quad, float dx, float dy)
{
	for (int i = 0; i < 4; ++i) {
		quad.X[i] += dx;
		quad.Y[i] += dy;
	}
	return quad;
}

static RECT2D Grow(const RECT2D& rect, float size)
{
	return { rect.X - size, rect.Y - size, rect.Width + size * 2.0f, rect.Height + size * 2.0f };
}

// 文字の横幅（height は Word・Shout に渡す高さ）。
static float TextWidth(const char* text, float height, TEXT2D_FONT font)
{
	return Text2DGetAspect(Text2DCreate(text, font)) * height * TEXT_SCALE;
}

static QUAD2D TextQuad(int id, float x, float y, float height, GLYPH_ALIGN align, const MENU_TURN& turn)
{
	const float drawHeight = height * TEXT_SCALE;
	const float width = Text2DGetAspect(id) * drawHeight;
	if (align == GLYPH_ALIGN_CENTER) x -= width * 0.5f;
	if (align == GLYPH_ALIGN_RIGHT) x -= width;
	return Turn(QuadCalcRect({ x, y, width, drawHeight }), turn);
}

// ふちのない文字。
static void Word(const char* text, float x, float y, float height, const XMFLOAT4& color,
	TEXT2D_FONT font = TEXT2D_FONT_BOLD, GLYPH_ALIGN align = GLYPH_ALIGN_LEFT, const MENU_TURN& turn = NO_TURN)
{
	const int id = Text2DCreate(text, font);
	Text2DDrawQuad(id, TextQuad(id, x, y, height, align, turn), color);
}

// 太いふちとずらした影のついた、ポップな見出しの文字。
static void Shout(const char* text, float x, float y, float height, const XMFLOAT4& fill, float outline, float shadow,
	GLYPH_ALIGN align = GLYPH_ALIGN_LEFT, const MENU_TURN& turn = NO_TURN)
{
	const int id = Text2DCreate(text, TEXT2D_FONT_POP);
	const QUAD2D quad = TextQuad(id, x, y, height, align, turn);
	if (shadow > 0.0f) {
		// 影も太くするため、影の位置でもふちと同じだけ広げて描く。
		for (int i = 0; i < 8; ++i) {
			const float angle = i * PI / 4.0f;
			Text2DDrawQuad(id, Move(quad, shadow + cosf(angle) * outline, shadow + sinf(angle) * outline), INK);
		}
	}
	for (int i = 0; i < 8; ++i) {
		const float angle = i * PI / 4.0f;
		Text2DDrawQuad(id, Move(quad, cosf(angle) * outline, sinf(angle) * outline), INK);
	}
	Text2DDrawQuad(id, quad, fill);
}

// 斜めの帯（ふち・ずらした影つき）。slant は高さ1あたりの傾き（0 なら傾けない）。
static void Bar(const RECT2D& rect, float slant, const XMFLOAT4& fill, float border, float shadow,
	const XMFLOAT4& shadowColor = INK, const MENU_TURN& turn = NO_TURN)
{
	if (shadow > 0.0f) {
		const RECT2D under = Grow({ rect.X + shadow, rect.Y + shadow, rect.Width, rect.Height }, border);
		Number2DDrawQuad(Turn(QuadCalcSkew(under, under.Height * slant), turn), shadowColor);
	}
	if (border > 0.0f) {
		const RECT2D edge = Grow(rect, border);
		Number2DDrawQuad(Turn(QuadCalcSkew(edge, edge.Height * slant), turn), INK);
	}
	Number2DDrawQuad(Turn(QuadCalcSkew(rect, rect.Height * slant), turn), fill);
}

// 表のスクロールの位置を縦の棒で見せる（全部の行が見えているときは出さない）。
static void ScrollBar(float x, float y, float height, int top, int shown, int count)
{
	if (shown <= 0 || count <= shown) return;
	const float thumbHeight = height * shown / count;
	const float thumbY = y + (height - thumbHeight) * top / (count - shown);
	Number2DDrawBox({ x, y, COST_SCROLL_WIDTH, height }, { INK.x, INK.y, INK.z, 0.35f });
	Number2DDrawBox({ x, thumbY, COST_SCROLL_WIDTH, thumbHeight }, LIME);
}

// 画面の端から端までの、傾いた帯（中心と太さで決める）。
static void Band(float centerX, float centerY, float thickness, float angle, const XMFLOAT4& color)
{
	const RECT2D rect{ centerX - 1100.0f, centerY - thickness * 0.5f, 2200.0f, thickness };
	Number2DDrawQuad(QuadCalcRotate(QuadCalcRect(rect), centerX, centerY, angle), color);
}

float CodexMenu::GetSeconds() const
{
	return std::chrono::duration<float>(std::chrono::steady_clock::now() - m_PreviewStart).count();
}

float CodexMenu::GetPop() const
{
	const float seconds = std::chrono::duration<float>(std::chrono::steady_clock::now() - m_ChangeTime).count();
	return MenuCalcEaseOutBack(seconds / POP_SECONDS);
}

void CodexMenu::Initialize()
{
	Finalize();
	for (int i = 0; i < CHARACTER_COUNT; ++i) {
		const auto& character = CHARACTER_DATA::Get(static_cast<CHARACTER_TYPE>(i));
		std::array<const char*, static_cast<size_t>(SKINNED_CLIP::Count)> paths{};
		// Load が必須とする待機・走り・左右腕だけ。攻撃の任意クリップは読まない。
		for (size_t clip = 0; clip < 4; ++clip) paths[clip] = character.Paths[clip];
		if (m_PreviewModels[i].Load(paths, character.Bones)) m_PreviewModels[i].Reset(m_PreviewPoses[i]);
	}
	m_PreviewStart = std::chrono::steady_clock::now();
	m_ChangeTime = m_PreviewStart;
	BackdropInitialize();
	// くり返し表示する説明は、最初に画像へ変えておく。Text2D が画像の寿命を管理する。
	for (const auto& lines : OPTION_HELP) for (const auto* text : lines) Text2DCreate(text, TEXT2D_FONT_BOLD);
	for (const auto* name : OPTION_NAMES) Text2DCreate(name, TEXT2D_FONT_BOLD);
	for (const auto* name : MODE_NAMES) Text2DCreate(name, TEXT2D_FONT_POP);
	for (const auto& lines : MODE_HELP) for (const auto* text : lines) Text2DCreate(text, TEXT2D_FONT_BOLD);
	Text2DCreate(MARQUEE_TEXT, TEXT2D_FONT_POP);
}

void CodexMenu::Finalize()
{
	for (auto& model : m_PreviewModels) model.Finalize();
	for (auto& pose : m_PreviewPoses) pose = {};
	BackdropFinalize();
}

void CodexMenu::Update(const SceneCalc& scene)
{
	if (scene.GetScene() == m_LastScene && scene.GetCursor() == m_LastCursor) return;
	m_LastScene = scene.GetScene();
	m_LastCursor = scene.GetCursor();
	m_ChangeTime = std::chrono::steady_clock::now();
}

void CodexMenu::SetBackdropCamera(const STAGE& stage) const
{
	const MENU_POINT center{ stage.CenterX, stage.TopY, stage.CenterZ };
	const float radius = (std::max)(stage.HalfWidth, stage.HalfDepth) * ORBIT_RADIUS_SCALE;
	const MENU_POINT eye = MenuCalcOrbitEye(GetSeconds(), center, radius, radius * ORBIT_HEIGHT_SCALE, ORBIT_SECONDS);
	Shader_SetViewMatrix(XMMatrixLookAtLH(XMVectorSet(eye.X, eye.Y, eye.Z, 1.0f),
		XMVectorSet(center.X, center.Y, center.Z, 1.0f), XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)));
	Shader_SetProjectionMatrix(XMMatrixPerspectiveFovLH(XMConvertToRadians(55.0f),
		static_cast<float>(BACKDROP_WIDTH) / static_cast<float>(BACKDROP_HEIGHT), 0.1f, radius * 6.0f));
}

void CodexMenu::DrawCharacterPreview(CHARACTER_TYPE character, const RECT2D& area, float yaw, float viewHeight) const
{
	const auto& model = m_PreviewModels[character];
	if (!model.IsLoaded()) {
		Word("モデルを読み込めません", area.X + 10, area.Y + area.Height * 0.5f, 15, MUTED);
		return;
	}
	const auto& data = CHARACTER_DATA::Get(character);
	const float scale = PREVIEW_BODY_HEIGHT / data.ModelHeight;
	const XMMATRIX world = XMMatrixScaling(scale, scale, scale)
		* XMMatrixRotationY(data.ModelYaw + yaw)
		* XMMatrixTranslation(0.0f, -data.ModelFootY * scale, 0.0f);
	// カード同士は重ならず、フレーム先頭の Clear で深度も消されている。
	SetViewport(area.X, area.Y, area.Width, area.Height);
	SetDepthEnable(true);
	Shader_SetViewMatrix(XMMatrixLookAtLH(XMVectorSet(0.0f, 1.3f, 4.5f, 1.0f),
		XMVectorSet(0.0f, 1.0f, 0.0f, 1.0f), XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)));
	Shader_SetProjectionMatrix(XMMatrixOrthographicLH(viewHeight * area.Width / area.Height, viewHeight, 0.1f, 20.0f));
	LIGHT light{};
	light.Enable = TRUE;
	light.Direction = { 0.5f, -1.0f, -0.8f, 0.0f };
	light.Diffuse = { 0.85f, 0.85f, 0.9f, 1.0f };
	light.Ambient = { 0.45f, 0.42f, 0.48f, 1.0f };
	Shader_SetLight(light);
	model.Draw(m_PreviewPoses[character], world, WHITE);
	// 後続のカード・文字・通信警告を、通常の2D描画へ戻す。
	SetViewport(0.0f, 0.0f, static_cast<float>(SCREEN_WIDTH), static_cast<float>(SCREEN_HEIGHT));
	SetDepthEnable(false);
	light.Enable = FALSE;
	Shader_SetLight(light);
	Shader_Begin();
	Shader_Set2DMatrix();
}

void CodexMenu::Background() const
{
	// ぼかした景色が作れなかったときもピンクの地になるよう、先に塗っておく。
	Number2DDrawBox({ 0, 0, 1280, 720 }, PINK);
	BackdropDraw(WHITE);
	Number2DDrawBox({ 0, 0, 1280, 720 }, BACKDROP_TINT);
}

void CodexMenu::Header(const char* title, const char* eyebrow) const
{
	const float titleWidth = TextWidth(title, 34, TEXT2D_FONT_POP);
	const float eyebrowWidth = TextWidth(eyebrow, 15, TEXT2D_FONT_BOLD);
	Bar({ -40, 22, 104 + titleWidth + 22 + eyebrowWidth + 40, 66 }, SLANT, INK, 0, 0);
	Word(title, 64, 28, 34, LIME, TEXT2D_FONT_POP);
	Word(eyebrow, 64 + titleWidth + 22, 52, 15, WHITE);
}

void CodexMenu::Footer(const char* help) const
{
	Number2DDrawBox({ 0, FOOTER_Y, 1280, 720 - FOOTER_Y }, INK);
	Number2DDrawBox({ 0, FOOTER_Y, 1280, 4 }, LIME);
	Word(help, 40, FOOTER_Y + 14, 16, WHITE);
	Word("Esc  終了  /  F11  全画面", 1240, FOOTER_Y + 16, 13, LIME, TEXT2D_FONT_BOLD, GLYPH_ALIGN_RIGHT);
}

void CodexMenu::Button(int index, const char* title, const char* subtitle, bool selected, bool dark,
	float x, float y, float width, float height) const
{
	// 選んだボタンは黄緑にして右へ伸ばし、弾んだあとも小さく突き出し続ける。
	const float jut = selected ? JUT_WIDTH * fabsf(sinf(GetSeconds() * JUT_SPEED)) : 0.0f;
	const float grow = selected ? POP_WIDTH * GetPop() : 0.0f;
	const XMFLOAT4 fill = selected ? LIME : dark ? PURPLE : WHITE;
	const XMFLOAT4 ink = dark && !selected ? WHITE : INK;
	Bar({ x + jut, y, width + grow, height }, SLANT, fill, 4, selected ? 9.0f : 5.0f);
	const float textX = x + jut + 46.0f;
	const float titleHeight = height * (subtitle[0] ? 0.42f : 0.46f);
	char number[8];
	sprintf_s(number, "%02d", index + 1);
	Word(number, textX - 4, y + height * 0.5f - 9, 15, dark && !selected ? LIME : PURPLE, TEXT2D_FONT_POP);
	Word(title, textX + 34, y + (height - titleHeight * TEXT_SCALE) * 0.5f, titleHeight, ink, TEXT2D_FONT_POP);
	if (subtitle[0]) {
		const float titleWidth = TextWidth(title, titleHeight, TEXT2D_FONT_POP);
		Word(subtitle, textX + 34 + titleWidth + 16, y + height * 0.5f - 8, 13, selected ? PURPLE : dark ? WHITE : MUTED);
	}
}

void CodexMenu::InfoCard(const RECT2D& area, const char* title, const char* line1, const char* line2, const char* line3) const
{
	// 白いカードを少し傾け、紫のずらした影と、上にテープを貼る。
	const MENU_TURN turn{ area.X + area.Width * 0.5f, area.Y + area.Height * 0.5f, CARD_TURN };
	Bar(area, 0.0f, WHITE, 5, 14, PURPLE, turn);
	Bar({ area.X + area.Width * 0.35f, area.Y - 18, 120, 34 }, 0.0f, { LIME.x, LIME.y, LIME.z, 0.85f }, 0, 0, INK,
		{ area.X + area.Width * 0.35f + 60, area.Y, 4.0f * PI / 180.0f });
	Word(title, area.X + 28, area.Y + 30, 30, INK, TEXT2D_FONT_POP, GLYPH_ALIGN_LEFT, turn);
	Bar({ area.X + 28, area.Y + 84, 70, 6 }, 0.0f, PINK, 0, 0, INK, turn);
	const char* lines[] = { line1, line2, line3 };
	for (int i = 0; i < 3; ++i) {
		// 空の行は描かない（耐久の表の説明は1行だけの腕がある）。
		if (lines[i][0]) Word(lines[i], area.X + 28, area.Y + 110 + i * 40.0f, 18, i == 0 ? INK : MUTED, TEXT2D_FONT_BOLD, GLYPH_ALIGN_LEFT, turn);
	}
}

void CodexMenu::DrawTitle(const SceneCalc& scene, const std::array<CHARACTER_TYPE, FIGHTER_COUNT>& characters) const
{
	const float seconds = GetSeconds();
	const float bandAngle = -8.0f * PI / 180.0f;
	// 斜めの黄緑の帯と、文字が流れる紫の帯。
	Band(640, 300, 204, bandAngle, INK);
	Band(640, 300, 190, bandAngle, LIME);
	Band(640, 446, 46, bandAngle, PURPLE);
	const float step = TextWidth(MARQUEE_TEXT, 19, TEXT2D_FONT_POP) + 40.0f;
	const float scroll = std::fmod(seconds * MARQUEE_SPEED, step);
	for (int i = -5; i <= 5; ++i) {
		Word(MARQUEE_TEXT, 640 + i * step - scroll, 446 - 13, 19, WHITE, TEXT2D_FONT_POP, GLYPH_ALIGN_LEFT, { 640, 446, bandAngle });
	}

	// 右：キャラを切り抜いて貼ったようなカード（キャラを順に入れ替える）。
	const RECT2D card{ 850, 70, 360, 330 };
	const MENU_TURN cardTurn{ card.X + card.Width * 0.5f, card.Y + card.Height * 0.5f, 5.0f * PI / 180.0f };
	Bar(card, 0.0f, WHITE, 5, 12, INK, cardTurn);
	const int shown = static_cast<int>(seconds / SHOWCASE_SECONDS) % CHARACTER_COUNT;
	const CHARACTER_TYPE character = static_cast<CHARACTER_TYPE>(shown);
	DrawCharacterPreview(character, { card.X + 40, card.Y + 14, card.Width - 80, card.Height - 64 }, 0.35f * sinf(seconds * 0.8f), 2.4f);
	Word(CHARACTER_DATA::Get(character).Name, card.X + card.Width * 0.5f, card.Y + card.Height - 46, 20, INK,
		TEXT2D_FONT_POP, GLYPH_ALIGN_CENTER, cardTurn);
	Bar({ card.X + 130, card.Y - 16, 110, 34 }, 0.0f, { 1.0f, 1.0f, 1.0f, 0.75f }, 0, 0, INK, { card.X + 185, card.Y, -6.0f * PI / 180.0f });
	(void)characters;

	// ロゴ：カクッカクッと小さく揺れる。
	const int logoStep = static_cast<int>(seconds / LOGO_STEP_SECONDS) % 4;
	const MENU_TURN logoTurn{ 380, 260, LOGO_ANGLES[logoStep] * PI / 180.0f };
	Shout("ブラキオン", 70, 140, 88, WHITE, 7, 9, GLYPH_ALIGN_LEFT, logoTurn);
	Shout("ファイト", 130, 262, 88, LIME, 7, 9, GLYPH_ALIGN_LEFT, logoTurn);
	Bar({ 560, 300, 236, 40 }, 0.0f, INK, 0, 0, INK, { 678, 320, 10.0f * PI / 180.0f });
	Word("BRACHION FIGHT", 678, 304, 20, LIME, TEXT2D_FONT_POP, GLYPH_ALIGN_CENTER, { 678, 320, 10.0f * PI / 180.0f });
	const MENU_TURN catchTurn{ 230, 72, -4.0f * PI / 180.0f };
	Bar({ 96, 52, 270, 42 }, 0.0f, WHITE, 4, 6, INK, catchTurn);
	Word("変な腕でぶっ飛ばせ！", 231, 58, 21, INK, TEXT2D_FONT_POP, GLYPH_ALIGN_CENTER, catchTurn);

	Button(0, "ゲームを始める", "対戦方法 → キャラ", scene.GetCursor() == 0, false, -40, 520, 560, 66);
	Button(1, "オプション", "遊び心地を調整", scene.GetCursor() == 1, true, -40, 598, 460, 54);
	Footer("↑↓ / W S / 左スティック  選ぶ    Enter / ○  決定");
}

void CodexMenu::DrawModeSelect(const SceneCalc& scene) const
{
	Header("あそびかた", "MODE SELECT");
	Bar({ 1010, 34, 220, 36 }, SLANT, WHITE, 3, 5);
	Word("STEP 1 / 2  キャラへ", 1120, 41, 15, INK, TEXT2D_FONT_BOLD, GLYPH_ALIGN_CENTER);
	static constexpr float WIDTHS[MODE_ITEM_COUNT] = { 560, 560, 540, 520, 440, 420 };
	float y = 124.0f;
	for (int i = 0; i < MODE_ITEM_COUNT; ++i) {
		const bool settings = i >= MODE_ITEM_OPTIONS; // オプション・戻るは小さな紫のボタン
		const float height = settings ? 54.0f : 66.0f;
		Button(i, MODE_NAMES[i], MODE_SUBTITLES[i], scene.GetCursor() == i, settings, -40, y, WIDTHS[i], height);
		y += height + 16.0f;
	}
	const int cursor = scene.GetCursor();
	InfoCard(INFO_CARD, MODE_NAMES[cursor], MODE_HELP[cursor][0], MODE_HELP[cursor][1], MODE_HELP[cursor][2]);
	Bar({ INFO_CARD.X + 10, INFO_CARD.Y + INFO_CARD.Height + 30, 420, 40 }, SLANT, INK, 0, 0);
	Word("次はキャラ選択。ルールもそこで選びます。", INFO_CARD.X + 30, INFO_CARD.Y + INFO_CARD.Height + 38, 16, LIME);
	Footer("↑↓ / W S / 左スティック  選ぶ    Enter / ○  決定    Backspace / ×  戻る");
}

void CodexMenu::DrawJoinAddress(const SessionAddressCalc& address) const
{
	Header("オンラインに参加", "JOIN / HOST ADDRESS");
	Bar({ 40, 150, 330, 44 }, SLANT, INK, 0, 0);
	Word("ホストの IPv4 アドレス", 64, 158, 20, WHITE);
	for (int part = 0; part < 4; ++part) {
		const float x = 64 + part * 150.0f;
		const bool selected = part == address.GetCursor();
		Bar({ x, 236 - (selected ? 6.0f : 0.0f), 124, 84 }, SLANT, selected ? LIME : WHITE, 4, selected ? 9.0f : 5.0f);
		Word(address.GetPart(part).c_str(), x + 62, 256 - (selected ? 6.0f : 0.0f), 32, INK, TEXT2D_FONT_POP, GLYPH_ALIGN_CENTER);
		if (part < 3) Shout(".", x + 136, 280, 30, WHITE, 3, 0);
	}
	const bool valid = address.IsValid();
	Bar({ 40, 362, valid ? 300.0f : 470.0f, 46 }, SLANT, valid ? LIME : WHITE, 4, 6);
	Word(valid ? "Enter / ○ で接続する" : "各区切りに 0 ～ 255 を入力してください", 64, 371, 19, INK, TEXT2D_FONT_POP);
	Word("Backspace は削除 / 空欄から戻る", 64, 436, 17, INK);
	InfoCard(INFO_CARD, "ホストに IP を聞こう",
		"キーボードで数字とピリオドを入力。", "パッドは左右で区切り、上下で数値変更。", "離れた相手とは Tailscale で接続します。");
	Footer("←→  区切りを選ぶ    ↑↓  数値を変える    Enter / ○  接続    ×  戻る");
}

void CodexMenu::DrawOptions(const SceneCalc& scene, const MATCH_TUNING& tuning) const
{
	Header("オプション", "OPTIONS / MATCH SETTINGS");
	const float jut = JUT_WIDTH * fabsf(sinf(GetSeconds() * JUT_SPEED));
	for (int i = 0; i < OPTION_ITEM_COUNT; ++i) {
		const float y = OPTION_TOP + i * OPTION_GAP;
		const bool selected = scene.GetCursor() == i;
		const bool command = i >= OPTION_ITEM_ARM_COSTS; // 表を開く・標準に戻す・戻る
		const float x = -30.0f + (selected ? jut : 0.0f);
		Bar({ x, y, 620 + (selected ? POP_WIDTH * 0.5f * GetPop() : 0.0f), OPTION_HEIGHT }, SLANT,
			selected ? LIME : command ? PURPLE : WHITE, 3, selected ? 7.0f : 4.0f);
		const XMFLOAT4 ink = command && !selected ? WHITE : INK;
		const char* name = i < TUNING_ITEM_COUNT ? OPTION_NAMES[i] : i == OPTION_ITEM_LOOK_SENSITIVITY ? "視点感度"
			: i == OPTION_ITEM_ARM_COSTS ? "腕の耐久の減る量" : i == OPTION_ITEM_RESET ? "標準に戻す" : "戻る";
		Word(name, x + 64, y + 8, 17, ink);
		char value[80] = "";
		if (i == OPTION_ITEM_ARM_COSTS) sprintf_s(value, "表を開く >");
		else if (i == OPTION_ITEM_LOOK_SENSITIVITY) sprintf_s(value, "× %.2f", ActionGetLookSensitivity());
		else if (i < TUNING_ITEM_COUNT) {
			const auto item = static_cast<TUNING_ITEM>(i);
			if (item == TUNING_ARM_USES && TuningCalcIsUnlimitedUses(tuning)) sprintf_s(value, "無制限");
			else if (item == TUNING_HAMMER_STUN) sprintf_s(value, "%.1f 秒", TuningCalcValue(tuning, item));
			else if (item == TUNING_CAPSULES) sprintf_s(value, "%d 個 / %.1f 秒", TuningCalcCapsuleCount(tuning), TuningCalcCapsuleSeconds(tuning));
			else sprintf_s(value, "× %.2f", TuningCalcValue(tuning, item));
		}
		if (value[0]) Word(value, x + OPTION_VALUE_X, y + 7, 18, selected ? PURPLE : ink, TEXT2D_FONT_POP);
		if (selected && i <= OPTION_ITEM_LOOK_SENSITIVITY) Word("<", x + OPTION_VALUE_X - 30, y + 7, 18, PURPLE, TEXT2D_FONT_POP);
	}
	const int selected = scene.GetCursor();
	const MENU_TURN turn{ INFO_CARD.X + INFO_CARD.Width * 0.5f, INFO_CARD.Y + INFO_CARD.Height * 0.5f, CARD_TURN };
	int levels = 0;
	int level = 0;
	if (selected < TUNING_ITEM_COUNT) {
		InfoCard(INFO_CARD, OPTION_NAMES[selected], OPTION_HELP[selected][0], OPTION_HELP[selected][1], OPTION_HELP[selected][2]);
		levels = TuningCalcLevelCount(static_cast<TUNING_ITEM>(selected));
		level = tuning.Levels[selected];
	}
	else if (selected == OPTION_ITEM_LOOK_SENSITIVITY) {
		InfoCard(INFO_CARD, "視点感度", "カメラを回す速さの倍率です。", "マウス・矢印キー・右スティックに効きます。",
			"この PC だけの設定で、すぐに効きます。");
		levels = ActionGetLookSensitivityLevelCount();
		level = ActionGetLookSensitivityLevel();
	}
	else if (selected == OPTION_ITEM_ARM_COSTS) InfoCard(INFO_CARD, "腕の耐久の減る量",
		"攻撃ごとに耐久が減る量の表です。", "Enter / ○ で表を開きます。", "オンラインははじめの値を使います。");
	else InfoCard(INFO_CARD, selected == OPTION_ITEM_RESET ? "標準設定に戻す" : "設定を閉じる",
		selected == OPTION_ITEM_RESET ? "すべての項目と表を標準へ戻します。" : "変更した値は、そのまま保持されます。",
		"Enter / ○ で決定します。", "ファイルへの保存は行いません。");
	// 段のメーター：今の段までを黄緑、残りを薄い紫の斜めの四角で並べる。
	if (levels > 0) {
		const float width = 380.0f / levels;
		for (int i = 0; i < levels; ++i) {
			const RECT2D cell{ INFO_CARD.X + 34 + i * width, INFO_CARD.Y + 246, width - 8, 26 };
			Bar(cell, SLANT, i <= level ? LIME : XMFLOAT4{ 0.85f, 0.80f, 0.92f, 1.0f }, i <= level ? 2.0f : 0.0f, 0, INK, turn);
		}
	}
	Bar({ INFO_CARD.X + 10, INFO_CARD.Y + INFO_CARD.Height + 30, 330, 40 }, SLANT, INK, 0, 0);
	Word("変更は次の試合から適用", INFO_CARD.X + 30, INFO_CARD.Y + INFO_CARD.Height + 37, 18, LIME, TEXT2D_FONT_POP);
	Word("オンラインはホストの設定を全員に適用します。", INFO_CARD.X + 20, INFO_CARD.Y + INFO_CARD.Height + 86, 15, INK);
	Footer("↑↓ / W S  選ぶ    ←→ / A D  変える    Enter / ○  決定    Backspace / ×  戻る");
}

void CodexMenu::DrawArmCosts(const SceneCalc& scene, const ARM_COST_TABLE& costs) const
{
	Header("腕の耐久の減る量", "OPTIONS / ARM DURABILITY");
	int selectedRow = -1, selectedItem = -1;
	ArmCostCalcUsedCell(scene.GetCursor(), selectedRow, selectedItem);
	const float cellLeft = COST_LEFT + COST_LABEL_WIDTH;
	Bar({ COST_LEFT - 60, 136, COST_ROW_WIDTH + 60, 34 }, 0.0f, INK, 0, 0);
	for (int item = 0; item < ARM_COST_ITEM_COUNT; ++item) {
		Word(COST_COLUMNS[item], cellLeft + item * COST_CELL_WIDTH + 4, 144, 12, item == selectedItem ? LIME : WHITE);
	}
	// 行が多いときは、見せる行の数だけ描き、右にスクロールの位置を出す（行の間隔は変えない）。
	const int top = scene.GetCostScrollTop();
	const int shown = (std::min)(static_cast<int>(ARM_COST_ROW_COUNT), ARM_COST_VISIBLE_ROWS);
	for (int row = top; row < top + shown; ++row) {
		const float y = COST_TOP + (row - top) * COST_GAP;
		const bool rowSelected = row == selectedRow;
		Bar({ COST_LEFT - 60, y, COST_ROW_WIDTH + 60, COST_GAP - 8 }, 0.0f, rowSelected ? LIME : WHITE, 3, rowSelected ? 6.0f : 3.0f);
		Word(COST_ROWS[row], COST_LEFT, y + 10, 15, INK);
		for (int item = 0; item < ARM_COST_ITEM_COUNT; ++item) {
			const float x = cellLeft + item * COST_CELL_WIDTH;
			const bool selected = rowSelected && item == selectedItem;
			if (selected) Bar({ x, y + 4, COST_CELL_WIDTH - 10, COST_GAP - 16 }, SLANT, PURPLE, 0, 0);
			char value[16];
			if (ArmCostCalcIsUsed(row, item)) sprintf_s(value, "%d", costs.Values[row][item]);
			else sprintf_s(value, "-"); // 使わない欄（文字画像に「―」がないので半角）
			Word(value, x + 22, y + 8, 17, selected ? WHITE : ArmCostCalcIsUsed(row, item) ? INK : MUTED, TEXT2D_FONT_POP);
		}
	}
	ScrollBar(COST_LEFT + COST_ROW_WIDTH + 14, COST_TOP, shown * COST_GAP - 8, top, shown, ARM_COST_ROW_COUNT);
	const int back = ArmCostCalcUsedCount();
	Button(ARM_COST_ROW_COUNT, "戻る", "", scene.GetCursor() == back, true, -40, COST_TOP + shown * COST_GAP + 2, 300, 44);
	if (selectedRow >= 0) {
		char line1[64];
		sprintf_s(line1, "%s：%d（はじめは %d）", COST_COLUMNS[selectedItem], costs.Values[selectedRow][selectedItem],
			ArmCostCalcDefault().Values[selectedRow][selectedItem]);
		InfoCard(INFO_CARD, COST_ROWS[selectedRow], line1, COST_NOTES[selectedRow][0], COST_NOTES[selectedRow][1]);
	}
	else InfoCard(INFO_CARD, "表を閉じる", "変更した値は、そのまま保持されます。", "Enter / ○ でオプションへ戻ります。", "");
	Word("当たり：1人でも効いた　外れ：誰にも効かなかった", INFO_CARD.X + 10, INFO_CARD.Y + INFO_CARD.Height + 34, 15, INK);
	Word("耐久は 100。オンラインははじめの値を使います。", INFO_CARD.X + 10, INFO_CARD.Y + INFO_CARD.Height + 64, 15, INK);
	Footer("↑↓ / W S  選ぶ    ←→ / A D  5 ずつ変える    Backspace / ×  戻る");
}

void CodexMenu::DrawLobby(const SceneCalc& scene, const VersusCalc& participants, const NetSession& net,
	const std::array<CHARACTER_TYPE, FIGHTER_COUNT>& characters) const
{
	const bool online = net.IsOnline();
	Header("キャラをえらぶ", online ? "CHARACTER SELECT / ONLINE" : "CHARACTER SELECT");
	// 1P（オンラインはホスト）だけが、上下でルールの行・自分のカード・人のいない枠を選ぶ。
	const bool owner = !online || net.IsHost();
	const int focus = owner ? participants.GetLobbyFocus() : -2;
	const GAME_RULE rule = online ? net.GetRule() : scene.GetRule();
	const bool ruleFocused = focus == VERSUS_LOBBY_FOCUS_RULE;
	const float ruleJut = ruleFocused ? JUT_WIDTH * fabsf(sinf(GetSeconds() * JUT_SPEED)) : 0.0f;
	Bar({ 700 + ruleJut, 36, 530, 42 }, SLANT, ruleFocused ? LIME : WHITE, 4, ruleFocused ? 8.0f : 5.0f);
	Word(rule == GAME_RULE_GAME ? "ルール   <  通常の対戦（GAME）  >" : "ルール   <  腕を自由に試す（DEBUG）  >",
		730 + ruleJut, 44, 17, INK, TEXT2D_FONT_POP);
	const float seconds = GetSeconds();
	const float yaw = std::fmod(seconds, PREVIEW_TURN_SECONDS) * (2.0f * PI / PREVIEW_TURN_SECONDS);
	int joinedCount = 0;
	int readyCount = 0;
	for (int slot = 0; slot < FIGHTER_COUNT; ++slot) {
		const float x = LOBBY_CARD_LEFT + slot * (LOBBY_CARD_WIDTH + LOBBY_CARD_GAP);
		const bool joined = online ? (net.GetMask() & (1u << slot)) != 0 : participants.IsHuman(slot);
		const bool ready = online ? (net.GetReadyMask() & (1u << slot)) != 0 : participants.IsReady(slot);
		if (joined) ++joinedCount;
		if (joined && ready) ++readyCount;
		const XMFLOAT4 color = Fighter::GetPlayerColor(slot);
		const RECT2D card{ x, LOBBY_CARD_TOP, LOBBY_CARD_WIDTH, LOBBY_CARD_HEIGHT };
		// 選んでいるカードは黄緑の太いふち。参加した人の影はプレイヤーの色。
		if (focus == slot) Number2DDrawBox(Grow(card, 11), LIME);
		Bar(card, 0.0f, joined ? WHITE : XMFLOAT4{ 0.93f, 0.88f, 0.95f, 1.0f }, 5, 10, joined ? color : INK);
		Bar({ x + 8, LOBBY_CARD_TOP + 10, LOBBY_CARD_WIDTH - 30, 50 }, SLANT, joined ? color : MUTED, 0, 0);
		char label[32];
		sprintf_s(label, "%dP", slot + 1);
		Shout(label, x + 24, LOBBY_CARD_TOP + 14, 30, WHITE, 3, 0);
		const int device = participants.GetDevice(slot);
		const char* deviceName = !joined ? "" : online ? (slot == net.GetLocalSlot() ? "ONLINE / あなた" : "ONLINE")
			: device == ACTION_DEVICE_KEYBOARD ? "KEYBOARD" : "CONTROLLER";
		Word(deviceName, x + 90, LOBBY_CARD_TOP + 26, 13, WHITE);
		const RECT2D preview{ x + 17, LOBBY_CARD_TOP + 70, LOBBY_CARD_WIDTH - 34, 300 };
		if (!joined) {
			// オンラインの空き枠はダミーだけ（キャラは選べない）。オフラインは 1P がダミーと CPU・キャラを選ぶ。
			const int level = online ? VERSUS_CPU_NONE : participants.GetCpuLevel(slot);
			Word(CPU_LEVEL_NAMES[level], x + 90, LOBBY_CARD_TOP + 26, 13, level != VERSUS_CPU_NONE ? LIME : WHITE);
			if (online) {
				Word("ダミー", x + LOBBY_CARD_WIDTH * 0.5f, LOBBY_CARD_TOP + 200, 24, MUTED, TEXT2D_FONT_POP, GLYPH_ALIGN_CENTER);
				continue;
			}
			DrawCharacterPreview(characters[slot], preview, yaw, PREVIEW_VIEW_HEIGHT);
			Word(CHARACTER_DATA::Get(characters[slot]).Name, x + LOBBY_CARD_WIDTH * 0.5f, LOBBY_CARD_TOP + 382, 22, MUTED,
				TEXT2D_FONT_POP, GLYPH_ALIGN_CENTER);
			if (focus == slot) Word("←→ キャラ   ○ CPU の強さ", x + 6, LOBBY_STATUS_Y, 15, INK);
			else if (!scene.IsSolo()) Word("Enter / ○ で参加", x + 6, LOBBY_STATUS_Y, 15, INK);
			continue;
		}
		DrawCharacterPreview(characters[slot], preview, yaw, PREVIEW_VIEW_HEIGHT);
		Word(CHARACTER_DATA::Get(characters[slot]).Name, x + LOBBY_CARD_WIDTH * 0.5f, LOBBY_CARD_TOP + 382, 22, INK,
			TEXT2D_FONT_POP, GLYPH_ALIGN_CENTER);
		if (ready) {
			// 準備OK は、カードの下に傾けて押したはんこ。
			const MENU_TURN stamp{ x + LOBBY_CARD_WIDTH * 0.5f, LOBBY_STATUS_Y + 4, -6.0f * PI / 180.0f };
			Bar({ x + 40, LOBBY_STATUS_Y - 20, LOBBY_CARD_WIDTH - 80, 48 }, 0.0f, LIME, 4, 6, INK, stamp);
			Word("準備OK！", x + LOBBY_CARD_WIDTH * 0.5f, LOBBY_STATUS_Y - 12, 22, INK,
				TEXT2D_FONT_POP, GLYPH_ALIGN_CENTER, stamp);
		}
		else Word("←→ キャラ   ○ 準備OK", x + 6, LOBBY_STATUS_Y, 15, INK);
	}
	// 全員が準備OKなら開始の帯を出す（オンラインはホストのほかに1人以上つながっていること）。
	const bool allReady = readyCount > 0 && readyCount == joinedCount && (!online || joinedCount > 1);
	if (allReady) {
		const float angle = -2.0f * PI / 180.0f;
		Band(640, LOBBY_BANNER_Y + 26, 56, angle, INK);
		Band(640, LOBBY_BANNER_Y + 26, 46, angle, LIME);
		Shout("READY TO FIGHT!", 300, LOBBY_BANNER_Y + 10, 26, WHITE, 3, 4, GLYPH_ALIGN_CENTER, { 640, LOBBY_BANNER_Y + 26, angle });
		Word(online ? "ホストが Enter / ○ で開始" : "1P が Enter / ○ で開始", 820, LOBBY_BANNER_Y + 16, 18, INK,
			TEXT2D_FONT_POP, GLYPH_ALIGN_CENTER, { 640, LOBBY_BANNER_Y + 26, angle });
	}
	else {
		Word(online ? "全員が ○ で準備OKになると、ホストが始められます。" : "参加した全員が準備OKになると、1P が始められます。",
			LOBBY_CARD_LEFT, LOBBY_BANNER_Y + 16, 16, INK);
		if (net.IsHost()) {
			// 相手に伝えるアドレスは、帯の右に1行で出す。
			const std::string ip = "HOST IP  " + net.GetAddresses();
			Bar({ 760, LOBBY_BANNER_Y + 8, 470, 36 }, SLANT, INK, 0, 0);
			Word(ip.c_str(), 784, LOBBY_BANNER_Y + 16, 15, LIME);
		}
	}
	Footer(online ? "↑↓  ルール（ホスト）   ←→  キャラ・ルール   Enter / ○  準備OK・ホストが開始   ×  戻る"
		: "↑↓  選ぶ（1P）   ←→  キャラ・ルール   Enter / ○  参加・準備OK・開始   Backspace / ×  取り消し・戻る");
}

void CodexMenu::DrawResult(const SceneCalc& scene, const VersusCalc& participants,
	const std::array<CHARACTER_TYPE, FIGHTER_COUNT>& characters) const
{
	Header("リザルト", "RESULT / KNOCKOUT RANKING");
	const int count = (std::min)(participants.GetHumanCount(), static_cast<int>(FIGHTER_COUNT));
	const float seconds = GetSeconds();
	if (count > 0) {
		// 勝った人を大きく。1位が2人以上なら引き分け。
		const int winner = participants.GetRankedSlot(0);
		const bool draw = count > 1 && participants.GetRank(participants.GetRankedSlot(1)) == participants.GetRank(winner);
		char title[48];
		if (draw) sprintf_s(title, "引き分け！");
		else sprintf_s(title, participants.GetDevice(winner) == VERSUS_DEVICE_CPU ? "%dP（CPU）の勝ち！" : "%dP の勝ち！", winner + 1);
		const MENU_TURN turn{ 400, 150, -3.0f * PI / 180.0f };
		Bar({ 60, 100, 150, 32 }, SLANT, INK, 0, 0, INK, turn);
		Word("WINNER", 135, 104, 16, LIME, TEXT2D_FONT_POP, GLYPH_ALIGN_CENTER, turn);
		Shout(title, 60, 128, 46, draw ? WHITE : Fighter::GetPlayerColor(winner), 5, 7, GLYPH_ALIGN_LEFT, turn);
	}
	for (int order = 0; order < count; ++order) {
		const int slot = participants.GetRankedSlot(order);
		const int rank = participants.GetRank(slot);
		const bool first = rank == 1;
		const float x = RESULT_PANEL_LEFT + order * (RESULT_PANEL_WIDTH + RESULT_PANEL_GAP);
		const float top = RESULT_PANEL_TOP - (first ? RESULT_WINNER_RISE : 0.0f);
		const float height = RESULT_PANEL_HEIGHT + (first ? RESULT_WINNER_RISE : 0.0f);
		const XMFLOAT4 color = Fighter::GetPlayerColor(slot);
		if (first) Bar(Grow({ x, top, RESULT_PANEL_WIDTH, height }, 8), 0.06f, LIME, 0, 0);
		Bar({ x, top, RESULT_PANEL_WIDTH, height }, 0.06f, color, 5, 10);
		// 1位はゆっくり左右を向き、ほかは正面で止まる。
		const float yaw = first ? 0.5f * sinf(seconds * 0.9f) : 0.0f;
		DrawCharacterPreview(characters[slot], { x + 14, top + 92, RESULT_PANEL_WIDTH - 28, height - 160 }, yaw, first ? 2.5f : 2.8f);
		char text[32];
		sprintf_s(text, "%d", rank);
		Shout(text, x + 18, top + 6, 54, first ? LIME : WHITE, 5, 5);
		Word("位", x + 18 + TextWidth(text, 54, TEXT2D_FONT_POP) + 4, top + 50, 18, WHITE, TEXT2D_FONT_POP);
		sprintf_s(text, participants.GetDevice(slot) == VERSUS_DEVICE_CPU ? "%dP  CPU" : "%dP", slot + 1);
		Shout(text, x + RESULT_PANEL_WIDTH - 16, top + 18, 22, WHITE, 3, 0, GLYPH_ALIGN_RIGHT);
		Bar({ x + 10, top + height - 58, RESULT_PANEL_WIDTH - 20, 46 }, 0.06f, INK, 0, 0);
		sprintf_s(text, "%d", participants.GetScore(slot));
		Word("KO", x + 30, top + height - 48, 17, WHITE, TEXT2D_FONT_POP);
		Word(text, x + RESULT_PANEL_WIDTH - 30, top + height - 54, 26, LIME, TEXT2D_FONT_POP, GLYPH_ALIGN_RIGHT);
	}
	Button(0, "もう一度", "", scene.GetCursor() == RESULT_ITEM_RETRY, false,
		RESULT_BUTTON_X, RESULT_BUTTON_TOP, RESULT_BUTTON_WIDTH, 62);
	Button(1, "キャラ選択へ", "", scene.GetCursor() == RESULT_ITEM_CHARACTER, false,
		RESULT_BUTTON_X, RESULT_BUTTON_TOP + RESULT_BUTTON_GAP, RESULT_BUTTON_WIDTH, 62);
	Button(2, "メニューへ", "", scene.GetCursor() == RESULT_ITEM_TITLE, true,
		RESULT_BUTTON_X, RESULT_BUTTON_TOP + RESULT_BUTTON_GAP * 2, RESULT_BUTTON_WIDTH, 62);
	Word("同じ参加者・キャラでもう一度", RESULT_BUTTON_X + 10, RESULT_BUTTON_TOP - 30, 14, INK);
	Footer("1P : ↑↓ / W S  選ぶ    Enter / ○  決定    （オンラインのメニューへは、ロビーへ戻ります）");
}

void CodexMenu::DrawPause(const SceneCalc& scene) const
{
	// 試合の画面を暗くして、斜めのピンクの帯の上にボタンを並べる。
	Number2DDrawBox({ 0, 0, 1280, 720 }, { INK.x, INK.y, INK.z, 0.55f });
	const float angle = -6.0f * PI / 180.0f;
	Band(640, 360, 330, angle, INK);
	Band(640, 360, 314, angle, { PINK.x, PINK.y, PINK.z, 0.92f });
	Shout("ポーズ", 120, 214, 64, WHITE, 6, 8, GLYPH_ALIGN_LEFT, { 220, 250, angle });
	Word("PAUSE", 134, 312, 20, LIME, TEXT2D_FONT_POP, GLYPH_ALIGN_LEFT, { 220, 250, angle });
	static constexpr const char* NAMES[PAUSE_ITEM_COUNT] = { "再開する", "ロビーに戻る", "タイトルに戻る" };
	static constexpr const char* SUBTITLES[PAUSE_ITEM_COUNT] = { "試合の続きから", "キャラ選択へ", "最初の画面へ" };
	for (int i = 0; i < PAUSE_ITEM_COUNT; ++i) {
		Button(i, NAMES[i], SUBTITLES[i], scene.GetCursor() == i, i == PAUSE_ITEM_TITLE, 560, 230 + i * 84.0f, 470, 66);
	}
	Footer("↑↓ / W S  選ぶ    Enter / ○  決定    R / OPTIONS・Backspace / ×  試合に戻る");
}

void CodexMenu::Draw(const SceneCalc& scene, const VersusCalc& participants, const NetSession& net,
	const SessionAddressCalc& address, const std::array<CHARACTER_TYPE, FIGHTER_COUNT>& characters,
	const MATCH_TUNING& tuning, const ARM_COST_TABLE& costs) const
{
	Background();
	switch (scene.GetScene()) {
	case SCENE_OPTIONS: DrawOptions(scene, tuning); break;
	case SCENE_ARM_COSTS: DrawArmCosts(scene, costs); break;
	case SCENE_TITLE: DrawLobby(scene, participants, net, characters); break;
	case SCENE_RESULT: DrawResult(scene, participants, characters); break;
	case SCENE_GAME_START: DrawTitle(scene, characters); break;
	case SCENE_MODE_SELECT: DrawModeSelect(scene); break;
	case SCENE_JOIN_ADDRESS: DrawJoinAddress(address); break;
	default: break;
	}
}
