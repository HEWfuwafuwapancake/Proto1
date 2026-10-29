#pragma once

#include "game/stage.h"
#include "game/fightermanager.h"
#include "game/camera.h"
#include "game/arm.h"
#include "game/match.h"
#include "game/hud.h"
#include "physics/scenecalc.h"

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
	/// @brief 3D のライト・物体と、その上に重なる画面表示を描く
	void Draw() const;
	/// @brief タイトルバーに表示する左右の装備を読む
	const ArmManager& GetArms() const { return m_Arms; }

private:
	void Restart();

	Stage m_Stage;
	FighterManager m_Fighters;
	Camera m_Camera;
	ArmManager m_Arms;
	Match m_Match;
	SceneCalc m_Scene;
	Hud m_Hud;
};
