#include "game/game.h"
#include "core/shader.h"
#include "input/action.h"
#include "input/pad.h"
#include "render/backdrop.h"
#include "render/number2d.h"
#include "render/text2d.h"
#include <chrono>
#include <cmath>

#ifdef _DEBUG
#include "core/debug_ostream.h"
#include <iostream>
#endif

// 調整用の値
static constexpr XMFLOAT4 EMPTY_VIEW_COLOR = { 0.0f, 0.0f, 0.0f, 1.0f };	// 3人のときの空いた画面（黒）
static constexpr uint32_t MATCH_SEED_STEP = 0x9e3779b9u;	// やり直すたびに試合の種へ足す値
static constexpr uint32_t MATCH_ROUND_STEP = 0xc2b2ae35u;	// メニューやロビーから始め直すたびに種へ足す値
static constexpr uint32_t CAPSULE_SEED_MASK = 0x85ebca6bu;	// 復活先とは別の乱数列にする
static constexpr int ARM_COST_STEP = 5;	// オプションで耐久の減る量を左右1回で変える量
// Ｇで動いている間も残す入力（メニュー・カメラの操作）。ほかの移動・攻撃・ガードなどは使わない。
static constexpr uint32_t LIFT_KEPT_BUTTONS = (1u << ACTION_RESTART) | (1u << ACTION_RUMBLE_TOGGLE) | (1u << ACTION_DECIDE)
	| (1u << ACTION_CAMERA_TOGGLE) | (1u << ACTION_TARGET_LEFT) | (1u << ACTION_TARGET_RIGHT);

VERSUS_VIEWPORT Game::GetViewport(int index) const
{
	return VersusCalcViewport(m_Net.IsOnline() ? 1 : m_HumanCount, index, (float)SCREEN_WIDTH, (float)SCREEN_HEIGHT);
}

bool Game::Initialize(const NET_OPTIONS& options)
{
	m_OfflineSeed = static_cast<uint32_t>(std::chrono::steady_clock::now().time_since_epoch().count());
	// 相手への参照は先に渡せるが、Reset の中では初期化済みの相手だけを使う。
	m_Stage.Initialize();
	m_Participants.Reset();
	for (auto& input : m_Inputs) input = {};
	for (auto& input : m_PreviousInputs) input = {};
	for (auto& input : m_DeviceInputs) input = {};
	m_LobbyCharacters.fill(CHARACTER_DEFAULT);
	m_Fighters.LoadModels();
	m_Fighters.Initialize(m_Stage, m_Participants);
	m_ArmCapsule.Initialize(m_Stage);
	// 腕のモデルは起動時に4枠分だけ読む。参加やTITLE・RETRYでは作り直さない。
	for (int i = 0; i < FIGHTER_COUNT; ++i) {
		m_Cameras[i].Initialize(m_Stage, m_Fighters, i, m_Arms[i], m_Match);
		m_Arms[i].Initialize(m_Stage, m_Fighters, i, m_Cameras[i]);
	}
	m_Match.Initialize(m_Fighters, m_Arms);
	m_Gimmick.Initialize(m_Stage, m_Fighters, m_Arms);
	for (int i = 0; i < FIGHTER_COUNT; ++i) {
		m_Huds[i].Initialize(m_Cameras[i], m_Fighters, i, m_Match, m_Scene, m_Arms);
	}
	ConfigurePlayers();
	m_CodexMenu.Initialize();
	const bool connected = m_Net.Initialize(options);
	if (connected && m_Net.IsHost()) m_Net.SetTuning(m_Tuning);
	m_Scene.Start(m_Net.IsOnline());
	ActionSetOnlineEquip(m_Net.IsOnline());
	UpdatePadLights();
#ifdef _DEBUG
	hal::dout << "シーン：タイトル（人 " << m_HumanCount << " 人）" << std::endl;
#endif
	return connected;
}

void Game::Finalize()
{
	m_CodexMenu.Finalize();
	m_Net.Finalize();
	Text2DFinalize();
	m_Match.Finalize();
	for (int i = 0; i < FIGHTER_COUNT; ++i) {
		m_Arms[i].Finalize();
		m_Cameras[i].Finalize();
	}
	m_ArmCapsule.Finalize();
	m_Fighters.Finalize();
	m_Stage.Finalize();
}

