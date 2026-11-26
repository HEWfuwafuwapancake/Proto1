#pragma once

/// @file scenecalc.h
/// @brief 起動・ルールとモードの選択・IP 入力・ロビー・試合・結果の流れ（DirectX・Windows 不使用）

#include <cstdint>
#include "physics/armcostcalc.h"
#include "physics/mapcalc.h"
#include "physics/tuningcalc.h"

/// @brief キャラ選択の画面で使うキャラ番号
enum CHARACTER_TYPE
{
	CHARACTER_MEKA, ///< メカ
	CHARACTER_MEKAGIRL, ///< MekaGirl
	CHARACTER_COUNT ///< キャラの数
};

/// @brief 最初に選ばれているキャラ（ダミーもこのキャラで描く）
static constexpr CHARACTER_TYPE CHARACTER_DEFAULT = CHARACTER_MEKAGIRL;

/// @brief 試合のルール
enum GAME_RULE : uint8_t
{
	GAME_RULE_GAME, ///< 通常の対戦
	GAME_RULE_DEBUG, ///< 腕を自由に試せるデバッグ対戦
	GAME_RULE_COUNT ///< ルールの数
};

/// @brief ゲームのシーン
enum SCENE
{
	SCENE_TITLE, ///< キャラ選択（参加・キャラ・準備OK。オンラインのロビーも兼ねる）
	SCENE_MATCH, ///< 試合（時間切れの画面も含む）
	SCENE_RESULT, ///< 結果
	SCENE_GAME_START, ///< 起動時の GAME START
	SCENE_MODE_SELECT, ///< 対戦方法の選択
	SCENE_JOIN_ADDRESS, ///< 参加先の IPv4 入力
	SCENE_OPTIONS, ///< 次の試合の設定
	SCENE_ARM_COSTS, ///< オプションの中の、腕の耐久の減る量の表
	SCENE_PAUSE, ///< 試合を止めて開くメニュー（再開・ロビーへ・タイトルへ。オフラインだけ）
	SCENE_MAP_SELECT, ///< キャラ選択のあと、1P（オンラインはホスト）が試合のマップを選ぶ
};

/// @brief シーンの更新後にゲーム側で行う処理
enum SCENE_EVENT
{
	SCENE_EVENT_NONE, ///< 何もしない
	SCENE_EVENT_START_MATCH, ///< 試合を最初から始める
	SCENE_EVENT_SHOW_RESULT, ///< 今の得点で結果を表示する
	SCENE_EVENT_BACK_TITLE, ///< 初期配置に戻してタイトルを表示する
	SCENE_EVENT_BACK_LOBBY, ///< 参加者を保ったまま準備OKを消し、キャラ選択を表示する
	SCENE_EVENT_BACK_MENU, ///< 通信と参加を閉じて対戦方法の選択へ戻す
	SCENE_EVENT_HOST, ///< ホストを開く（成功したら OpenLobby を呼ぶ）
	SCENE_EVENT_JOIN, ///< 入力したアドレスへ接続する（成功したら OpenLobby を呼ぶ）
	SCENE_EVENT_TUNING_CHANGE, ///< 選択した設定を左右の入力方向へ変更する
	SCENE_EVENT_TUNING_RESET, ///< 全設定と耐久の減る量の表をはじめの値へ戻す
	SCENE_EVENT_ARM_COST_CHANGE, ///< 選択した耐久の減る量を左右の入力方向へ変更する
	SCENE_EVENT_LOOK_SENSITIVITY_CHANGE, ///< 視点感度を左右の入力方向へ変更する
	SCENE_EVENT_OPEN_MAP_SELECT, ///< キャラ選択で決まった参加者で体を作り、マップ選択を表示する
};

/// @brief モード選択の項目
enum MODE_ITEM
{
	MODE_ITEM_SOLO, ///< 決定した機器だけでキャラ選択へ進む
	MODE_ITEM_LOCAL, ///< この PC で参加を募るキャラ選択へ進む
	MODE_ITEM_HOST, ///< オンラインのホストを開いてキャラ選択（ロビー）へ進む
	MODE_ITEM_JOIN, ///< 参加先の IP を入力する
	MODE_ITEM_OPTIONS, ///< 次の試合の設定を開く
	MODE_ITEM_BACK, ///< GAME START へ戻る
	MODE_ITEM_COUNT, ///< 項目数
};

/// @brief オプションの画面で、試合の設定（0〜TUNING_ITEM_COUNT − 1）の後ろに並ぶ項目
enum OPTION_ITEM
{
	OPTION_ITEM_LOOK_SENSITIVITY = TUNING_ITEM_COUNT, ///< 視点感度（この PC だけの設定。通信では送らない）
	OPTION_ITEM_ARM_COSTS, ///< 耐久の減る量の表を開く
	OPTION_ITEM_RESET, ///< 全設定と表をはじめの値へ戻す
	OPTION_ITEM_BACK, ///< 元のメニューへ戻る
	OPTION_ITEM_COUNT, ///< 項目数
};

