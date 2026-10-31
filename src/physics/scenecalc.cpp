#include "physics/scenecalc.h"
#include <math.h>

//------------------------------------------------------------------------------
// 調整用の値
//------------------------------------------------------------------------------
static constexpr float MENU_STICK_THRESHOLD = 0.6f; // 弾きとして受け付ける大きさ
static constexpr float MENU_STICK_DEADZONE = 0.3f; // 次の弾きの前に戻す範囲

void SceneCalc::Start(bool online)
{
	m_Scene = online ? SCENE_TITLE : SCENE_GAME_START;
	m_Cursor = RESULT_ITEM_RETRY;
	m_MenuReady = true;
	m_Online = online;
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

SCENE_EVENT SceneCalc::Step(const SCENE_INPUT& input, bool matchFinished)
{
	if (m_Scene == SCENE_GAME_START) {
		if (input.Decide) ShowModes();
		return SCENE_EVENT_NONE;
	}
	if (m_Scene == SCENE_MODE_SELECT) {
		MoveCursor(input.MoveY, MODE_ITEM_COUNT);
		if (input.Back || (input.Decide && m_Cursor == MODE_ITEM_BACK)) {
			m_Scene = SCENE_GAME_START;
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
		else ShowModes();
		return SCENE_EVENT_BACK_TITLE;
	}
	return SCENE_EVENT_NONE;
}