void Game::ConfigurePlayers()
{
	// 前の顔ぶれの保持をすべて離してから、新しい体へ割り当てる。
	for (int i = 0; i < FIGHTER_COUNT; ++i) m_Arms[i].Reset();
	// 人のいない枠のうち CPU にした枠は、CPU の機器番号で体を作る（オンラインはダミーだけ）。
	VersusCalc roster = m_Participants;
	if (!m_Net.IsOnline()) {
		for (int slot = 0; slot < FIGHTER_COUNT; ++slot) {
			if (m_Participants.IsCpu(slot)) roster.SetSlotDevice(slot, VERSUS_DEVICE_CPU);
		}
	}
	m_Fighters.Initialize(m_Stage, roster);
	m_HumanCount = 0;
	m_FightingCount = 0;
	for (int i = 0; i < FIGHTER_COUNT; ++i) {
		Fighter& fighter = m_Fighters.GetFighter(i);
		fighter.SetCharacter(m_LobbyCharacters[i]); // ダミー・CPU もキャラ選択で選んだキャラで描く
		fighter.SetInput(m_Inputs[i], m_PreviousInputs[i]);
		if (fighter.IsHuman()) m_Fighting[m_FightingCount++] = i;
		if (fighter.IsHuman() && !fighter.IsCpu()) m_Humans[m_HumanCount++] = i;
	}
	// 腕とカメラは人と CPU の両方に持たせる。CPU のカメラは画面に出さず、ロックオンの相手を選ぶのに使う。
	for (int k = 0; k < m_FightingCount; ++k) {
		const int slot = m_Fighting[k];
		m_Fighters.BindPlayer(slot, m_Cameras[slot], m_Arms[slot]);
		m_Cameras[slot].SetViewSize((float)SCREEN_WIDTH, (float)SCREEN_HEIGHT);
		m_Cameras[slot].Reset();
	}
	for (int k = 0; k < m_HumanCount; ++k) {
		const int slot = m_Humans[k];
		const VERSUS_VIEWPORT view = GetViewport(k);
		m_Cameras[slot].SetViewSize(view.Width, view.Height);
		m_Cameras[slot].Reset();
		m_Huds[slot].SetViewSize(view.Width, view.Height);
	}
	if (m_HumanCount == 0) {
		// 未参加のタイトルは1つのカメラで初期配置を映す。
		m_Cameras[0].SetViewSize((float)SCREEN_WIDTH, (float)SCREEN_HEIGHT);
		m_Cameras[0].Reset();
	}
}

void Game::UpdatePadLights() const
{
	for (int pad = 0; pad < PAD_COUNT; ++pad) {
		if (m_Net.IsOnline() && pad != m_Net.GetLocalDevice()) continue;
		XMFLOAT4 color{}; // 未参加は消灯
		for (int slot = 0; slot < FIGHTER_COUNT; ++slot) {
			if (m_Participants.GetDevice(slot) == pad) color = Fighter::GetPlayerColor(slot);
		}
		PadSetLightColor(pad, color.x, color.y, color.z);
	}
}

void Game::Restart(bool retry)
{
	// 試合中に引いた乱数の回数にかかわらず、やり直しでは全員が同じ種を使う。
	const GAME_RULE rule = GetRule();
	if (!retry && !m_Net.IsOnline()) m_OfflineSeed += MATCH_ROUND_STEP;
	m_MatchSeed = retry ? m_MatchSeed + MATCH_SEED_STEP : GetSeed();
	m_Match.SetRule(rule);
	const MATCH_TUNING& tuning = GetTuning();
	// マップはマップ選択で 1P（オンラインはホスト）が選んだもの。もう一度でも同じマップを使う。
	m_Stage.SetMap(m_Scene.GetMap(), TuningCalcValue(tuning, TUNING_MAP_SIZE));
	m_Fighters.SetMatchSettings(rule, m_MatchSeed, tuning);
	m_ArmCapsule.SetAmount(TuningCalcCapsuleCount(tuning), TuningCalcCapsuleSeconds(tuning));

	// 敵を戻す前に腕を離し、プレイヤー・敵のあとにカメラを戻す。
	for (int k = 0; k < m_FightingCount; ++k) m_Arms[m_Fighting[k]].ResetForMatch(rule, tuning);
	m_Fighters.Reset();
	m_Gimmick.Reset();
	if (rule == GAME_RULE_GAME) m_ArmCapsule.Reset(m_MatchSeed ^ CAPSULE_SEED_MASK);
	for (int k = 0; k < m_FightingCount; ++k) m_Cameras[m_Fighting[k]].Reset();
	for (int k = 0; k < m_FightingCount; ++k) {
		const int slot = m_Fighting[k];
		if (m_Fighters.GetFighter(slot).IsCpu()) m_Cpus[slot].Reset(m_MatchSeed, slot, m_Participants.GetCpuLevel(slot));
	}
	m_Match.Reset();
#ifdef _DEBUG
	hal::dout << "試合：やり直し" << std::endl;
#endif
}

