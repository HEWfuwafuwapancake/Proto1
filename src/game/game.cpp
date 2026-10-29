#include "game/game.h"
#include "core/shader.h"
#include "input/action.h"
#include "render/number2d.h"

#ifdef _DEBUG
#include "core/debug_ostream.h"
#include <iostream>
#endif

// 調整用の値
static constexpr XMFLOAT4 EMPTY_VIEW_COLOR = { 0.0f, 0.0f, 0.0f, 1.0f };	// 3人のときの空いた画面（黒）

VERSUS_VIEWPORT Game::GetViewport(int index) const
{
	return VersusCalcViewport(m_HumanCount, index, (float)SCREEN_WIDTH, (float)SCREEN_HEIGHT);
}

void Game::Initialize()
{
	// 相手への参照は先に渡せるが、Reset の中では初期化済みの相手だけを使う。
	m_Stage.Initialize();
	m_Fighters.Initialize(m_Stage);
	m_HumanCount = 0;
	for (int i = 0; i < FIGHTER_COUNT; ++i) {
		if (m_Fighters.GetFighter(i).IsHuman()) m_Humans[m_HumanCount++] = i;
	}
	for (int k = 0; k < m_HumanCount; ++k) {
		const int slot = m_Humans[k];
		const VERSUS_VIEWPORT view = GetViewport(k);
		m_Fighters.BindPlayer(slot, m_Cameras[slot], m_Arms[slot]);
		m_Cameras[slot].SetViewSize(view.Width, view.Height);
		m_Cameras[slot].Initialize(m_Stage, m_Fighters, slot, m_Arms[slot], m_Match);
		m_Arms[slot].Initialize(m_Stage, m_Fighters, slot, m_Cameras[slot]);
	}
	m_Match.Initialize(m_Fighters, m_Arms);
	for (int k = 0; k < m_HumanCount; ++k) {
		const int slot = m_Humans[k];
		const VERSUS_VIEWPORT view = GetViewport(k);
		m_Huds[slot].Initialize(m_Cameras[slot], m_Fighters, slot, m_Match, m_Scene);
		m_Huds[slot].SetViewSize(view.Width, view.Height);
	}
	m_Scene.Start();
#ifdef _DEBUG
	hal::dout << "シーン：タイトル（人 " << m_HumanCount << " 人）" << std::endl;
#endif
}

void Game::Finalize()
{
	for (int k = 0; k < m_HumanCount; ++k) m_Huds[m_Humans[k]].Finalize();
	m_Match.Finalize();
	for (int k = 0; k < m_HumanCount; ++k) {
		m_Arms[m_Humans[k]].Finalize();
		m_Cameras[m_Humans[k]].Finalize();
	}
	m_Stage.Finalize();
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
	const SCENE_INPUT input{ ActionIsTrigger(ACTION_DECIDE), ActionIsTrigger(ACTION_RESTART), ActionGetMove().Y };
	const SCENE oldScene = m_Scene.GetScene();
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
	if (event == SCENE_EVENT_START_MATCH || event == SCENE_EVENT_BACK_TITLE) Restart();
	else if (oldScene == SCENE_MATCH) m_Match.Update();
#ifdef _DEBUG
	if (oldScene != m_Scene.GetScene()) {
		const char* name = m_Scene.GetScene() == SCENE_TITLE ? "タイトル"
			: (m_Scene.GetScene() == SCENE_MATCH ? "試合" : "結果");
		hal::dout << "シーン：" << name << std::endl;
	}
#endif
	for (int k = 0; k < m_HumanCount; ++k) m_Huds[m_Humans[k]].Update();
}

void Game::Draw() const
{
	// ライト（斜め上から当たる白い光）
	LIGHT light{};
	light.Diffuse = { 1.0f, 1.0f, 1.0f, 1.0f };
	light.Ambient = { 0.3f, 0.3f, 0.3f, 1.0f };
	light.Direction = { 0.5f, -1.0f, 0.8f, 0.0f };

	// 人ごとに「ビューポート → そのカメラ → 3D → その人の HUD」の順に描く。
	for (int k = 0; k < m_HumanCount; ++k) {
		const int slot = m_Humans[k];
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
	if (m_HumanCount == 3) {
		const VERSUS_VIEWPORT empty = GetViewport(3);
		Number2DDrawBox({ empty.X, empty.Y, empty.Width, empty.Height }, EMPTY_VIEW_COLOR);
	}
	if (m_HumanCount > 0) m_Huds[m_Humans[0]].DrawScene();
}
