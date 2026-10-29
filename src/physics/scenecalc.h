#pragma once

/// @file scenecalc.h
/// @brief タイトル・試合・結果の流れとメニューの選択（DirectX・Windows 不使用）

/// @brief ゲームのシーン
enum SCENE
{
	SCENE_TITLE, ///< タイトル
	SCENE_MATCH, ///< 試合（時間切れの画面も含む）
	SCENE_RESULT, ///< 結果
};

/// @brief シーンの更新後にゲーム側で行う処理
enum SCENE_EVENT
{
	SCENE_EVENT_NONE, ///< 何もしない
	SCENE_EVENT_START_MATCH, ///< 試合を最初から始める
	SCENE_EVENT_SHOW_RESULT, ///< 今の得点で結果を表示する
	SCENE_EVENT_BACK_TITLE, ///< 初期配置に戻してタイトルを表示する
};

/// @brief 結果画面のメニュー項目
enum RESULT_ITEM
{
	RESULT_ITEM_RETRY, ///< もう一回
	RESULT_ITEM_TITLE, ///< タイトルへ
	RESULT_ITEM_COUNT, ///< 項目数（選択する項目ではない）
};

/// @brief シーンに渡す1フレーム分の入力
struct SCENE_INPUT
{
	bool Decide; ///< 決定を押した瞬間なら true
	bool Restart; ///< やり直しを押した瞬間なら true
	float MoveY; ///< 左スティック・W / S の上下（上がプラス）
};

/// @brief シーンの遷移と、弾くたびに1つ動くメニューを管理する
class SceneCalc
{
public:
	/// @brief タイトルから始め、選択とスティックの記憶を最初に戻す
	void Start();
	/// @brief 入力からシーンとメニューを1フレーム進める
	/// @param input このフレームの入力
	/// @param matchFinished 時間切れか（true：時間切れ、false：試合中）
	/// @return ゲーム側で行う処理
	SCENE_EVENT Step(const SCENE_INPUT& input, bool matchFinished);
	/// @brief 今のシーンを取得する
	SCENE GetScene() const { return m_Scene; }
	/// @brief 結果画面で選んでいる項目の番号を取得する
	int GetCursor() const { return m_Cursor; }

private:
	SCENE m_Scene = SCENE_TITLE;
	int m_Cursor = RESULT_ITEM_RETRY;
	bool m_MenuReady = true;
};
