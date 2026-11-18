#include "game/codex_menu.h"
#include "game/netsession.h"
#include "game/character.h"
#include "game/fighter.h"
#include "core/shader.h"
#include "input/action.h"
#include "render/number2d.h"
#include "render/text2d.h"
#include <cmath>
#include <cstdio>

// 調整用の値（1280×720 の共通座標。文字と背景は試合の描画に依存しない）
static constexpr XMFLOAT4 INK{ 0.018f, 0.032f, 0.065f, 1.0f };
static constexpr XMFLOAT4 PANEL{ 0.033f, 0.063f, 0.105f, 1.0f };
static constexpr XMFLOAT4 LINE{ 0.10f, 0.21f, 0.28f, 1.0f };
static constexpr XMFLOAT4 CYAN{ 0.24f, 0.93f, 0.92f, 1.0f };
static constexpr XMFLOAT4 WHITE{ 0.91f, 0.96f, 1.0f, 1.0f };
static constexpr XMFLOAT4 MUTED{ 0.51f, 0.65f, 0.73f, 1.0f };
static constexpr XMFLOAT4 GOLD{ 1.0f, 0.73f, 0.34f, 1.0f };
static constexpr float LEFT = 64.0f;
static constexpr float ROW_WIDTH = 580.0f;
static constexpr float RIGHT = 708.0f;
static constexpr float OPTION_TOP = 156.0f;
static constexpr float OPTION_GAP = 46.0f;
static constexpr float TEXT_SCALE = 1.35f; // 文字画像の上下の余白を見込み、メニューの文字を大きくする。
static constexpr float PI = 3.14159265f;
static constexpr float PREVIEW_TURN_SECONDS = 12.0f; // 全身を見せる1回転の秒数
static constexpr float PREVIEW_BODY_HEIGHT = 2.0f; // キャラ共通の表示上の高さ
static constexpr float PREVIEW_TOP = 244.0f;
static constexpr float PREVIEW_WIDTH = 248.0f;
static constexpr float PREVIEW_HEIGHT = 284.0f;
static constexpr float PREVIEW_VIEW_HEIGHT = 2.8f; // 上下と回転中の張り出しの余白を含む
static constexpr float LOBBY_CARD_TOP = 160.0f; // キャラ選択の 1P〜4P のカード
static constexpr float LOBBY_CARD_WIDTH = 270.0f;
static constexpr float LOBBY_CARD_HEIGHT = 420.0f;
static constexpr float LOBBY_CARD_GAP = 24.0f; // 4枚で LEFT から 1152 の幅に収める
static constexpr float LOBBY_STATUS_Y = 540.0f; // カードの下の「準備OK」などの行
static constexpr float LOBBY_BANNER_Y = 590.0f; // 「READY TO FIGHT」の帯
static constexpr float LOBBY_IP_Y = 138.0f; // ホストの IP（見出しの右下）
static constexpr float RESULT_ROW_TOP = 440.0f; // 結果の画面のメニューの1行目
static constexpr float RESULT_ROW_GAP = 64.0f;
static constexpr float RESULT_ROW_HEIGHT = 58.0f;
static constexpr float COST_TOP = 178.0f; // 耐久の減る量の表の1行目
static constexpr float COST_GAP = 49.0f; // 表の行の間隔
static constexpr float COST_LABEL_WIDTH = 150.0f; // 表の腕の名前の列の幅
static constexpr float COST_CELL_WIDTH = 86.0f; // 表の数字の欄の幅
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
static constexpr const char* COST_ROWS[ARM_COST_ROW_COUNT] = { "触手", "ハンマー", "ナックル", "レーザー", "刀", "モーニングスター", "槍", "ガード" };
static constexpr const char* COST_NOTES[ARM_COST_ROW_COUNT][2] = {
	{ "弱：つかんだ・足場に付いた", "強：回転" }, { "強：衝撃波", "そのほか：空中の跳ね" }, { "強：突進（空中も同じ）", "" },
	{ "弱：単発のレーザー銃", "強：溜めきった砲（貫通）" },
	{ "強・外れ：飛んだ瞬間に減る", "当たれば強・当たりとの差を足す" }, { "弱：扇　強：投げた鉄球", "回している間は減らない" },
	{ "弱：突き（直線上の全員）", "強：投げた槍が刺さったか" },
	{ "パリィできずに防いだとき", "" }
};
static constexpr const char* MODE_NAMES[] = {
	"ひとりで遊ぶ", "この PC で対戦", "オンラインを開催", "オンラインに参加", "オプション", "タイトルへ戻る"
};
static constexpr const char* MODE_SUBTITLES[] = {
	"SOLO / ダミー相手に練習", "LOCAL VS / 最大 4 人・画面分割", "HOST / 参加者を待つ", "JOIN / ホストの IP を入力", "OPTIONS / 次の試合を調整", "BACK / 最初の画面へ"
};
static constexpr const char* MODE_HELP[][3] = {
	{ "決定した機器を 1P にして遊びます。", "ほかのキャラは練習用のダミーです。", "操作や腕の動きを試すのにおすすめ。" },
	{ "キャラ選択でパッドやキーボードから参加。", "人数に合わせて画面を分割します。", "全員が準備OKなら 1P の決定で開始。" },
	{ "この PC をホストにして待機します。", "表示された IP を相手に伝えます。", "離れた相手とは Tailscale で接続。" },
	{ "ホストの IPv4 アドレスを入力します。", "接続後はロビーで参加を待ちます。", "ホストと同じ版のゲームを使います。" },
	{ "試合の遊び心地を調整できます。", "変更は次の試合から適用されます。", "オンラインはホストの設定を使用。" },
	{ "キャラとルールは選び直せます。", "オプションは起動中そのまま保持。", "終了は Esc キーから行えます。" }
};

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
	// くり返し表示する説明は、最初に画像へ変えておく。Text2D が画像の寿命を管理する。
	for (const auto* name : OPTION_NAMES) Text2DCreate(name);
	for (const auto& lines : OPTION_HELP) for (const auto* text : lines) Text2DCreate(text);
	for (const auto* name : COST_COLUMNS) Text2DCreate(name);
	for (const auto* name : COST_ROWS) Text2DCreate(name);
	for (const auto* name : MODE_NAMES) Text2DCreate(name);
	for (const auto* text : MODE_SUBTITLES) Text2DCreate(text);
	for (const auto& lines : MODE_HELP) for (const auto* text : lines) Text2DCreate(text);
}

