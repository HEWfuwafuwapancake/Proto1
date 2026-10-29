#pragma once

#include "game/stage.h"
#include "game/fightermanager.h"
#include "game/camera.h"
#include "game/arm.h"
#include "game/match.h"
#include "game/hud.h"
#include "physics/scenecalc.h"
#include "input/action.h"

/// @brief ゲームの部品を所有し、初期化・更新・描画・やり直しの順番をまとめる
class Game
{
public:
	/// @brief 基盤の初期化後に、ステージから画面表示までを順に初期化する
	void Initialize();
	/// @brief 初期化と逆の順にゲームの部品を終了する
	void Finalize();
	/// @brief 入力の更新後にゲームを1フレーム進める
	void Update();
	/// @brief 人ごとの画面に 3D と HUD を描き、最後に画面全体のおおいを重ねる
	void Draw() const;
	/// @brief タイトルバーに表示する左右の装備を読む（ダミーのスロットは装備を変えられない）
	/// @param slot スロット番号（0〜3）
	const ArmManager& GetArms(int slot) const { return m_Arms[slot]; }

private:
	void Restart();
	void ConfigurePlayers();
	void UpdatePadLights() const;
	VERSUS_VIEWPORT GetViewport(int index) const;

	Stage m_Stage;
	VersusCalc m_Participants; // タイトルで決まる参加者。試合の得点はFighterManagerが持つ。
	FighterManager m_Fighters;
	// カメラ・腕・HUD はスロット番号で持つ。使うのは人のスロットだけ。
	Camera m_Cameras[FIGHTER_COUNT];
	ArmManager m_Arms[FIGHTER_COUNT];
	Hud m_Huds[FIGHTER_COUNT];
	int m_Humans[FIGHTER_COUNT] = {};	// 画面の順に並べた人のスロット番号
	int m_HumanCount = 0;
	PLAYER_INPUT m_Inputs[FIGHTER_COUNT];
	PLAYER_INPUT m_PreviousInputs[FIGHTER_COUNT];
	PLAYER_INPUT m_DeviceInputs[ACTION_DEVICE_COUNT]; // 参加前の決定にも使う
	Match m_Match;
	SceneCalc m_Scene;
};