GAME_RULE Game::GetRule() const
{
	return m_Net.IsOnline() ? m_Net.GetRule() : m_Scene.GetRule();
}

const MATCH_TUNING& Game::GetTuning() const
{
	return m_Net.IsOnline() ? m_Net.GetTuning() : m_Tuning;
}

bool Game::CanEditTuning() const
{
	return !m_Net.IsOnline() || (m_Net.IsHost() && !m_Net.IsRunning() && m_Scene.GetScene() != SCENE_MATCH);
}

void Game::SetTuning(const MATCH_TUNING& tuning)
{
	if (!CanEditTuning() || !TuningCalcIsValid(tuning)) return;
	if (m_Net.IsOnline()) m_Net.SetTuning(tuning);
	else m_Tuning = tuning;
}

uint32_t Game::GetSeed() const
{
	return m_Net.IsOnline() ? m_Net.GetSeed() + m_Net.GetRound() * MATCH_ROUND_STEP : m_OfflineSeed;
}

void Game::ResetParticipants()
{
	m_Participants.Reset();
	m_LobbyCharacters.fill(CHARACTER_DEFAULT);
	for (auto& value : m_Inputs) value = {};
	for (auto& value : m_PreviousInputs) value = {};
	for (int device = 0; device < ACTION_DEVICE_COUNT; ++device) m_DeviceInputs[device] = ActionGetPlayerInput(device);
	ConfigurePlayers();
	m_Match.Reset();
	UpdatePadLights();
}

bool Game::IsLobbyBack() const
{
	for (int device = 0; device < ACTION_DEVICE_COUNT; ++device) {
		if (ActionGetMenuInput(device).Back) return true;
	}
	return false;
}

void Game::UpdateMenu()
{
	const SCENE oldScene = m_Scene.GetScene();
	SCENE_INPUT input{};
	for (int device = 0; device < ACTION_DEVICE_COUNT; ++device) {
		const ACTION_MENU_INPUT menu = ActionGetMenuInput(device);
		// メニューからロビーへ入った決定を、参加の再入力として使わない。
		m_DeviceInputs[device] = ActionGetPlayerInput(device);
		if (menu.Decide && !input.Decide) { input.Decide = true; m_MenuDevice = device; }
		if (fabsf(menu.MoveY) > fabsf(input.MoveY)) input.MoveY = menu.MoveY;
		if (input.CharacterDirection == 0) input.CharacterDirection = menu.CharacterDirection;
		if (oldScene == SCENE_JOIN_ADDRESS) {
			m_JoinAddress.Move(menu.Horizontal);
			m_JoinAddress.Adjust(menu.Vertical);
			for (char c : menu.Text) {
				if (c >= '0' && c <= '9') m_JoinAddress.AppendDigit(c - '0');
				else if (c == '.') m_JoinAddress.Move(1);
				else if (c == '\b' && !m_JoinAddress.Erase()) input.Back = true;
			}
			// キーボードの Backspace は上で削除。パッドの×はいつでも戻る。
			if (device != ACTION_DEVICE_KEYBOARD) input.Back = input.Back || menu.Back;
		}
		else input.Back = input.Back || menu.Back;
	}
	input.AddressValid = m_JoinAddress.IsValid();
	const SCENE_EVENT event = m_Scene.Step(input, false);
	if (event == SCENE_EVENT_TUNING_RESET) {
		SetTuning(TuningCalcDefault());
		m_ArmCosts = ArmCostCalcDefault();
		ActionSetLookSensitivityLevel(ActionGetLookSensitivityDefaultLevel());
	}
	else if (event == SCENE_EVENT_LOOK_SENSITIVITY_CHANGE) {
		ActionSetLookSensitivityLevel(ActionGetLookSensitivityLevel() + input.CharacterDirection);
	}
	else if (event == SCENE_EVENT_ARM_COST_CHANGE) {
		// オプションはオフラインのメニューからしか開かないので、表はいつでも書き換えてよい。
		int row = 0, item = 0;
		if (ArmCostCalcUsedCell(m_Scene.GetCursor(), row, item)) {
			ArmCostCalcSet(m_ArmCosts, row, item, m_ArmCosts.Values[row][item] + input.CharacterDirection * ARM_COST_STEP);
		}
	}
	else if (event == SCENE_EVENT_TUNING_CHANGE) {
		MATCH_TUNING tuning = GetTuning();
		const auto item = static_cast<TUNING_ITEM>(m_Scene.GetCursor());
		const int next = tuning.Levels[item] + input.CharacterDirection;
		if (next >= 0 && next < TuningCalcLevelCount(item)) {
			tuning.Levels[item] = static_cast<uint8_t>(next);
			SetTuning(tuning);
		}
	}
	if (oldScene != SCENE_JOIN_ADDRESS && m_Scene.GetScene() == SCENE_JOIN_ADDRESS) {
		const std::string addresses = NetSocket::GetLocalAddresses();
		m_JoinAddress.Initialize(addresses.substr(0, addresses.find(" / ")));
	}
	if (event == SCENE_EVENT_HOST || event == SCENE_EVENT_JOIN) {
		NET_OPTIONS options;
		options.Mode = event == SCENE_EVENT_HOST ? NET_MODE_HOST : NET_MODE_JOIN;
		options.Host = m_JoinAddress.GetText();
		options.Delay = LOCKSTEP_AUTO_DELAY;
		// キャラはロビー（キャラ選択）で選ぶので、はじめのキャラで接続する。
		if (m_Net.Initialize(options, CHARACTER_DEFAULT, m_Scene.GetRule())) {
			if (m_Net.IsHost()) m_Net.SetTuning(m_Tuning);
			m_Scene.OpenLobby();
		}
		else m_Scene.ShowModes(); // 理由は NetSession がタイトルバーへ残す。
		ActionSetOnlineEquip(m_Net.IsOnline());
		// オフラインで替えた装備が残ると、相手の画面と腕がずれて DESYNC になる。
		// フリーカメラの切り替えも同じなので、ロックオンにそろえる。
		for (int i = 0; i < FIGHTER_COUNT; ++i) {
			m_Arms[i].ResetEquip();
			m_Cameras[i].ResetFreeLook();
		}
		ResetParticipants();
	}
	else if (oldScene == SCENE_MODE_SELECT && m_Scene.GetScene() == SCENE_TITLE) {
		// 対戦方法を決めた機器を 1P にしてキャラ選択へ入る（準備OKはまだ）。
		m_Participants.Reset();
		m_LobbyCharacters.fill(CHARACTER_DEFAULT);
		m_Participants.AssignDevice(m_MenuDevice);
		UpdatePadLights();
	}
}

