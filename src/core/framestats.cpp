#include "core/framestats.h"

// 調整用の値
static constexpr float HITCH_MS = 25.0f; // 60fps の1.5フレームより間隔が長ければカクつき

void FrameStatsAdd(FRAME_STATS& stats, float intervalMs, float workMs)
{
	++stats.FrameCount;
	stats.TotalWorkMs += workMs;
	if (intervalMs > stats.MaxIntervalMs) stats.MaxIntervalMs = intervalMs;
	if (intervalMs > HITCH_MS) ++stats.HitchCount;
}

FRAME_STATS_RESULT FrameStatsTake(FRAME_STATS& stats)
{
	FRAME_STATS_RESULT result{};
	if (stats.FrameCount > 0) result.AverageWorkMs = stats.TotalWorkMs / stats.FrameCount;
	result.MaxIntervalMs = stats.MaxIntervalMs;
	result.HitchCount = stats.HitchCount;
	stats = {};
	return result;
}