/// @brief 結果画面のメニュー項目
enum RESULT_ITEM
{
	RESULT_ITEM_RETRY, ///< もう一回
	RESULT_ITEM_CHARACTER, ///< キャラ選択へ
	RESULT_ITEM_TITLE, ///< タイトルへ
	RESULT_ITEM_COUNT, ///< 項目数（選択する項目ではない）
};

/// @brief 試合中のメニュー（ポーズ）の項目
enum PAUSE_ITEM
{
	PAUSE_ITEM_RESUME, ///< 試合に戻る
	PAUSE_ITEM_LOBBY, ///< 参加者とキャラを残してキャラ選択へ戻る
	PAUSE_ITEM_TITLE, ///< タイトル（GAME START）へ戻る
	PAUSE_ITEM_COUNT, ///< 項目数（選択する項目ではない）
};

/// @brief 耐久の減る量の表に一度に見せる行の数（調整用の値。これより行が多いと上下にスクロールする）
static constexpr int ARM_COST_VISIBLE_ROWS = 8;

/// @brief 選んでいる行が見える範囲に入るように、スクロールの先頭の行を求める
/// @details 見えている行を選んでいる間は動かさず、範囲の外へ出たときだけ、その行が端に来るまで動かす
/// @param top 今の先頭の行
/// @param selected 選んでいる行（行を選んでいないときは -1）
/// @param visible 一度に見せる行の数
/// @param count 行の数
/// @return 新しい先頭の行（0 〜 count − visible。行が visible 以下なら 0）
int SceneCalcScrollTop(int top, int selected, int visible, int count);

/// @brief シーンに渡す1フレーム分の入力
struct SCENE_INPUT
{
	bool Decide; ///< 決定を押した瞬間なら true
	bool Restart; ///< やり直しを押した瞬間なら true
	float MoveY; ///< 左スティック・W / S の上下（上がプラス）
	bool Back = false; ///< 戻るを押した瞬間なら true
	bool AddressValid = false; ///< IP の全区切りが入力済みで 0〜255 なら true
	int CharacterDirection = 0; ///< キャラの切り替え（左 -1、右 +1、なし 0）
	bool Pause = false; ///< 試合中のメニューを開く・閉じるボタンを押した瞬間なら true（オフラインだけ。オンラインは Restart を使う）
	float MoveX = 0.0f; ///< 左スティック・A / D の左右（右がプラス。マップ選択のカーソルに使う）
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
	/// @brief キャラ選択の画面で試合のルールを切り替える（GAME と DEBUG を順に回す）
	/// @param direction 切り替える向き（左 -1、右 +1、なし 0）
	void CycleRule(int direction);
	/// @brief モード選択・オプション・結果画面で選んでいる項目の番号を取得する
	/// （耐久の減る量の表では ArmCostCalcUsedCell の番号。最後の ArmCostCalcUsedCount() は「戻る」）
	int GetCursor() const { return m_Cursor; }
	/// @brief 耐久の減る量の表で、いちばん上に見せる行（ARM_COST_ROW）を取得する
	int GetCostScrollTop() const { return m_CostScrollTop; }
	/// @brief キャラ選択に、決定した機器のほかは参加できないか取得する（true：SOLO、false：LOCAL VS・オンライン）
	bool IsSolo() const { return !m_Online && m_LobbyMode == MODE_ITEM_SOLO; }
	/// @brief キャラ選択で選んでいる試合のルールを取得する
	GAME_RULE GetRule() const { return m_Rule; }
	/// @brief 左右の入力でキャラを一周させる（左 -1、右 +1、なし 0）
	static CHARACTER_TYPE CycleCharacter(CHARACTER_TYPE character, int direction);
	/// @brief 画面の移り方が「前の画面へ戻った」か調べる（戻るの音を、押したボタンでなく結果で選ぶため）
	/// @details オプションの「戻る」やモード選択の「タイトルに戻る」を決定で選んだときも、戻るボタンと同じく true
	/// @param from 移る前の画面
	/// @param to 移ったあとの画面
	/// @return メニューの階層を上へ戻る移り方なら true
	static bool IsBackTransition(SCENE from, SCENE to);
	/// @brief マップ選択で選んでいるマップ（もう一度・試合中は、その試合のマップ）を取得する
	STAGE_MAP GetMap() const { return m_Map; }

private:
	void MoveCursor(float moveY, int count);
	STAGE_MAP m_Map = STAGE_MAP_DEFAULT; // マップ選択で選んだマップ
	bool m_MapReady = true; // 左右の弾きを次に受け付けるか（一度スティックを戻すまで次へ動かさない）
	int m_LobbyMode = MODE_ITEM_SOLO; // 選んだ対戦方法（SOLO・LOCAL VS・HOST。キャラ選択から戻るときの行）
	GAME_RULE m_Rule = GAME_RULE_GAME;
	SCENE m_Scene = SCENE_GAME_START;
	SCENE m_OptionsReturn = SCENE_GAME_START;
	int m_Cursor = RESULT_ITEM_RETRY;
	int m_CostScrollTop = 0; // 耐久の減る量の表のスクロール（いちばん上に見せる行）
	bool m_MenuReady = true;
	bool m_Online = false;
};