void CodexMenu::Finalize()
{
	for (auto& model : m_PreviewModels) model.Finalize();
	for (auto& pose : m_PreviewPoses) pose = {};
}

void CodexMenu::DrawCharacterPreview(CHARACTER_TYPE character, float x, float yaw) const
{
	const auto& model = m_PreviewModels[character];
	if (!model.IsLoaded()) {
		Text("モデルを読み込めません", x + 20, 365, 15, MUTED);
		return;
	}
	const auto& data = CHARACTER_DATA::Get(character);
	const float scale = PREVIEW_BODY_HEIGHT / data.ModelHeight;
	const XMMATRIX world = XMMatrixScaling(scale, scale, scale)
		* XMMatrixRotationY(data.ModelYaw + yaw)
		* XMMatrixTranslation(0.0f, -data.ModelFootY * scale, 0.0f);
	// カード同士は重ならず、フレーム先頭の Clear で深度も消されている。
	SetViewport(x + 16.0f, PREVIEW_TOP, PREVIEW_WIDTH, PREVIEW_HEIGHT);
	SetDepthEnable(true);
	Shader_SetViewMatrix(XMMatrixLookAtLH(XMVectorSet(0.0f, 1.3f, 4.5f, 1.0f),
		XMVectorSet(0.0f, 1.0f, 0.0f, 1.0f), XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)));
	Shader_SetProjectionMatrix(XMMatrixOrthographicLH(
		PREVIEW_VIEW_HEIGHT * PREVIEW_WIDTH / PREVIEW_HEIGHT, PREVIEW_VIEW_HEIGHT, 0.1f, 20.0f));
	LIGHT light{};
	light.Enable = TRUE;
	light.Direction = { 0.5f, -1.0f, -0.8f, 0.0f };
	light.Diffuse = { 0.8f, 0.85f, 0.9f, 1.0f };
	light.Ambient = { 0.4f, 0.4f, 0.45f, 1.0f };
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

