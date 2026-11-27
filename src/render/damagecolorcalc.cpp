#include "render/damagecolorcalc.h"

namespace
{
	// 調整用の値（plan/11_エフェクトと手ごたえ の 2-1b。遊んで飛びやすくなる % に合わせて直す）
	struct COLOR_STOP
	{
		float Percent; // この % で
		DAMAGE_COLOR Color; // この色になる
	};
	constexpr COLOR_STOP TEXT_STOPS[] = {
		{ 0.0f, { 1.0f, 1.0f, 1.0f } }, // 白
		{ 50.0f, { 1.0f, 0.95f, 0.3f } }, // 黄
		{ 100.0f, { 1.0f, 0.55f, 0.1f } }, // オレンジ
		{ 150.0f, { 1.0f, 0.15f, 0.1f } }, // 赤
		{ 200.0f, { 0.6f, 0.05f, 0.05f } }, // 暗い赤
	};
	constexpr int TEXT_STOP_COUNT = sizeof(TEXT_STOPS) / sizeof(TEXT_STOPS[0]);
	constexpr float TINT_START_PERCENT = 70.0f; // 体が赤くなり始める %
	constexpr float TINT_FULL_PERCENT = 160.0f; // 体の赤みがいちばん強くなる %
	constexpr float TINT_MAX = 0.55f; // 体の色に混ぜる赤のいちばん多い割合
	constexpr float SMOKE_GRAY_PERCENT = 100.0f; // 灰色の煙が出始める %
	constexpr float SMOKE_BLACK_PERCENT = 150.0f; // 黒い煙と火花になる %
}

DAMAGE_COLOR DamageColorCalcText(float percent)
{
	if (percent <= TEXT_STOPS[0].Percent) return TEXT_STOPS[0].Color;
	for (int i = 1; i < TEXT_STOP_COUNT; ++i) {
		if (percent < TEXT_STOPS[i].Percent) {
			const COLOR_STOP& a = TEXT_STOPS[i - 1];
			const COLOR_STOP& b = TEXT_STOPS[i];
			const float t = (percent - a.Percent) / (b.Percent - a.Percent);
			return { a.Color.R + (b.Color.R - a.Color.R) * t, a.Color.G + (b.Color.G - a.Color.G) * t,
				a.Color.B + (b.Color.B - a.Color.B) * t };
		}
	}
	return TEXT_STOPS[TEXT_STOP_COUNT - 1].Color;
}

float DamageColorCalcBodyTint(float percent)
{
	if (percent <= TINT_START_PERCENT) return 0.0f;
	if (percent >= TINT_FULL_PERCENT) return TINT_MAX;
	return TINT_MAX * (percent - TINT_START_PERCENT) / (TINT_FULL_PERCENT - TINT_START_PERCENT);
}

DAMAGE_SMOKE DamageColorCalcSmoke(float percent)
{
	if (percent >= SMOKE_BLACK_PERCENT) return DAMAGE_SMOKE_BLACK;
	if (percent >= SMOKE_GRAY_PERCENT) return DAMAGE_SMOKE_GRAY;
	return DAMAGE_SMOKE_NONE;
}