void Game::Update()
{
	// オンラインは全員が同じ減り方になるよう、はじめの表を使う。
	const ARM_COST_TABLE costs = m_Net.IsOnline() ? ArmCostCalcDefault() : m_ArmCosts;
	for (auto& arms : m_Arms) arms.SetCosts(costs);
	// 前のフレームで変わった画面・カーソルを覚え、選んだボタンを弾ませる（1フレーム遅れても見た目は変わらない）。
	m_CodexMenu.Update(m_Scene);
	const SCENE oldScene = m_Scene.GetScene();
	if (oldScene == SCENE_GAME_START || oldScene == SCENE_MODE_SELECT || oldScene == SCENE_JOIN_ADDRESS || oldScene == SCENE_OPTIONS || oldScene == SCENE_ARM_COSTS) {
		UpdateMenu();
		return;
	}
	// オフラインのキャラ選択の戻るは、機器ごとに下で扱う（準備OKの取り消し・参加をやめる・1P は前の画面へ）。
	if (oldScene == SCENE_TITLE && m_Net.IsOnline() && IsLobbyBack()) {
		m_Scene.Step({ false, false, 0.0f, true }, false);
		m_Net.Finalize();
		ActionSetOnlineEquip(false);
		ResetParticipants();
		return;
	}
	bool start = false;
	int32_t onlineFrame = -1;
	if (m_Net.IsOnline()) {
		m_Net.Update(); // 入力待ちでも受信・再送・機器選択を続ける。
		if (!m_Net.IsOnline()) {
			// ホスト切断・参加拒否では、リモート枠と前入力を捨ててモード選択へ戻す。
			ActionSetOnlineEquip(false);
			m_Scene.ShowModes();
			ResetParticipants();
			return;
		}
		if (oldScene == SCENE_TITLE) {
			for (int slot = 0; slot < FIGHTER_COUNT; ++slot) {
				int device = VERSUS_DEVICE_DUMMY;
				if (m_Net.GetReadyMask() & (1u << slot)) {
					device = slot == m_Net.GetLocalSlot() ? m_Net.GetLocalDevice() : VERSUS_DEVICE_REMOTE;
				}
				m_Participants.SetSlotDevice(slot, device);
				m_LobbyCharacters[slot] = m_Net.GetCharacter(slot);
			}
		}
		if (oldScene == SCENE_TITLE) {
			// 機器を選ぶ前は入力のあった機器、選んだあとはその機器で操作する。
			ACTION_MENU_INPUT menu{};
			for (int device = 0; device < ACTION_DEVICE_COUNT; ++device) {
				if (m_Net.GetLocalDevice() >= 0 && m_Net.GetLocalDevice() != device) continue;
				const ACTION_MENU_INPUT candidate = ActionGetMenuInput(device);
				if (candidate.MoveX != 0.0f || candidate.MoveY != 0.0f || candidate.CharacterDirection != 0) { menu = candidate; break; }
			}
			// ホストは左スティックの上下でルールの行と自分のカードを行き来し、ルールの行では十字キーの左右でルールを変える。
			if (m_Net.IsHost()) m_Participants.MoveLobbyFocus(menu.MoveX, menu.MoveY, false);
			if (m_Net.IsHost() && m_Participants.GetLobbyFocus() == VERSUS_LOBBY_FOCUS_RULE) m_Net.CycleRule(menu.CharacterDirection);
			else m_Net.CycleLocalCharacter(menu.CharacterDirection);
		}
		UpdatePadLights();
		LOCKSTEP_FRAME frame;
		if (!m_Net.TakeFrame(frame)) return; // ゲームの計算・前入力・カメラは進めない。
		onlineFrame = frame.Number;
		start = oldScene == SCENE_TITLE;
		if (start) {
			for (auto& value : m_Inputs) value = {};
		}
		for (int slot = 0; slot < FIGHTER_COUNT; ++slot) {
			m_PreviousInputs[slot] = m_Inputs[slot];
			m_Inputs[slot] = frame.Inputs[slot];
			m_Fighters.GetFighter(slot).SetInput(m_Inputs[slot], m_PreviousInputs[slot]);
		}
	}
	else {
		// キャラ選択：決定で参加 → 準備OK → 全員が準備OKなら 1P で開始。戻るで準備OKの取り消し → 参加をやめる。
		// 1P は左スティック（上でルールの行、左右で横に並んだカード）で選び、十字キーでルール・ダミーや CPU のキャラと強さも変える。
		// 同一フレームの参加は機器番号順。
		bool back = false;
		for (int device = 0; device < ACTION_DEVICE_COUNT; ++device) {
			const PLAYER_INPUT current = ActionGetPlayerInput(device);
			if (oldScene == SCENE_TITLE) {
				const ACTION_MENU_INPUT menu = ActionGetMenuInput(device);
				const bool decide = ActionCalcPlayerTrigger(current, m_DeviceInputs[device], ACTION_DECIDE);
				const int slot = m_Participants.FindSlot(device);
				int focus = slot; // 1P 以外は自分のカードだけを操作する
				if (slot == 0) {
					m_Participants.MoveLobbyFocus(menu.MoveX, menu.MoveY, true);
					focus = m_Participants.GetLobbyFocus();
				}
				if (focus == VERSUS_LOBBY_FOCUS_RULE) m_Scene.CycleRule(menu.CharacterDirection);
				else if (focus > 0 && focus != slot) {
					// 1P が人のいない枠を選んでいる：十字キーの左右でキャラ、上下でダミー ⇔ CPU よわい ⇔ ふつう ⇔ つよい を切り替える。
					// キーボードには十字キーの上下がないので、決定（Enter）でも強いほうへ回す。
					m_LobbyCharacters[focus] = SceneCalc::CycleCharacter(m_LobbyCharacters[focus], menu.CharacterDirection);
					if (menu.Vertical != 0) m_Participants.CycleCpu(focus, menu.Vertical);
					else if (decide) m_Participants.CycleCpu(focus, 1);
				}
				else if (slot >= 0 && !m_Participants.IsReady(slot)) {
					// 準備OKにした人は、取り消すまでキャラを変えない。
					m_LobbyCharacters[slot] = SceneCalc::CycleCharacter(m_LobbyCharacters[slot], menu.CharacterDirection);
				}
				if (decide && (focus == slot || slot < 0)) {
					if (m_Participants.LobbyDecide(device, !m_Scene.IsSolo()) == VERSUS_LOBBY_DECIDE_START) start = true;
				}
				else if (!decide && menu.Back && m_Participants.LobbyCancel(device) == VERSUS_LOBBY_CANCEL_BACK) back = true;
			}
			m_DeviceInputs[device] = current;
		}
		if (back && !start) {
			m_Scene.Step({ false, false, 0.0f, true }, false);
			ResetParticipants();
			return;
		}
		for (int slot = 0; slot < FIGHTER_COUNT; ++slot) {
			m_PreviousInputs[slot] = m_Inputs[slot];
			const int device = m_Participants.GetDevice(slot);
			m_Inputs[slot] = device >= 0 && device < ACTION_DEVICE_COUNT ? m_DeviceInputs[device] : ActionGetPlayerInput(device);
			// F1〜F4は機器ではなく人のスロットを選ぶ。パッドの人にも従来どおり配る。
			m_Inputs[slot].Equip = m_Participants.IsHuman(slot) ? ActionGetEquipKey(slot) : ACTION_EQUIP_KEY{ -1, -1 };
			if (m_Fighters.GetFighter(slot).IsCpu()) {
				// CPU は試合中だけ、前のフレームの状況から人と同じ形の入力を作る。
				m_Inputs[slot] = oldScene == SCENE_MATCH && m_Match.IsPlaying()
					? m_Cpus[slot].Update(m_Stage, m_Fighters, m_Arms[slot], m_Cameras[slot], m_ArmCapsule, GetRule()) : PLAYER_INPUT{};
			}
			m_Fighters.GetFighter(slot).SetInput(m_Inputs[slot], m_PreviousInputs[slot]);
		}
	}
	SCENE_INPUT input{ ActionCalcPlayerTrigger(m_Inputs[0], m_PreviousInputs[0], ACTION_DECIDE),
		ActionCalcPlayerTrigger(m_Inputs[0], m_PreviousInputs[0], ACTION_RESTART), ActionCalcPlayerMove(m_Inputs[0]).Y };
	if (oldScene == SCENE_TITLE) input = { start, false, 0.0f };
	if (oldScene == SCENE_MAP_SELECT) {
		// マップは 1P（オンラインはホスト）が、全員の PC に届いた 1P の入力で選ぶ（オンラインでも全員同じに進む）。
		input.Restart = false;
		input.MoveX = ActionCalcPlayerMove(m_Inputs[0]).X;
		// キャラ選択の開始に押した決定は、1P の入力に1フレーム遅れて届くことがある。一度離すまでマップを決めない。
		if (m_MapDecideHeld) {
			if (!ActionCalcPlayerPress(m_Inputs[0], ACTION_DECIDE)) m_MapDecideHeld = false;
			input.Decide = false;
		}
		// 戻る（キャラ選択へ）はオフラインだけ。1P の機器の Backspace / ×。
		if (!m_Net.IsOnline()) input.Back = ActionGetMenuInput(m_Participants.GetDevice(0)).Back;
	}
	if (!m_Net.IsOnline() && (oldScene == SCENE_MATCH || oldScene == SCENE_PAUSE)) {
		// オフラインの OPTIONS（キーボードは R）は、試合を止めるメニューを開く・閉じる。参加しているだれが押してもよい。
		// やり直しは時間切れのあとだけ（オンラインは全員の入力をそろえる必要があるので、今までどおり 1P のやり直し）。
		bool pause = false;
		for (int k = 0; k < m_HumanCount; ++k) {
			const int slot = m_Humans[k];
			if (ActionCalcPlayerTrigger(m_Inputs[slot], m_PreviousInputs[slot], ACTION_RESTART)) pause = true;
		}
		const bool finished = m_Match.GetState() == MATCH_STATE_FINISHED;
		input.Restart = input.Restart && finished;
		input.Pause = pause;
		if (oldScene == SCENE_PAUSE) {
			// メニューの中は、どの機器の上下・決定・戻るでも動かせる。
			input.Decide = false;
			input.MoveY = 0.0f;
			for (int device = 0; device < ACTION_DEVICE_COUNT; ++device) {
				const ACTION_MENU_INPUT menu = ActionGetMenuInput(device);
				if (menu.Decide) input.Decide = true;
				if (menu.Back) input.Back = true;
				if (fabsf(menu.MoveY) > fabsf(input.MoveY)) input.MoveY = menu.MoveY;
			}
		}
	}
	if (oldScene == SCENE_MATCH) {
		// 全員の体・体当たり → カプセル → カメラ → キャラの向き → 腕の順を保つ。
		m_Stage.Update();
		if (m_Match.IsPlaying()) {
			// Ｇで動いている人（今受け付けた人を含む）は、このフレームの移動・ジャンプ・攻撃などの入力を使わない。
			const int lifting = m_Gimmick.PreUpdate();
			for (int slot = 0; slot < FIGHTER_COUNT; ++slot) {
				if ((lifting & (1 << slot)) == 0) continue;
				m_Inputs[slot].MoveX = 0;
				m_Inputs[slot].MoveY = 0;
				m_Inputs[slot].Buttons &= LIFT_KEPT_BUTTONS;
				m_Fighters.GetFighter(slot).SetInput(m_Inputs[slot], m_PreviousInputs[slot]);
			}
			m_Fighters.Update();
			m_Gimmick.PostUpdate();
			if (GetRule() == GAME_RULE_GAME) {
				// 復活した地点で拾った腕まで、あとから外さないようにする。
				for (int k = 0; k < m_FightingCount; ++k) {
					const int slot = m_Fighting[k];
					if (m_Fighters.GetFighter(slot).DidRespawn()) m_Arms[slot].ResetForMatch(GAME_RULE_GAME, m_Arms[slot].GetTuning());
				}
				m_ArmCapsule.Update(m_Fighters, m_Arms);
			}
		}
		for (int k = 0; k < m_FightingCount; ++k) m_Cameras[m_Fighting[k]].Update();
		if (m_Match.IsPlaying()) m_Fighters.UpdateFacing();
		// DEBUG の装備切り替えは時間切れのあとも受け付ける。
		for (int k = 0; k < m_FightingCount; ++k) m_Arms[m_Fighting[k]].Update(m_Match.IsPlaying());
		if (m_Match.IsPlaying()) m_Fighters.UpdateAnimation();
	}
	const SCENE_EVENT event = m_Scene.Step(input, m_Match.GetState() == MATCH_STATE_FINISHED);
	if (event == SCENE_EVENT_OPEN_MAP_SELECT) {
		// キャラ選択で決まった参加者で体を作り、選んでいるマップを背景に映す。
		ConfigurePlayers();
		m_MapDecideHeld = true;
		m_Stage.SetMap(m_Scene.GetMap(), TuningCalcValue(GetTuning(), TUNING_MAP_SIZE));
	}
	else if (event == SCENE_EVENT_START_MATCH) {
		// マップ選択から始めるときは新しい試合、試合中・結果からはもう一度（同じ種の続き）。
		Restart(oldScene != SCENE_MAP_SELECT);
	}
	else if (event == SCENE_EVENT_BACK_LOBBY) {
		// 参加者とキャラはそのまま、準備OKだけ消してキャラ選択へ戻る。
		if (m_Net.IsOnline()) m_Net.ReturnToLobby();
		else m_Participants.ClearReady();
		ConfigurePlayers();
		m_Match.Reset();
	}
	else if (event == SCENE_EVENT_BACK_TITLE) {
		if (m_Net.IsOnline()) {
			m_Net.ReturnToLobby();
			ConfigurePlayers();
			m_Match.Reset();
		}
		else ResetParticipants();
	}
	else if (oldScene == SCENE_MATCH) m_Match.Update();
	// マップ選択では、選んでいるマップを背景のぼかした3Dに映す。
	if (m_Scene.GetScene() == SCENE_MAP_SELECT && m_Stage.GetMap() != m_Scene.GetMap()) {
		m_Stage.SetMap(m_Scene.GetMap(), TuningCalcValue(GetTuning(), TUNING_MAP_SIZE));
	}
#ifdef _DEBUG
	if (oldScene != m_Scene.GetScene()) {
		const SCENE scene = m_Scene.GetScene();
		const char* name = scene == SCENE_TITLE ? "ロビー" : scene == SCENE_MATCH ? "試合"
			: scene == SCENE_RESULT ? "結果" : scene == SCENE_PAUSE ? "試合中のメニュー" : "メニュー";
		hal::dout << "シーン：" << name << std::endl;
	}
#endif
	for (int k = 0; k < m_HumanCount; ++k) m_Huds[m_Humans[k]].Update();
	if (onlineFrame >= 0 && (onlineFrame + 1) % LOCKSTEP_CHECKSUM_INTERVAL == 0) {
		LOCKSTEP_STATE states[FIGHTER_COUNT];
		for (int slot = 0; slot < FIGHTER_COUNT; ++slot) {
			const Fighter& fighter = m_Fighters.GetFighter(slot);
			const XMFLOAT3 position = fighter.GetPosition();
			const ArmManager& arms = m_Arms[slot];
			states[slot] = { { position.x, position.y, position.z }, fighter.GetPercent(), m_Fighters.GetScore(slot),
				{ ARM_TYPE_NONE, ARM_TYPE_NONE }, { 0, 0 } };
			if (fighter.IsHuman()) {
				for (int hand = 0; hand < 2; ++hand) {
					states[slot].ArmTypes[hand] = arms.GetType(hand);
					states[slot].ArmUses[hand] = arms.GetDurability(hand);
				}
			}
		}
		m_Net.RecordChecksum(onlineFrame, states);
	}
	UpdatePadLights();
}