void CodexMenu::Text(const char* text, float x, float y, float height, const XMFLOAT4& color) const
{
	// 固定文字列はキャッシュされる。IP と設定値も同じ値なら画像を作り直さない。
	Text2DDraw(Text2DCreate(text), x, y, height * TEXT_SCALE, GLYPH_ALIGN_LEFT, color);
}

void CodexMenu::Background() const
{
	Number2DDrawBox({ 0, 0, 1280, 720 }, INK);
	Number2DDrawBox({ 676, 0, 604, 720 }, { 0.023f, 0.044f, 0.079f, 1 });
	RECT2D grid[80]{};
	int count = 0;
	for (int x = 4; x < 1280; x += 48) grid[count++] = { static_cast<float>(x), 0, 1, 720 };
	for (int y = 0; y < 720; y += 48) grid[count++] = { 0, static_cast<float>(y), 1280, 1 };
	Number2DDrawRects(grid, count, { 0.10f, 0.25f, 0.34f, 0.16f });
	Number2DDrawBox({ LEFT, 30, 32, 4 }, CYAN);
	Number2DDrawBox({ 102, 30, 10, 4 }, GOLD);
	Text("BRACHION FIGHT", 124, 20, 19, WHITE);
	Text("BATTLE SYSTEM / 01", 1005, 22, 16, MUTED);
	Number2DDrawBox({ LEFT, 58, 1152, 1 }, LINE);
	Number2DDrawBox({ LEFT, 641, 1152, 1 }, LINE);
	Text("PROTO1  /  ブラキオンファイト", LEFT, 693, 13, MUTED);
	Text("READY TO CONNECT", 1052, 693, 13, CYAN);
}

void CodexMenu::Reactor(float x, float y, float radius) const
{
	// 円弧・目盛りを四角の一覧にして描く。ゲームのモデルを背景に使わない。
	RECT2D marks[512]{};
	int count = 0;
	for (int ring = 0; ring < 3; ++ring) {
		const float r = radius - ring * 22.0f;
		for (int i = 0; i < 120; ++i) {
			if ((i / 15 + ring) % 4 == 0) continue;
			const float angle = static_cast<float>(i) * PI / 60.0f;
			marks[count++] = { x + cosf(angle) * r, y + sinf(angle) * r, ring == 1 ? 3.0f : 2.0f, 3 };
		}
	}
	Number2DDrawRects(marks, count, { CYAN.x, CYAN.y, CYAN.z, 0.40f });
	Number2DDrawBox({ x - radius - 25, y, radius * 2 + 50, 1 }, LINE);
	Number2DDrawBox({ x, y - radius - 25, 1, radius * 2 + 50 }, LINE);
	// 左右へ広がる腕と中央のコアを、回路のような直線で組む。
	Number2DDrawBox({ x - 28, y - 55, 56, 110 }, PANEL);
	Number2DDrawBox({ x - 16, y - 39, 32, 78 }, CYAN);
	for (int side = -1; side <= 1; side += 2) {
		const float left = side < 0 ? x - 104 : x + 28;
		Number2DDrawBox({ left, y - 8, 76, 16 }, CYAN);
		Number2DDrawBox({ side < 0 ? x - 116 : x + 104, y - 45, 12, 53 }, CYAN);
		Number2DDrawBox({ side < 0 ? x - 124 : x + 100, y - 59, 28, 14 }, GOLD);
	}
	Text("ARM LINK", x - 52, y + 77, 19, WHITE);
	Text("DUAL EQUIPMENT SYSTEM", x - 110, y + radius + 36, 15, MUTED);
}

void CodexMenu::Header(const char* title, const char* eyebrow, int step) const
{
	Text(eyebrow, LEFT, 79, 15, CYAN);
	Text(title, LEFT, 101, 34, WHITE);
	if (step > 0) {
		const char* names[] = { "01  対戦方法", "02  ルール", "03  キャラクター" };
		for (int i = 0; i < 3; ++i) {
			const float x = RIGHT + i * 172.0f;
			Number2DDrawBox({ x, 100, 154, 3 }, i + 1 == step ? CYAN : LINE);
			Text(names[i], x, 112, 16, i + 1 == step ? WHITE : MUTED);
		}
	}
}

