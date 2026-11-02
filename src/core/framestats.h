#pragma once

/// @file framestats.h
/// @brief フレームの間隔と処理時間の集計（Windows・DirectX を使わない）

/// @brief 直近1秒分の集計用データ。{} で空の状態にする
struct FRAME_STATS
{
	int FrameCount;          ///< 処理時間を足したフレーム数
	float TotalWorkMs;       ///< 処理時間の合計（ミリ秒。Present の待ちは含めない）
	float MaxIntervalMs;     ///< 前のフレーム開始からの最大間隔（ミリ秒）
	int HitchCount;          ///< 間隔がカクつきのしきい値を超えた回数
};

/// @brief 集計を取り出した結果
struct FRAME_STATS_RESULT
{
	float AverageWorkMs;     ///< 平均の処理時間（ミリ秒。フレームがなければ 0）
	float MaxIntervalMs;     ///< 最大のフレーム間隔（ミリ秒）
	int HitchCount;          ///< カクつきの回数
};

/// @brief 1フレーム分の間隔と処理時間を足す
/// @param[in,out] stats 集計用データ
/// @param intervalMs 前のフレーム開始からの間隔（ミリ秒）。最初や仕切り直しは 0 を渡して間隔だけ数えない
/// @param workMs Update 開始から Present 直前までの処理時間（ミリ秒）
void FrameStatsAdd(FRAME_STATS& stats, float intervalMs, float workMs);

/// @brief 結果を取り出し、集計を空に戻す（1秒ごとに呼ぶ）
/// @param[in,out] stats 集計用データ
/// @return 平均の処理時間・最大間隔・カクつきの回数
FRAME_STATS_RESULT FrameStatsTake(FRAME_STATS& stats);
