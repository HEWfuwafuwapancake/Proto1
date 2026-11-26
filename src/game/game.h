#pragma once

#include "game/stage.h"
#include "game/fightermanager.h"
#include "game/camera.h"
#include "game/arm.h"
#include "game/armcapsule.h"
#include "game/match.h"
#include "game/hud.h"
#include "game/mapgimmick.h"
#include "game/codex_menu.h"
#include "physics/scenecalc.h"
#include "physics/settingscalc.h"
#include "input/action.h"
#include "game/netsession.h"
#include "game/cpu.h"
#include "game/menuse.h"

/// @brief ゲームの部品を所有し、初期化・更新・描画・やり直しの順番をまとめる
class Game
{
public:
	/// @brief 基盤の初期化後に、ステージから画面表示・接続までを初期化する（接続準備の失敗時false）
	bool Initialize(const NET_OPTIONS& options = {});
	/// @brief 接続状態と、このPCが操作するスロット・機器を取得する
	const NetSession& GetNetSession() const { return m_Net; }
	/// @brief 初期化と逆の順にゲームの部品を終了する
	void Finalize();
	/// @brief 入力の更新後にゲームを1フレーム進める
	void Update();
	/// @brief フレームのすき間に通信を受け取り、そろった入力を中継する
	void PollNetwork() { m_Net.Poll(); }
	/// @brief 人ごとの画面に 3D と HUD を描き、最後に画面全体のおおいを重ねる
	void Draw() const;
	/// @brief 全画面の表示を設定する（true：オンラインの警告を画面左上にも出す、false：タイトルバーだけ）
	void SetFullscreen(bool fullscreen) { m_Fullscreen = fullscreen; }
	/// @brief タイトルバーに表示する左右の装備を読む（ダミーのスロットは装備を変えられない）
	/// @param slot スロット番号（0〜3）
	const ArmManager& GetArms(int slot) const { return m_Arms[slot]; }
	/// @brief 次の試合開始時に適用する設定を取得する
	const MATCH_TUNING& GetTuning() const;
	/// @brief 編集可能なときだけ次の試合の設定を変更する（範囲外は無視）
	void SetTuning(const MATCH_TUNING& tuning);
	/// @brief オフライン、または試合中でないオンラインのホストならtrueを返す
	bool CanEditTuning() const;

private:
	void UpdateFrame();
	MENU_SE_STATE GetMenuSeState() const;
	void UpdateSeListeners() const;
	void Restart(bool retry);
	void ResetParticipants();
	void UpdateMenu();
	bool IsLobbyBack() const;
	void ConfigurePlayers();
	void UpdatePadLights() const;
	VERSUS_VIEWPORT GetViewport(int index) const;
	GAME_RULE GetRule() const;
	uint32_t GetSeed() const;
	OPTION_SETTINGS GetOptionSettings() const; // 今のオプションの設定（試合の設定・耐久の表・視点感度・音量）
	void LoadOptionSettings(); // 起動時に設定のファイルを読んで使う（なければはじめの値のまま）
	bool WriteOptionSettings(const OPTION_SETTINGS& settings) const; // 設定をファイルに書く（書けなければ false）

	Stage m_Stage;
	MATCH_TUNING m_Tuning = TuningCalcDefault();
	ARM_COST_TABLE m_ArmCosts = ArmCostCalcDefault(); // オプションで変える、オフラインの減る量の表（オンラインははじめの表）。
	VersusCalc m_Participants; // タイトルで決まる参加者。試合の得点はFighterManagerが持つ。
	FighterManager m_Fighters;
	ArmCapsule m_ArmCapsule;
	MapGimmick m_Gimmick; // 対戦マップのＧ・ダッシュ板
	std::array<CHARACTER_TYPE, FIGHTER_COUNT> m_LobbyCharacters{};
	// カメラ・腕・HUD はスロット番号で持つ。使うのは人のスロットだけ。
	Camera m_Cameras[FIGHTER_COUNT];
	ArmManager m_Arms[FIGHTER_COUNT];
	Hud m_Huds[FIGHTER_COUNT];
	int m_Humans[FIGHTER_COUNT] = {};	// 画面の順に並べた人のスロット番号
	int m_HumanCount = 0;
	int m_Fighting[FIGHTER_COUNT] = {};	// 腕とカメラを動かす体（人と CPU）のスロット番号
	int m_FightingCount = 0;
	Cpu m_Cpus[FIGHTER_COUNT]; // CPU の枠の入力を作る（使うのは CPU の枠だけ）
	PLAYER_INPUT m_Inputs[FIGHTER_COUNT];
	PLAYER_INPUT m_PreviousInputs[FIGHTER_COUNT];
	PLAYER_INPUT m_DeviceInputs[ACTION_DEVICE_COUNT]; // 参加前の決定にも使う
	Match m_Match;
	SceneCalc m_Scene;
	CodexMenu m_CodexMenu;
	MenuSe m_MenuSe; // メニューの状態の変わり方から効果音を鳴らす
	NetSession m_Net;
	SessionAddressCalc m_JoinAddress;
	uint32_t m_OfflineSeed = 0;
	uint32_t m_MatchSeed = 0;
	int m_MenuDevice = ACTION_DEVICE_KEYBOARD; // SOLO の決定を押した機器
	bool m_Fullscreen = false;
	bool m_TuningResetSe = false; // このフレームに「標準に戻す」を受け付けた（メニューの音を選ぶため）
	OPTION_SETTINGS m_SavedSettings; // ファイルに保存してある設定（保存していない変更があるかを見るため）
	bool m_OptionsSaveSe = false; // このフレームにオプションの「保存」でファイルに書けた（メニューの音を選ぶため）
	bool m_MapDecideHeld = false; // マップ選択に入ってから、1P が決定をまだ離していない（離すまで決定を受け付けない）
};