void CodexMenu::Row(const char* title, const char* subtitle, int index, bool selected, float y, float height) const
{
	Number2DDrawBox({ LEFT, y, ROW_WIDTH, height }, selected ? XMFLOAT4{ 0.055f, 0.20f, 0.25f, 1 } : PANEL);
	Number2DDrawBox({ LEFT, y, selected ? 4.0f : 1.0f, height }, selected ? CYAN : LINE);
	Number2DDrawBox({ LEFT, y + height - 1, ROW_WIDTH, 1 }, selected ? CYAN : LINE);
	char number[8];
	sprintf_s(number, "%02d", index + 1);
	Text(number, LEFT + 20, y + 15, 17, selected ? CYAN : MUTED);
	Text(title, LEFT + 67, y + 7, subtitle[0] ? 25.0f : 24.0f, selected ? WHITE : MUTED);
	if (subtitle[0]) Text(subtitle, LEFT + 68, y + 38, 14, selected ? CYAN : MUTED);
	if (selected) Text(">", LEFT + ROW_WIDTH - 32, y + 14, 22, CYAN);
}

void CodexMenu::Detail(const char* tag, const char* title, const char* line1, const char* line2, const char* line3) const
{
	Number2DDrawBox({ RIGHT, 204, 508, 324 }, PANEL);
	Number2DDrawBox({ RIGHT, 204, 40, 3 }, CYAN);
	Text(tag, RIGHT + 28, 234, 15, CYAN);
	Text(title, RIGHT + 28, 274, 28, WHITE);
	// 空の行は描かない（耐久の表の説明は1行だけの腕がある）。
	if (line1[0]) Text(line1, RIGHT + 28, 342, 20, WHITE);
	if (line2[0]) Text(line2, RIGHT + 28, 381, 20, MUTED);
	if (line3[0]) Text(line3, RIGHT + 28, 420, 20, MUTED);
}

void CodexMenu::Footer(const char* help) const
{
	Text(help, LEFT, 659, 18, WHITE);
	Text("Esc  終了  /  F11  全画面", 973, 660, 16, MUTED);
}

void CodexMenu::DrawOptions(const SceneCalc& scene, const MATCH_TUNING& tuning) const
{
	Header("オプション", "OPTIONS / MATCH SETTINGS", 0);
	for (int i = 0; i < OPTION_ITEM_COUNT; ++i) {
		const float y = OPTION_TOP + i * OPTION_GAP;
		Row(i < TUNING_ITEM_COUNT ? OPTION_NAMES[i] : i == OPTION_ITEM_ARM_COSTS ? "腕の耐久の減る量"
			: i == OPTION_ITEM_RESET ? "標準に戻す" : "戻る", "", i, scene.GetCursor() == i, y, 41);
		if (i == OPTION_ITEM_ARM_COSTS) Text("表を開く >", 458, y + 9, 20, scene.GetCursor() == i ? CYAN : WHITE);
		if (i >= TUNING_ITEM_COUNT) continue;
		const auto item = static_cast<TUNING_ITEM>(i);
		char value[80];
		if (item == TUNING_ARM_USES && TuningCalcIsUnlimitedUses(tuning)) sprintf_s(value, "無制限");
		else if (item == TUNING_HAMMER_STUN) sprintf_s(value, "%.1f 秒", TuningCalcValue(tuning, item));
		else if (item == TUNING_CAPSULES) sprintf_s(value, "%d 個 / %.1f 秒", TuningCalcCapsuleCount(tuning), TuningCalcCapsuleSeconds(tuning));
		else sprintf_s(value, "× %.2f", TuningCalcValue(tuning, item));
		Text(value, 458, y + 9, 20, scene.GetCursor() == i ? CYAN : WHITE);
		if (scene.GetCursor() == i) Text("<", 428, y + 9, 20, CYAN);
	}
	const int selected = scene.GetCursor();
	if (selected < TUNING_ITEM_COUNT) {
		Detail("PARAMETER", OPTION_NAMES[selected], OPTION_HELP[selected][0], OPTION_HELP[selected][1], OPTION_HELP[selected][2]);
		const int levels = TuningCalcLevelCount(static_cast<TUNING_ITEM>(selected));
		for (int level = 0; level < levels; ++level) {
			Number2DDrawBox({ RIGHT + 28 + level * 60.0f, 481, 48, 5 }, level <= tuning.Levels[selected] ? CYAN : LINE);
		}
	}
	else if (selected == OPTION_ITEM_ARM_COSTS) Detail("ARM DURABILITY", "腕の耐久の減る量",
		"攻撃ごとに耐久が減る量の表です。", "Enter / ○ で表を開きます。", "オンラインははじめの値を使います。");
	else Detail("SETTINGS", selected == OPTION_ITEM_RESET ? "標準設定に戻す" : "設定を閉じる",
		selected == OPTION_ITEM_RESET ? "すべての項目と表を標準へ戻します。" : "変更した値は、そのまま保持されます。",
		"Enter / ○ で決定します。", "ファイルへの保存は行いません。");
	Text("変更は次の試合から適用", RIGHT, 560, 23, GOLD);
	Text("オンラインはホストの設定を全員に適用します。", RIGHT, 598, 17, MUTED);
	Footer("↑↓ / W S  選択    ←→ / A D  変更    Enter / ○  決定    Backspace / ×  戻る");
}

