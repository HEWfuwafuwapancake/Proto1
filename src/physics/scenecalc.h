#pragma once

/// @file scenecalc.h
/// @brief 起動・モード選択・IP 入力・ロビー・試合・結果の流れ（DirectX・Windows 不使用）

/// @brief ゲームのシーン
enum SCENE
{
	SCENE_TITLE, ///< ロビー（参加画面）
	SCENE_MATCH, ///< 試合（時間切れの画面も含む）
	SCENE_RESULT, ///< 結果
	SCENE_GAME_START, ///< 起動時の GAME START
	SCENE_MODE_SELECT, ///< 対戦方法の選択
	SCENE_JOIN_ADDRESS, ///< 参加先の IPv4 入力
};

/// @brief シーンの更新後にゲーム側で行う処理
enum SCENE_EVENT
{
	SCENE_EVENT_NONE, ///< 何もしない
	SCENE_EVENT_START_MATCH, ///< 試合を最初から始める
	SCENE_EVENT_SHOW_RESULT, ///< 今の得点で結果を表示する
	SCENE_EVENT_BACK_TITLE, ///< 初期配置に戻してタイトルを表示する
	SCENE_EVENT_BACK_MENU, ///< 通信と参加を閉じてモード選択へ戻す
	SCENE_EVENT_HOST, ///< ホストを開く（成功したら OpenLobby を呼ぶ）
	SCENE_EVENT_JOIN, ///< 入力したアドレスへ接続する（成功したら OpenLobby を呼ぶ）
};

/// @brief モード選択の項目
enum MODE_ITEM
{
	MODE_ITEM_SOLO, ///< 決定した機器だけで試合を始める
	MODE_ITEM_LOCAL, ///< この PC で参加を募る
	MODE_ITEM_HOST, ///< オンラインのホストを開く
	MODE_ITEM_JOIN, ///< 参加先の IP を入力する
	MODE_ITEM_BACK, ///< GAME START へ戻る
	MODE_ITEM_COUNT, ///< 項目数
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
	bool Back = false; ///< 戻るを押した瞬間なら true
	bool AddressValid = false; ///< IP の全区切りが入力済みで 0〜255 なら true
};

/// @brief シーンの遷移と、弾くたびに1つ動くメニューを管理する
class SceneCalc
{
public:
	/// @brief 起動画面を設定する（true：オンラインのロビー、false：GAME START）
	void Start(bool online = false);
	/// @brief 接続成功後、オンラインのロビーを表示する
	void OpenLobby();
	/// @brief オフラインのモード選択へ戻す（切断・参加拒否にも使う）
	void ShowModes();
	/// @brief 入力からシーンとメニューを1フレーム進める
	/// @param input このフレームの入力
	/// @param matchFinished 時間切れか（true：時間切れ、false：試合中）
	/// @return ゲーム側で行う処理
	SCENE_EVENT Step(const SCENE_INPUT& input, bool matchFinished);
	/// @brief 今のシーンを取得する
	SCENE GetScene() const { return m_Scene; }
	/// @brief モード選択・結果画面で選んでいる項目の番号を取得する
	int GetCursor() const { return m_Cursor; }

private:
	void MoveCursor(float moveY, int count);
	SCENE m_Scene = SCENE_GAME_START;
	int m_Cursor = RESULT_ITEM_RETRY;
	bool m_MenuReady = true;
	bool m_Online = false;
};
