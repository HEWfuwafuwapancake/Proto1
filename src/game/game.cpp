#include "game/game.h"
#include "core/shader.h"
#include "input/action.h"

#ifdef _DEBUG
#include "core/debug_ostream.h"
#include <iostream>
#endif

void Game::Initialize()
{
	// 相手への参照は先に渡せるが、Reset の中では初期化済みの相手だけを使う。
	m_Stage.Initialize();
	m_Player.Initialize(m_Stage, m_Camera, m_Dummies, m_Arms);
	m_Dummies.Initialize(m_Stage, m_Player, m_Camera);
	m_Camera.Initialize(m_Stage, m_Player, m_Dummies, m_Arms, m_Match);
	m_Arms.Initialize(m_Stage, m_Player, m_Dummies, m_Camera);
	m_Match.Initialize(m_Dummies, m_Arms);
	m_Hud.Initialize(m_Camera, m_Dummies, m_Match);
}

void Game::Finalize()
{
	m_Hud.Finalize();
	m_Match.Finalize();
	m_Arms.Finalize();
	m_Camera.Finalize();
	m_Dummies.Finalize();
	m_Player.Finalize();
	m_Stage.Finalize();
}

void Game::Restart()
{
	// 敵を戻す前に腕を離し、プレイヤー・敵のあとにカメラを戻す。
	m_Arms.Reset();
	m_Player.Reset();
	m_Dummies.Reset();
	m_Camera.Reset();
	m_Match.Reset();
#ifdef _DEBUG
	hal::dout << "試合：やり直し" << std::endl;
#endif
}

void Game::Update()
{
	//プレイヤーを動かし、ダミーがプレイヤーに押されてから、カメラがプレイヤーを追いかける
	//（時間切れのあとは、プレイヤー・ダミー・触腕を止める。カメラは回せる）
	m_Stage.Update();
	if (m_Match.IsPlaying()) {
		m_Player.Update();
		m_Dummies.Update();
	}
	m_Camera.Update();
	if (m_Match.IsPlaying()) {
		m_Player.UpdateFacing();	// 腕の入力と同じフレームのカメラへ向いてから、肩の位置を計算する
	}
	// 装備切り替えは時間切れのあとも受け付け、腕の動きは試合中だけ更新する。
	m_Arms.Update(m_Match.IsPlaying());
	//試合の流れ（場外の回数を数える・時間切れ・やり直し）はダミーが動いたあとに呼ぶ
	if (ActionIsTrigger(ACTION_RESTART)) Restart();
	else m_Match.Update();
	m_Hud.Update();
}

void Game::Draw() const
{
	// ---- 3D描画 ----
	SetDepthEnable(true);
	m_Camera.Draw();

	// ライト（斜め上から当たる白い光）
	LIGHT light{};
	light.Enable = TRUE;
	light.Direction = { 0.5f, -1.0f, 0.8f, 0.0f };
	light.Diffuse = { 1.0f, 1.0f, 1.0f, 1.0f };
	light.Ambient = { 0.3f, 0.3f, 0.3f, 1.0f };
	Shader_SetLight(light);

	m_Stage.Draw();
	m_Player.Draw();
	m_Dummies.Draw();
	m_Arms.Draw();

	// ---- 2D描画（UI）：3Dの上に重ねるので最後に描く ----
	SetDepthEnable(false);
	light.Enable = FALSE;
	Shader_SetLight(light);

	m_Hud.Draw();
}
