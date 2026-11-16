#include "physics/scenecalc.h"
#include <math.h>

//------------------------------------------------------------------------------
// 調整用の値
//------------------------------------------------------------------------------
static constexpr float MENU_STICK_THRESHOLD = 0.6f; // 弾きとして受け付ける大きさ
static constexpr float MENU_STICK_DEADZONE = 0.3f; // 次の弾きの前に戻す範囲

void SceneCalc::Start(bool online)
{
	m_Character = CHARACTER_DEFAULT;
	m_Rule = online ? GAME_RULE_DEBUG : GAME_RULE_GAME;
	m_Scene = online ? SCENE_TITLE : SCENE_GAME_START;
	m_Cursor = RESULT_ITEM_RETRY;
	m_MenuReady = true;
	m_Online = online;
	m_OptionsReturn = SCENE_GAME_START;
}

void SceneCalc::OpenLobby()
{
	m_Online = true;
	m_Scene = SCENE_TITLE;
}

void SceneCalc::ShowModes()
{
	m_Online = false;
	m_Scene = SCENE_MODE_SELECT;
	m_Cursor = MODE_ITEM_SOLO;
	m_MenuReady = false;
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

SCENE_EVENT SceneCalc::Step(const SCENE_INPUT& input, bool matchFinished)
{
	if (m_Scene == SCENE_GAME_START) {
		MoveCursor(input.MoveY, 2);
		if (input.Decide) {
			m_OptionsReturn = SCENE_GAME_START;
			m_Scene = m_Cursor == 0 ? SCENE_CHARACTER_SELECT : SCENE_OPTIONS;
			m_Cursor = 0;
			m_MenuReady = false;
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
			m_MenuReady = false;
		}
		else if (input.Decide && m_Cursor == OPTION_ITEM_RESET) return SCENE_EVENT_TUNING_RESET;
		else if (m_Cursor < TUNING_ITEM_COUNT && input.CharacterDirection != 0) return SCENE_EVENT_TUNING_CHANGE;
		return SCENE_EVENT_NONE;
	}
	if (m_Scene == SCENE_ARM_COSTS) {
		// 使う欄を上から順に並べ、最後が「戻る」。
		const int back = ArmCostCalcUsedCount();
		MoveCursor(input.MoveY, back + 1);
		if (input.Back || (input.Decide && m_Cursor == back)) {
			m_Scene = SCENE_OPTIONS;
			m_Cursor = OPTION_ITEM_ARM_COSTS;
			m_MenuReady = false;
		}
		else if (m_Cursor < back && input.CharacterDirection != 0) return SCENE_EVENT_ARM_COST_CHANGE;
		return SCENE_EVENT_NONE;
	}
	if (m_Scene == SCENE_CHARACTER_SELECT) {
		if (input.Back) { m_Scene = SCENE_GAME_START; m_Cursor = 0; m_MenuReady = false; }
		else {
			m_Character = CycleCharacter(m_Character, input.CharacterDirection);
			if (input.Decide) {
				m_Scene = SCENE_RULE_SELECT;
				m_Cursor = m_Rule;
				m_MenuReady = fabsf(input.MoveY) < MENU_STICK_DEADZONE;
			}
		}
		return SCENE_EVENT_NONE;
	}
	if (m_Scene == SCENE_RULE_SELECT) {
		MoveCursor(input.MoveY, GAME_RULE_COUNT);
		if (input.Back) m_Scene = SCENE_CHARACTER_SELECT;
		else if (input.Decide) {
			m_Rule = static_cast<GAME_RULE>(m_Cursor);
			ShowModes();
		}
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
			m_Scene = SCENE_MATCH;
			return SCENE_EVENT_START_MATCH;
		case MODE_ITEM_LOCAL: m_Scene = SCENE_TITLE; break;
		case MODE_ITEM_HOST: return SCENE_EVENT_HOST;
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
		if (input.Back) { ShowModes(); return SCENE_EVENT_BACK_MENU; }
		if (!input.Decide) return SCENE_EVENT_NONE;
		m_Scene = SCENE_MATCH;
		return SCENE_EVENT_START_MATCH;
	}
	if (m_Scene == SCENE_MATCH) {
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
	if (input.Decide) {
		if (m_Online) m_Scene = SCENE_TITLE;
		else { m_Scene = SCENE_GAME_START; m_Cursor = 0; m_MenuReady = false; }
		return SCENE_EVENT_BACK_TITLE;
	}
	return SCENE_EVENT_NONE;
}
