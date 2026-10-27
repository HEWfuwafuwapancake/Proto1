#pragma once


class Camera;
class DummyManager;
class Match;

/// @file hud.h
/// @brief 画面表示（残り秒数・スコア・ダミーの頭の上のダメージ%・時間切れの画面）
/// @details 数字は 7 セグ、英字は 5×7 ドット（render/number2d.h）。置き場所・大きさ・色は hud.cpp の「調整用の値」。
///          時間切れでは画面を半透明の黒でおおい、TIME UP・スコア・PRESS R / OPTIONS を出す

/// @brief 試合とダミーの画面表示を管理する
class Hud
{
public:

	/// @brief 画面表示の初期化（今は何もしない。Number2DInitialize のあとに呼ぶ）
	void Initialize(const Camera& camera, const DummyManager& dummies, const Match& match);

	/// @brief 状態を最初に戻す（参照はそのまま）
	void Reset(void);

	/// @brief 画面表示の終了処理（今は何もしない）
	void Finalize(void);

	/// @brief 画面表示の更新（今は何もしない。値は描くときに読む）
	void Update(void);

	/// @brief 画面表示を描く（2D 描画の中で、深度テストを切りライトを消してから呼ぶ）
	void Draw(void) const;

private:
	void DrawDummyPercent(int index) const;
	const Camera* m_Camera = nullptr;
	const DummyManager* m_Dummies = nullptr;
	const Match* m_Match = nullptr;
};