void CodexMenu::DrawArmCosts(const SceneCalc& scene, const ARM_COST_TABLE& costs) const
{
	Header("腕の耐久の減る量", "OPTIONS / ARM DURABILITY", 0);
	int selectedRow = -1, selectedItem = -1;
	ArmCostCalcUsedCell(scene.GetCursor(), selectedRow, selectedItem);
	const float cellLeft = LEFT + COST_LABEL_WIDTH;
	for (int item = 0; item < ARM_COST_ITEM_COUNT; ++item) {
		Text(COST_COLUMNS[item], cellLeft + item * COST_CELL_WIDTH + 4, 150, 13, item == selectedItem ? CYAN : MUTED);
	}
	for (int row = 0; row < ARM_COST_ROW_COUNT; ++row) {
		const float y = COST_TOP + row * COST_GAP;
		const bool rowSelected = row == selectedRow;
		Number2DDrawBox({ LEFT, y, ROW_WIDTH, COST_GAP - 6 }, rowSelected ? XMFLOAT4{ 0.055f, 0.20f, 0.25f, 1 } : PANEL);
		Number2DDrawBox({ LEFT, y, rowSelected ? 4.0f : 1.0f, COST_GAP - 6 }, rowSelected ? CYAN : LINE);
		Text(COST_ROWS[row], LEFT + 16, y + 9, 17, rowSelected ? WHITE : MUTED);
		for (int item = 0; item < ARM_COST_ITEM_COUNT; ++item) {
			const float x = cellLeft + item * COST_CELL_WIDTH;
			const bool selected = rowSelected && item == selectedItem;
			if (selected) Number2DDrawBox({ x, y + 4, COST_CELL_WIDTH - 8, COST_GAP - 14 }, { 0.10f, 0.36f, 0.42f, 1 });
			char value[16];
			if (ArmCostCalcIsUsed(row, item)) sprintf_s(value, "%d", costs.Values[row][item]);
			else sprintf_s(value, "-"); // 使わない欄（文字画像に「―」がないので半角）
			Text(value, x + 22, y + 8, 18, selected ? CYAN : ArmCostCalcIsUsed(row, item) ? WHITE : LINE);
		}
	}
	const int back = ArmCostCalcUsedCount();
	Row("戻る", "", ARM_COST_ROW_COUNT, scene.GetCursor() == back, COST_TOP + ARM_COST_ROW_COUNT * COST_GAP + 4, 41);
	if (selectedRow >= 0) {
		char line1[64];
		sprintf_s(line1, "%s：%d（はじめは %d）", COST_COLUMNS[selectedItem], costs.Values[selectedRow][selectedItem],
			ArmCostCalcDefault().Values[selectedRow][selectedItem]);
		Detail("ARM DURABILITY", COST_ROWS[selectedRow], line1, COST_NOTES[selectedRow][0], COST_NOTES[selectedRow][1]);
	}
	else Detail("SETTINGS", "表を閉じる", "変更した値は、そのまま保持されます。", "Enter / ○ でオプションへ戻ります。", "");
	Text("当たり：1人でも効いた　外れ：誰にも効かなかった", RIGHT, 560, 17, MUTED);
	Text("耐久は 100。オンラインははじめの値を使います。", RIGHT, 590, 17, MUTED);
	Footer("↑↓ / W S  選択    ←→ / A D  5 ずつ変更    Backspace / ×  戻る");
}