void Game::Draw() const
{
	// ライト（斜め上から当たる白い光）
	LIGHT light{};
	light.Diffuse = { 1.0f, 1.0f, 1.0f, 1.0f };
	light.Ambient = { 0.3f, 0.3f, 0.3f, 1.0f };
	light.Direction = { 0.5f, -1.0f, 0.8f, 0.0f };
	const SCENE scene = m_Scene.GetScene();
	if (scene != SCENE_MATCH && scene != SCENE_PAUSE) {
		// メニューの背景：ステージのまわりをゆっくり回るカメラで3Dを小さく描き、ぼかしておく（貼るのは CodexMenu::Draw）。
		if (BackdropBegin()) {
			m_CodexMenu.SetBackdropCamera(m_Stage.GetStage());
			light.Enable = TRUE;
			Shader_SetLight(light);
			m_Stage.Draw();
			if (scene == SCENE_MAP_SELECT) m_Gimmick.Draw(-1);
			else {
				// マップ選択では体が前の試合の位置に残っているので、地形だけを見せる。
				m_Fighters.Draw(-1);
				for (int j = 0; j < m_FightingCount; ++j) m_Arms[m_Fighting[j]].Draw();
			}
			BackdropEnd();
		}
		// メニューの UI は、ぼかした背景の上に2Dで描く。
		SetViewport(0.0f, 0.0f, (float)SCREEN_WIDTH, (float)SCREEN_HEIGHT);
		SetDepthEnable(false);
		light.Enable = FALSE;
		Shader_SetLight(light);
		m_CodexMenu.Draw(m_Scene, scene == SCENE_RESULT ? m_Fighters.GetVersus() : m_Participants,
			m_Net, m_JoinAddress, m_LobbyCharacters, GetTuning(), m_ArmCosts);
		// 接続の問題はウィンドウ表示中にも読めるよう、メニューの上へ出す。
		m_Huds[0].DrawCornerWarning(m_Net);
		return;
	}
	const bool drawCapsules = GetRule() == GAME_RULE_GAME && (scene == SCENE_MATCH || scene == SCENE_RESULT);

	// 人ごとに「ビューポート → そのカメラ → 3D → その人の HUD」の順に描く。
	const int viewCount = m_Net.IsOnline() ? 1 : (m_HumanCount > 0 ? m_HumanCount : 1);
	for (int k = 0; k < viewCount; ++k) {
		const int slot = m_Net.IsOnline() && m_HumanCount > 0 ? m_Net.GetLocalSlot() : (m_HumanCount > 0 ? m_Humans[k] : 0);
		const Camera& camera = m_Cameras[slot];
		const VERSUS_VIEWPORT view = GetViewport(k);
		SetViewport(view.X, view.Y, view.Width, view.Height);

		// ---- 3D描画 ----
		SetDepthEnable(true);
		camera.Draw();
		light.Enable = TRUE;
		Shader_SetLight(light);

		m_Stage.Draw();
		m_Gimmick.Draw(m_HumanCount > 0 ? slot : -1);
		if (drawCapsules) m_ArmCapsule.Draw();
		m_Fighters.Draw(camera.GetTarget());
		for (int j = 0; j < m_FightingCount; ++j) m_Arms[m_Fighting[j]].Draw();

		// ---- 2D描画（UI）：3Dの上に重ねるので最後に描く ----
		SetDepthEnable(false);
		light.Enable = FALSE;
		Shader_SetLight(light);

		m_Huds[slot].Draw();
	}

	// ---- 画面全体：3人のときの空き → タイトル・結果のおおい ----
	SetViewport(0.0f, 0.0f, (float)SCREEN_WIDTH, (float)SCREEN_HEIGHT);
	if (!m_Net.IsOnline() && m_HumanCount == 3) {
		const VERSUS_VIEWPORT empty = GetViewport(3);
		Number2DDrawBox({ empty.X, empty.Y, empty.Width, empty.Height }, EMPTY_VIEW_COLOR);
	}
	m_Huds[0].DrawScene(m_Participants, m_Net, m_JoinAddress, m_LobbyCharacters);
	// 試合中のメニューは、止まった試合の画面の上に重ねる。
	if (scene == SCENE_PAUSE) m_CodexMenu.DrawPause(m_Scene);
	if (m_Fullscreen) m_Huds[0].DrawCornerWarning(m_Net);
}
