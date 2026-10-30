#include "game/game.h"
#include "core/shader.h"
#include "input/action.h"
#include "input/pad.h"
#include "render/number2d.h"

#ifdef _DEBUG
#include "core/debug_ostream.h"
#include <iostream>
#endif

// 調整用の値
static constexpr XMFLOAT4 EMPTY_VIEW_COLOR = { 0.0f, 0.0f, 0.0f, 1.0f };	// 3人のときの空いた画面（黒）

VERSUS_VIEWPORT Game::GetViewport(int index) const
{
	return VersusCalcViewport(m_Net.IsOnline() ? 1 : m_HumanCount, index, (float)SCREEN_WIDTH, (float)SCREEN_HEIGHT);
}

bool Game::Initialize(const NET_OPTIONS& options)
{
	// 相手への参照は先に渡せるが、Reset の中では初期化済みの相手だけを使う。
	m_Stage.Initialize();
	m_Participants.Reset();
	for (auto& input : m_Inputs) input = {};
	for (auto& input : m_PreviousInputs) input = {};
	for (auto& input : m_DeviceInputs) input = {};
	m_Fighters.Initialize(m_Stage, m_Participants);
	// モデルは起動時に4枠分だけ読む。参加やTITLE・RETRYでは作り直さない。
	for (int i = 0; i < FIGHTER_COUNT; ++i) {
		m_Cameras[i].Initialize(m_Stage, m_Fighters, i, m_Arms[i], m_Match);
		m_Arms[i].Initialize(m_Stage, m_Fighters, i, m_Cameras[i]);
	}
	m_Match.Initialize(m_Fighters, m_Arms);
	for (int i = 0; i < FIGHTER_COUNT; ++i) {
		m_Huds[i].Initialize(m_Cameras[i], m_Fighters, i, m_Match, m_Scene);
	}
	ConfigurePlayers();
	m_Scene.Start();
	const bool connected = m_Net.Initialize(options);
	ActionSetOnlineEquip(m_Net.IsOnline());
	UpdatePadLights();
#ifdef _DEBUG
	hal::dout << "シーン：タイトル（人 " << m_HumanCount << " 人）" << std::endl;
#endif
	return connected;
}

void Game::Finalize()
{
	m_Net.Finalize();
	for (int i = 0; i < FIGHTER_COUNT; ++i) m_Huds[i].Finalize();
	m_Match.Finalize();
	for (int i = 0; i < FIGHTER_COUNT; ++i) {
		m_Arms[i].Finalize();
		m_Cameras[i].Finalize();
	}
	m_Stage.Finalize();
}

