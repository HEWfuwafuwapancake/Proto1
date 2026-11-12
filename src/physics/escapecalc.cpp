#include "physics/escapecalc.h"
#include <math.h>

// 調整用の値
static constexpr int ESCAPE_BASE = 6; // 0% のときに抜けるための回数
static constexpr int ESCAPE_MAX = 14; // 必要な回数の上限
static constexpr float ESCAPE_PERCENT_STEP = 25.0f; // 必要な回数が1増えるダメージ%
static constexpr float STICK_RELEASE = 0.3f; // 次の倒し直しのために戻す大きさ
static constexpr float STICK_PRESS = 0.7f; // 1回と数える大きさ

void EscapeCalc::Reset()
{
	*this = EscapeCalc{};
}

bool EscapeCalc::Step(int presses, float moveMagnitude, float percent)
{
	if (moveMagnitude <= STICK_RELEASE) m_StickReady = true;
	if (m_StickReady && moveMagnitude > STICK_PRESS) {
		++m_Count;
		m_StickReady = false;
	}
	m_Count += presses;
	const float extra = ceilf(fmaxf(0.0f, percent) / ESCAPE_PERCENT_STEP);
	const int required = static_cast<int>(fminf(static_cast<float>(ESCAPE_MAX), ESCAPE_BASE + extra));
	return m_Count >= required;
}
