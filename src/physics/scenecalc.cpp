#include "physics/scenecalc.h"
#include <math.h>

//------------------------------------------------------------------------------
// 調整用の値
//------------------------------------------------------------------------------
static constexpr float MENU_STICK_THRESHOLD = 0.6f; // 弾きとして受け付ける大きさ
static constexpr float MENU_STICK_DEADZONE = 0.3f; // 次の弾きの前に戻す範囲

void SceneCalc::Start(bool online)
{
	m_LobbyMode = MODE_ITEM_SOLO;
	m_Rule = online ? GAME_RULE_DEBUG : GAME_RULE_GAME;
	m_Scene = online ? SCENE_TITLE : SCENE_GAME_START;
	m_Cursor = RESULT_ITEM_RETRY;
	m_MenuReady = true;
	m_Online = online;
	m_OptionsReturn = SCENE_GAME_START;
	m_Map = STAGE_MAP_DEFAULT;
	m_MapReady = true;
}

void SceneCalc::OpenLobby()
{
	m_Online = true;
	m_Scene = SCENE_TITLE;
	// マップ選択はホストの入力を全員の PC で同じように進めるので、つないだ時点で全員そろえる。
	m_Map = STAGE_MAP_DEFAULT;
	m_MapReady = true;
}

void SceneCalc::ShowModes()
{
	m_Online = false;
	m_Scene = SCENE_MODE_SELECT;
	m_Cursor = MODE_ITEM_SOLO;
	m_MenuReady = false;
}

int SceneCalcScrollTop(int top, int selected, int visible, int count)
{
	const int maxTop = count > visible ? count - visible : 0;
	if (selected >= 0) {
		if (selected < top) top = selected;
		if (selected >= top + visible) top = selected - visible + 1;
	}
	return top < 0 ? 0 : (top > maxTop ? maxTop : top);
}

void SceneCalc::MoveCursor(float moveY, int count)
{
	if (fabsf(moveY) < MENU_STICK_DEADZONE) m_MenuReady = true;
	if (m_MenuReady && fabsf(moveY) > MENU_STICK_THRESHOLD) {
		if (moveY > 0.0f && m_Cursor > 0) --m_Cursor;
		if (moveY < 0.0f && m_Cursor < count - 1) ++m_Cursor;
		m_MenuReady = false;
	}
}

CHARACTER_TYPE SceneCalc::CycleCharacter(CHARACTER_TYPE character, int direction)
{
	return static_cast<CHARACTER_TYPE>((character + direction + CHARACTER_COUNT) % CHARACTER_COUNT);
}

bool SceneCalc::IsBackTransition(SCENE from, SCENE to)
{
	switch (from) {
	case SCENE_OPTIONS: return to == SCENE_GAME_START || to == SCENE_MODE_SELECT;
	case SCENE_ARM_COSTS: return to == SCENE_OPTIONS;
	case SCENE_MODE_SELECT: return to == SCENE_GAME_START;
	case SCENE_JOIN_ADDRESS: return to == SCENE_MODE_SELECT;
	case SCENE_TITLE: return to == SCENE_MODE_SELECT;
	case SCENE_MAP_SELECT: return to == SCENE_TITLE;
	default: return false;
	}
}

void SceneCalc::CycleRule(int direction)
{
	m_Rule = static_cast<GAME_RULE>((m_Rule + direction + GAME_RULE_COUNT) % GAME_RULE_COUNT);
}