void CodexMenu::DrawLobby(const SceneCalc& scene, const VersusCalc& participants, const NetSession& net,
	const std::array<CHARACTER_TYPE, FIGHTER_COUNT>& characters) const
{
	const bool online = net.IsOnline();
	Header("キャラクターを選択", online ? "CHARACTER SELECT / ONLINE LOBBY" : "CHARACTER SELECT", 3);
	if (net.IsHost()) {
		// 相手に伝えるアドレスは、カードの上の1行に収める。
		const std::string ip = "HOST IP  " + net.GetAddresses();
		Text(ip.c_str(), RIGHT, LOBBY_IP_Y, 14, CYAN);
	}
	const double elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - m_PreviewStart).count();
	const float yaw = static_cast<float>(std::fmod(elapsed, PREVIEW_TURN_SECONDS)) * (2.0f * PI / PREVIEW_TURN_SECONDS);
	int joinedCount = 0;
	int readyCount = 0;
	for (int slot = 0; slot < FIGHTER_COUNT; ++slot) {
		const float x = LEFT + slot * (LOBBY_CARD_WIDTH + LOBBY_CARD_GAP);
		const bool joined = online ? (net.GetMask() & (1u << slot)) != 0 : participants.IsHuman(slot);
		const bool ready = online ? (net.GetReadyMask() & (1u << slot)) != 0 : participants.IsReady(slot);
		if (joined) ++joinedCount;
		if (joined && ready) ++readyCount;
		const XMFLOAT4 color = Fighter::GetPlayerColor(slot);
		Number2DDrawBox({ x, LOBBY_CARD_TOP, LOBBY_CARD_WIDTH, LOBBY_CARD_HEIGHT },
			ready ? XMFLOAT4{ 0.10f, 0.17f, 0.10f, 1 } : (joined ? XMFLOAT4{ 0.055f, 0.20f, 0.25f, 1 } : PANEL));
		Number2DDrawBox({ x, LOBBY_CARD_TOP, LOBBY_CARD_WIDTH, 5 }, joined ? color : LINE);
		char label[32];
		sprintf_s(label, "%dP", slot + 1);
		Text(label, x + 18, LOBBY_CARD_TOP + 16, 24, joined ? color : MUTED);
		const int device = participants.GetDevice(slot);
		const char* deviceName = !joined ? "" : online ? (slot == net.GetLocalSlot() ? "ONLINE / あなた" : "ONLINE")
			: device == ACTION_DEVICE_KEYBOARD ? "KEYBOARD" : "CONTROLLER";
		Text(deviceName, x + 70, LOBBY_CARD_TOP + 24, 14, MUTED);
		if (!joined) {
			Text("ダミー", x + 18, LOBBY_CARD_TOP + 60, 22, MUTED);
			if (!online && !scene.IsSolo()) Text("Enter / ○ で参加", x + 18, LOBBY_STATUS_Y, 16, WHITE);
			continue;
		}
		Text(CHARACTER_DATA::Get(characters[slot]).Name, x + 18, LOBBY_CARD_TOP + 60, 22, WHITE);
		DrawCharacterPreview(characters[slot], x, yaw);
		if (ready) {
			Number2DDrawBox({ x, LOBBY_STATUS_Y - 8, LOBBY_CARD_WIDTH, 40 }, GOLD);
			Text("準備OK", x + 18, LOBBY_STATUS_Y - 2, 20, INK);
		}
		else Text("←→ キャラ   ○ 準備OK", x + 18, LOBBY_STATUS_Y, 16, CYAN);
	}
	// 全員が準備OKなら開始の帯を出す（オンラインはホストのほかに1人以上つながっていること）。
	const bool allReady = readyCount > 0 && readyCount == joinedCount && (!online || joinedCount > 1);
	if (allReady) {
		Number2DDrawBox({ LEFT, LOBBY_BANNER_Y, 1152, 44 }, GOLD);
		Text(online ? "READY TO FIGHT    ホストが Enter / ○ で開始" : "READY TO FIGHT    1P が Enter / ○ で開始",
			LEFT + 24, LOBBY_BANNER_Y + 7, 22, INK);
	}
	else {
		Text(online ? "全員が ○ で準備OKになると、ホストが始められます。" : "参加した全員が準備OKになると、1P が始められます。",
			LEFT, LOBBY_BANNER_Y + 10, 17, MUTED);
		const GAME_RULE rule = online ? net.GetRule() : scene.GetRule();
		Text(rule == GAME_RULE_GAME ? "RULE / 通常の対戦" : "RULE / 腕を自由に試す", 1010, LOBBY_BANNER_Y + 10, 17, GOLD);
	}
	Footer(online ? "←→  キャラ    Enter / ○  準備OK・ホストが開始    ×  対戦方法へ戻る"
		: "←→  キャラ    Enter / ○  参加・準備OK・1P が開始    Backspace / ×  取り消し・戻る");
}

