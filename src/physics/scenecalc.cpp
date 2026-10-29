#include "physics/scenecalc.h"
#include <math.h>

//------------------------------------------------------------------------------
// 調整用の値
//------------------------------------------------------------------------------
static constexpr float MENU_STICK_THRESHOLD = 0.6f; // 弾きとして受け付ける大きさ
static constexpr float MENU_STICK_DEADZONE = 0.3f; // 次の弾きの前に戻す範囲

void SceneCalc::Start()
{
	m_Scene = SCENE_TITLE;
	m_Cursor = RESULT_ITEM_RETRY;
	m_MenuReady = true;
}

SCENE_EVENT SceneCalc::Step(const SCENE_INPUT& input, bool matchFinished)
{
	if (m_Scene == SCENE_TITLE) {
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

	if (fabsf(input.MoveY) < MENU_STICK_DEADZONE) m_MenuReady = true;
	if (m_MenuReady && fabsf(input.MoveY) > MENU_STICK_THRESHOLD) {
		if (input.MoveY > 0.0f && m_Cursor > RESULT_ITEM_RETRY) --m_Cursor;
		if (input.MoveY < 0.0f && m_Cursor < RESULT_ITEM_COUNT - 1) ++m_Cursor;
		m_MenuReady = false;
	}
	if (input.Restart || (input.Decide && m_Cursor == RESULT_ITEM_RETRY)) {
		m_Scene = SCENE_MATCH;
		return SCENE_EVENT_START_MATCH;
	}
	if (input.Decide) {
		m_Scene = SCENE_TITLE;
		return SCENE_EVENT_BACK_TITLE;
	}
	return SCENE_EVENT_NONE;
}
