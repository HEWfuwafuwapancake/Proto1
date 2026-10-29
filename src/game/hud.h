#pragma once


class Camera;
class FighterManager;
class Match;
class SceneCalc;

/// @file hud.h
/// @brief シーンごとの画面表示（タイトル・試合の HUD・結果のメニュー）
/// @details 数字は 7 セグ、英字は 5×7 ドット（render/number2d.h）。置き場所・大きさ・色は hud.cpp の「調整用の値」。
///          タイトルと結果は半透明のおおいとメニュー、時間切れは TIME UP・スコア・結果への案内を出す

/// @brief シーン・試合・ダミーの状態から画面表示を管理する
class Hud
{
public:

	/// @brief カメラ・ダミー・試合・シーンへの参照を受け取る（Number2DInitialize のあとに呼ぶ）
	void Initialize(const Camera& camera, const FighterManager& fighters, int slot, const Match& match, const SceneCalc& scene);

	/// @brief 状態を最初に戻す（参照はそのまま）
	void Reset(void);

	/// @brief 画面表示の終了処理（今は何もしない）
	void Finalize(void);

	/// @brief 画面表示の更新（今は何もしない。値は描くときに読む）
	void Update(void);

	/// @brief 画面表示を描く（2D 描画の中で、深度テストを切りライトを消してから呼ぶ）
	void Draw(void) const;

private:
	void DrawFighterPercent(int index) const;
	void DrawMenuItem(const char* text, float y, bool selected) const;
	void DrawMenuHelp(float selectY, float decideY) const;
	const Camera* m_Camera = nullptr;
	const FighterManager* m_Fighters = nullptr;
	const Match* m_Match = nullptr;
	const SceneCalc* m_Scene = nullptr;
	int m_Slot = 0;
};