void Game::ConfigurePlayers()
{
	// 前の顔ぶれの保持をすべて離してから、新しい体へ割り当てる。
	for (int i = 0; i < FIGHTER_COUNT; ++i) m_Arms[i].Reset();
	m_Fighters.Initialize(m_Stage, m_Participants);
	m_HumanCount = 0;
	for (int i = 0; i < FIGHTER_COUNT; ++i) {
		m_Fighters.GetFighter(i).SetInput(m_Inputs[i], m_PreviousInputs[i]);
		if (m_Fighters.GetFighter(i).IsHuman()) m_Humans[m_HumanCount++] = i;
	}
	for (int k = 0; k < m_HumanCount; ++k) {
		const int slot = m_Humans[k];
		const VERSUS_VIEWPORT view = GetViewport(k);
		m_Fighters.BindPlayer(slot, m_Cameras[slot], m_Arms[slot]);
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

void Game::Restart()
{
	// 敵を戻す前に腕を離し、プレイヤー・敵のあとにカメラを戻す。
	for (int k = 0; k < m_HumanCount; ++k) m_Arms[m_Humans[k]].Reset();
	m_Fighters.Reset();
	for (int k = 0; k < m_HumanCount; ++k) m_Cameras[m_Humans[k]].Reset();
	m_Match.Reset();
#ifdef _DEBUG
	hal::dout << "試合：やり直し" << std::endl;
#endif
}

void Game::Update()
{
	const SCENE oldScene = m_Scene.GetScene();
	bool start = false;
	int32_t onlineFrame = -1;
	if (m_Net.IsOnline()) {
		m_Net.Update(); // 入力待ちでも受信・再送・機器選択を続ける。
		if (!m_Net.IsOnline()) {
			// ホスト切断・参加拒否では、リモート枠と前入力を捨ててローカルのタイトルへ戻す。
			ActionSetOnlineEquip(false);
			m_Scene.Start();
			m_Participants.Reset();
			for (auto& value : m_Inputs) value = {};
			for (auto& value : m_PreviousInputs) value = {};
			for (int device = 0; device < ACTION_DEVICE_COUNT; ++device) m_DeviceInputs[device] = ActionGetPlayerInput(device);
			ConfigurePlayers();
			m_Match.Reset();
			UpdatePadLights();
			return;
		}
		if (oldScene == SCENE_TITLE) {
			for (int slot = 0; slot < FIGHTER_COUNT; ++slot) {
				int device = VERSUS_DEVICE_DUMMY;
				if (m_Net.GetReadyMask() & (1u << slot)) {
					device = slot == m_Net.GetLocalSlot() ? m_Net.GetLocalDevice() : VERSUS_DEVICE_REMOTE;
				}
				m_Participants.SetSlotDevice(slot, device);
			}
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
		// 同一フレームの参加は機器番号順。開始フレームの参加も取り込む。
		for (int device = 0; device < ACTION_DEVICE_COUNT; ++device) {
			const PLAYER_INPUT current = ActionGetPlayerInput(device);
			if (oldScene == SCENE_TITLE && ActionCalcPlayerTrigger(current, m_DeviceInputs[device], ACTION_DECIDE)
				&& m_Participants.JoinOrStart(device)) start = true;
			m_DeviceInputs[device] = current;
		}
		for (int slot = 0; slot < FIGHTER_COUNT; ++slot) {
			m_PreviousInputs[slot] = m_Inputs[slot];
			const int device = m_Participants.GetDevice(slot);
			m_Inputs[slot] = device >= 0 && device < ACTION_DEVICE_COUNT ? m_DeviceInputs[device] : ActionGetPlayerInput(device);
			// F1〜F4は機器ではなく人のスロットを選ぶ。パッドの人にも従来どおり配る。
			m_Inputs[slot].Equip = m_Participants.IsHuman(slot) ? ActionGetEquipKey(slot) : ACTION_EQUIP_KEY{ -1, -1 };
			m_Fighters.GetFighter(slot).SetInput(m_Inputs[slot], m_PreviousInputs[slot]);
		}
	}
	SCENE_INPUT input{ ActionCalcPlayerTrigger(m_Inputs[0], m_PreviousInputs[0], ACTION_DECIDE),
		ActionCalcPlayerTrigger(m_Inputs[0], m_PreviousInputs[0], ACTION_RESTART), ActionCalcPlayerMove(m_Inputs[0]).Y };
	if (oldScene == SCENE_TITLE) input = { start, false, 0.0f };
	if (oldScene == SCENE_MATCH) {
		// 全員の体・体当たり → カメラ → キャラの向き → 腕の順を保つ。
		m_Stage.Update();
		if (m_Match.IsPlaying()) {
			m_Fighters.Update();
		}
		for (int k = 0; k < m_HumanCount; ++k) m_Cameras[m_Humans[k]].Update();
		if (m_Match.IsPlaying()) m_Fighters.UpdateFacing();
		// 装備切り替えは時間切れのあとも受け付ける。
		for (int k = 0; k < m_HumanCount; ++k) m_Arms[m_Humans[k]].Update(m_Match.IsPlaying());
	}
	const SCENE_EVENT event = m_Scene.Step(input, m_Match.GetState() == MATCH_STATE_FINISHED);
	if (event == SCENE_EVENT_START_MATCH) {
		if (oldScene == SCENE_TITLE) ConfigurePlayers();
		Restart();
	}
	else if (event == SCENE_EVENT_BACK_TITLE) {
		if (m_Net.IsOnline()) m_Net.ReturnToLobby();
		else m_Participants.Reset();
		ConfigurePlayers();
		m_Match.Reset();
	}
	else if (oldScene == SCENE_MATCH) m_Match.Update();
#ifdef _DEBUG
	if (oldScene != m_Scene.GetScene()) {
		const char* name = m_Scene.GetScene() == SCENE_TITLE ? "タイトル"
			: (m_Scene.GetScene() == SCENE_MATCH ? "試合" : "結果");
		hal::dout << "シーン：" << name << std::endl;
	}
#endif
	for (int k = 0; k < m_HumanCount; ++k) m_Huds[m_Humans[k]].Update();
	if (onlineFrame >= 0 && (onlineFrame + 1) % LOCKSTEP_CHECKSUM_INTERVAL == 0) {
		LOCKSTEP_STATE states[FIGHTER_COUNT];
		for (int slot = 0; slot < FIGHTER_COUNT; ++slot) {
			const Fighter& fighter = m_Fighters.GetFighter(slot);
			const XMFLOAT3 position = fighter.GetPosition();
			states[slot] = { { position.x, position.y, position.z }, fighter.GetPercent(), m_Fighters.GetScore(slot) };
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
		m_Fighters.Draw(camera.GetTarget());
		for (int j = 0; j < m_HumanCount; ++j) m_Arms[m_Humans[j]].Draw();

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
	m_Huds[0].DrawScene(m_Participants);
}
