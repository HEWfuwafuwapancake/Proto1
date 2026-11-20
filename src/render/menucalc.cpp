#include "render/menucalc.h"
#include <cmath>

// 調整用の値
static constexpr float PI = 3.14159265f;
static constexpr float OVERSHOOT = 1.70158f; // 行き過ぎの強さ（よく使われる値。約 10% 行き過ぎる）

MENU_POINT MenuCalcOrbitEye(float seconds, const MENU_POINT& center, float radius, float height, float periodSeconds)
{
	const float angle = periodSeconds > 0.0f ? 2.0f * PI * std::fmod(seconds, periodSeconds) / periodSeconds : 0.0f;
	return { center.X + cosf(angle) * radius, center.Y + height, center.Z + sinf(angle) * radius };
}

float MenuCalcEaseOutBack(float progress)
{
	const float t = fminf(fmaxf(progress, 0.0f), 1.0f) - 1.0f;
	return 1.0f + (OVERSHOOT + 1.0f) * t * t * t + OVERSHOOT * t * t;
}