void CodexMenu::DrawResult(const SceneCalc& scene, const VersusCalc& participants) const
{
	Header("対戦結果", "RESULT / KNOCKOUT RANKING", 0);
	Text("順位", LEFT + 20, 165, 18, MUTED);
	Text("プレイヤー", LEFT + 180, 165, 18, MUTED);
	Text("KO", LEFT + 470, 165, 18, MUTED);
	for (int order = 0; order < participants.GetHumanCount(); ++order) {
		const int slot = participants.GetRankedSlot(order);
		const float y = 206 + order * 58.0f;
		Number2DDrawBox({ LEFT, y, ROW_WIDTH, 50 }, PANEL);
		char score[80];
		sprintf_s(score, "%02d", participants.GetRank(slot));
		Text(score, LEFT + 20, y + 9, 23, order == 0 ? GOLD : WHITE);
		sprintf_s(score, "%dP", slot + 1);
		Text(score, LEFT + 180, y + 9, 23, WHITE);
		sprintf_s(score, "%d", participants.GetScore(slot));
		Text(score, LEFT + 470, y + 9, 23, WHITE);
	}
	Row("もう一度遊ぶ", "RETRY / 同じ参加者・キャラで再戦", 0, scene.GetCursor() == RESULT_ITEM_RETRY, RESULT_ROW_TOP, RESULT_ROW_HEIGHT);
	Row("キャラ選択へ", "CHARACTER / 同じ参加者でキャラを選び直す", 1, scene.GetCursor() == RESULT_ITEM_CHARACTER,
		RESULT_ROW_TOP + RESULT_ROW_GAP, RESULT_ROW_HEIGHT);
	Row("メニューへ戻る", "TITLE / オンラインはロビーへ", 2, scene.GetCursor() == RESULT_ITEM_TITLE,
		RESULT_ROW_TOP + RESULT_ROW_GAP * 2, RESULT_ROW_HEIGHT);
	Reactor(961, 350, 143);
	Footer("1P : ↑↓ / W S  選択    Enter / ○  決定");
}