SCENE_EVENT SceneCalc::Step(const SCENE_INPUT& input, bool matchFinished)
{
	if (m_Scene == SCENE_GAME_START) {
		MoveCursor(input.MoveY, 2);
		if (input.Decide) {
			m_OptionsReturn = SCENE_GAME_START;
			if (m_Cursor == 0) ShowModes();
			else {
				m_Scene = SCENE_OPTIONS;
				m_Cursor = 0;
				m_MenuReady = false;
			}
		}
		return SCENE_EVENT_NONE;
	}
	if (m_Scene == SCENE_OPTIONS) {
		MoveCursor(input.MoveY, OPTION_ITEM_COUNT);
		if (input.Back || (input.Decide && m_Cursor == OPTION_ITEM_BACK)) {
			m_Scene = m_OptionsReturn;
			m_Cursor = m_Scene == SCENE_GAME_START ? 1 : MODE_ITEM_OPTIONS;
			m_MenuReady = false;
		}
		else if (input.Decide && m_Cursor == OPTION_ITEM_ARM_COSTS) {
			m_Scene = SCENE_ARM_COSTS;
			m_Cursor = 0;
			m_CostScrollTop = 0;
			m_MenuReady = false;
		}
		else if (input.Decide && m_Cursor == OPTION_ITEM_RESET) return SCENE_EVENT_TUNING_RESET;
		else if (m_Cursor < TUNING_ITEM_COUNT && input.CharacterDirection != 0) return SCENE_EVENT_TUNING_CHANGE;
		else if (m_Cursor == OPTION_ITEM_LOOK_SENSITIVITY && input.CharacterDirection != 0) return SCENE_EVENT_LOOK_SENSITIVITY_CHANGE;
		return SCENE_EVENT_NONE;
	}
	if (m_Scene == SCENE_ARM_COSTS) {
		// 使う欄を上から順に並べ、最後が「戻る」。
		const int back = ArmCostCalcUsedCount();
		MoveCursor(input.MoveY, back + 1);
		// 行が多いときは、選んでいる欄の行が見えるまで表を上下にスクロールする（「戻る」を選んでいる間は動かさない）。
		int row = -1, item = -1;
		ArmCostCalcUsedCell(m_Cursor, row, item);
		m_CostScrollTop = SceneCalcScrollTop(m_CostScrollTop, row, ARM_COST_VISIBLE_ROWS, ARM_COST_ROW_COUNT);
		if (input.Back || (input.Decide && m_Cursor == back)) {
			m_Scene = SCENE_OPTIONS;
			m_Cursor = OPTION_ITEM_ARM_COSTS;
			m_MenuReady = false;
		}
		else if (m_Cursor < back && input.CharacterDirection != 0) return SCENE_EVENT_ARM_COST_CHANGE;
		return SCENE_EVENT_NONE;
	}
	if (m_Scene == SCENE_MODE_SELECT) {
		MoveCursor(input.MoveY, MODE_ITEM_COUNT);
		if (input.Back || (input.Decide && m_Cursor == MODE_ITEM_BACK)) {
			m_Scene = SCENE_GAME_START;
			m_Cursor = 0;
			m_MenuReady = false;
			return SCENE_EVENT_NONE;
		}
		if (!input.Decide) return SCENE_EVENT_NONE;
		switch (m_Cursor) {
		case MODE_ITEM_SOLO:
		case MODE_ITEM_LOCAL:
			// ルールはキャラ選択の画面で選ぶ。
			m_LobbyMode = m_Cursor;
			m_Scene = SCENE_TITLE;
			break;
		case MODE_ITEM_HOST:
			m_LobbyMode = m_Cursor;
			return SCENE_EVENT_HOST;
		case MODE_ITEM_JOIN: m_Scene = SCENE_JOIN_ADDRESS; break;
		case MODE_ITEM_OPTIONS:
			m_OptionsReturn = SCENE_MODE_SELECT;
			m_Scene = SCENE_OPTIONS;
			m_Cursor = 0;
			m_MenuReady = false;
			break;
		default: break;
		}
		return SCENE_EVENT_NONE;
	}
	if (m_Scene == SCENE_JOIN_ADDRESS) {
		if (input.Back) { ShowModes(); return SCENE_EVENT_NONE; }
		return input.Decide && input.AddressValid ? SCENE_EVENT_JOIN : SCENE_EVENT_NONE;
	}
	if (m_Scene == SCENE_TITLE) {
		if (input.Back) {
			// オンラインは接続を閉じる。どちらも選んでいた対戦方法の行へ戻る。
			const int mode = m_LobbyMode;
			ShowModes();
			m_Cursor = mode;
			return SCENE_EVENT_BACK_MENU;
		}
		if (!input.Decide) return SCENE_EVENT_NONE;
		// キャラ選択が終わったら、1P（オンラインはホスト）が試合のマップを選ぶ。
		m_Scene = SCENE_MAP_SELECT;
		m_MapReady = fabsf(input.MoveX) < MENU_STICK_DEADZONE;
		return SCENE_EVENT_OPEN_MAP_SELECT;
	}
	if (m_Scene == SCENE_MAP_SELECT) {
		// 横に並んだマップのカードを、左スティックの左右の弾きで1つずつ選ぶ。
		if (fabsf(input.MoveX) < MENU_STICK_DEADZONE) m_MapReady = true;
		if (m_MapReady && fabsf(input.MoveX) > MENU_STICK_THRESHOLD) {
			m_Map = MapCalcCycle(m_Map, input.MoveX > 0.0f ? 1 : -1);
			m_MapReady = false;
		}
		if (input.Back) {
			m_Scene = SCENE_TITLE;
			return SCENE_EVENT_BACK_LOBBY;
		}
		if (!input.Decide) return SCENE_EVENT_NONE;
		m_Scene = SCENE_MATCH;
		return SCENE_EVENT_START_MATCH;
	}
	if (m_Scene == SCENE_PAUSE) {
		// 開いたボタン・戻る・「再開」で試合へ戻る。ロビー・タイトルへは結果画面と同じ処理で戻る。
		MoveCursor(input.MoveY, PAUSE_ITEM_COUNT);
		if (input.Pause || input.Back || (input.Decide && m_Cursor == PAUSE_ITEM_RESUME)) {
			m_Scene = SCENE_MATCH;
			return SCENE_EVENT_NONE;
		}
		if (!input.Decide) return SCENE_EVENT_NONE;
		if (m_Cursor == PAUSE_ITEM_LOBBY) {
			m_Scene = SCENE_TITLE;
			return SCENE_EVENT_BACK_LOBBY;
		}
		m_Scene = SCENE_GAME_START;
		m_Cursor = 0;
		m_MenuReady = false;
		return SCENE_EVENT_BACK_TITLE;
	}
	if (m_Scene == SCENE_MATCH) {
		if (input.Pause && !matchFinished) {
			m_Scene = SCENE_PAUSE;
			m_Cursor = PAUSE_ITEM_RESUME;
			m_MenuReady = fabsf(input.MoveY) < MENU_STICK_DEADZONE;
			return SCENE_EVENT_NONE;
		}
		if (input.Restart) return SCENE_EVENT_START_MATCH;
		if (!matchFinished || !input.Decide) return SCENE_EVENT_NONE;
		m_Scene = SCENE_RESULT;
		m_Cursor = RESULT_ITEM_RETRY;
		// 入る前から倒している入力では選択を動かさず、一度戻すのを待つ。
		m_MenuReady = fabsf(input.MoveY) < MENU_STICK_DEADZONE;
		return SCENE_EVENT_SHOW_RESULT;
	}

	MoveCursor(input.MoveY, RESULT_ITEM_COUNT);
	if (input.Restart || (input.Decide && m_Cursor == RESULT_ITEM_RETRY)) {
		m_Scene = SCENE_MATCH;
		return SCENE_EVENT_START_MATCH;
	}
	if (input.Decide && m_Cursor == RESULT_ITEM_CHARACTER) {
		m_Scene = SCENE_TITLE;
		return SCENE_EVENT_BACK_LOBBY;
	}
	if (input.Decide) {
		if (m_Online) m_Scene = SCENE_TITLE;
		else { m_Scene = SCENE_GAME_START; m_Cursor = 0; m_MenuReady = false; }
		return SCENE_EVENT_BACK_TITLE;
	}
	return SCENE_EVENT_NONE;
}