void CodexMenu::Draw(const SceneCalc& scene, const VersusCalc& participants, const NetSession& net,
	const SessionAddressCalc& address, const std::array<CHARACTER_TYPE, FIGHTER_COUNT>& characters,
	const MATCH_TUNING& tuning, const ARM_COST_TABLE& costs) const
{
	Background();
	const SCENE current = scene.GetScene();
	if (current == SCENE_OPTIONS) { DrawOptions(scene, tuning); return; }
	if (current == SCENE_ARM_COSTS) { DrawArmCosts(scene, costs); return; }
	if (current == SCENE_TITLE) { DrawLobby(scene, participants, net, characters); return; }
	if (current == SCENE_RESULT) { DrawResult(scene, participants); return; }
	if (current == SCENE_GAME_START) {
		Text("PICK UP. POWER UP. KNOCK OUT.", LEFT, 139, 18, CYAN);
		Text("ブラキオンファイト", LEFT - 3, 181, 54, WHITE);
		Text("変な腕で、ぶっ飛ばせ。", LEFT, 257, 31, WHITE);
		Text("腕を拾って装着。相手を場外へ飛ばす、3 分間の乱闘。", LEFT, 318, 19, MUTED);
		Row("ゲームを始める", "PLAY / 対戦方法 → ルール → キャラクター", 0, scene.GetCursor() == 0, 398, 76);
		Row("オプション", "OPTIONS / 次の試合の遊び心地を調整", 1, scene.GetCursor() == 1, 488, 76);
		Text("MOVE  WASD    JUMP  Space    ARMS  左右クリック / L1 R1", LEFT, 597, 16, MUTED);
		Reactor(962, 342, 179);
		Footer("↑↓ / W S / 左スティック  選択    Enter / ○  決定");
		return;
	}
	if (current == SCENE_RULE_SELECT) {
		Header("遊び方を選択", "RULE SELECT", 2);
		Row("通常の対戦", "GAME / 腕を拾って戦う・耐久あり", 0, scene.GetCursor() == GAME_RULE_GAME, 236, 87);
		Row("腕を自由に試す", "DEBUG / 装備を切り替えて練習", 1, scene.GetCursor() == GAME_RULE_DEBUG, 340, 87);
		const bool normal = scene.GetCursor() == GAME_RULE_GAME;
		Detail(normal ? "GAME" : "DEBUG", normal ? "腕を拾って、ぶっ飛ばす" : "お気に入りの腕を探そう",
			normal ? "素手でスタート。カプセルで腕を装着。" : "好きな腕を自由に切り替えられます。",
			normal ? "耐久がなくなったら、新しい腕を探そう。" : "数字キー / 十字キーで装備を変更。",
			normal ? "3 分間で、いちばん多く KO を取ろう。" : "耐久が 0 になっても腕を使えます。");
		Footer("↑↓ / W S / 左スティック  選択    Enter / ○  決定    Backspace / ×  戻る");
	}
	else if (current == SCENE_MODE_SELECT) {
		Header("対戦方法を選択", "MODE SELECT", 1);
		for (int i = 0; i < MODE_ITEM_COUNT; ++i) Row(MODE_NAMES[i], MODE_SUBTITLES[i], i, scene.GetCursor() == i, 168 + i * 75.0f);
		const int cursor = scene.GetCursor();
		Detail("HOW TO PLAY", MODE_NAMES[cursor], MODE_HELP[cursor][0], MODE_HELP[cursor][1], MODE_HELP[cursor][2]);
		Text("次にルールを選び、キャラ選択へ進みます。", RIGHT, 557, 18, GOLD);
		Text("設定を変えるには「オプション」へ。", RIGHT, 592, 18, MUTED);
		Footer("↑↓ / W S / 左スティック  選択    Enter / ○  決定    Backspace / ×  戻る");
	}
	else if (current == SCENE_JOIN_ADDRESS) {
		Header("オンラインに参加", "ONLINE JOIN / HOST ADDRESS", 0);
		Text("ホストの IPv4 アドレス", LEFT, 218, 24, WHITE);
		for (int part = 0; part < 4; ++part) {
			const float x = LEFT + part * 146.0f;
			const bool selected = part == address.GetCursor();
			Number2DDrawBox({ x, 276, 125, 80 }, PANEL);
			Number2DDrawBox({ x, 354, 125, 2 }, selected ? CYAN : LINE);
			Text(address.GetPart(part).c_str(), x + 14, 295, 36, selected ? CYAN : WHITE);
			if (part < 3) Text(".", x + 130, 307, 24, MUTED);
		}
		Text(address.IsValid() ? "Enter / ○ で接続する" : "各区切りに 0 ～ 255 を入力してください", LEFT, 393, 23, address.IsValid() ? CYAN : GOLD);
		Detail("CONNECTION GUIDE", "ホストに IP を聞こう",
			"キーボードで数字とピリオドを入力。", "パッドは左右で区切り、上下で数値変更。", "離れた相手とは Tailscale で接続します。");
		Text("Backspace は削除 / 空欄から戻る", LEFT, 475, 19, MUTED);
		Footer("←→  区切りを選択    ↑↓  数値変更    Enter / ○  接続    ×  戻る");
	}
}
