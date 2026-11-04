#pragma once

/// @file armphasecalc.h
/// @brief 腕の「溜め → 攻撃 → 休み」のような段階を時間で進める共通の計算（DirectX と Windows は使わない）

/// @brief 1フレームで刻みを返す最大の数（長いフレームでも一度に効かせすぎない）
static constexpr int ARM_PHASE_MAX_TICKS = 5;

/// @brief 1フレームの段階の変化
struct ARM_PHASE_STEP
{
	int Entered; ///< このフレームに入った段階（入っていなければ -1。待機に戻ったときも -1）
	bool Finished; ///< このフレームに最後の段階を終えて待機に戻ったか
	int Ticks; ///< このフレームに過ぎた刻みの数（0〜ARM_PHASE_MAX_TICKS）
};

/// @brief 腕の段階（0〜）と、その段階になってからの時間を管理する。段階 -1 は待機
class ArmPhaseCalc
{
public:
	/// @brief 待機で初期化する
	ArmPhaseCalc() = default;
	/// @brief 待機（段階 -1）へ戻し、時間を 0 にする
	void Reset();
	/// @brief 指定した段階を時間 0 から始める
	/// @param phase 始める段階（0〜）
	void Start(int phase);
	/// @brief 時間を進め、段階の秒数を過ぎたら次の段階へ、最後の段階を過ぎたら待機へ戻す
	/// @details 段階を変えたフレームの残りの時間は次の段階へ持ち越さない（shieldcalc と同じ）。
	///          tickSeconds が正なら、段階の中で tickSeconds ごとの刻みを数える（段階の始まりは数えない）
	/// @param dt 進める秒数（0 以下なら何もしない）
	/// @param seconds 段階ごとの秒数の並び（count 個）
	/// @param count 段階の数
	/// @param tickSeconds 刻みの間隔（0 以下なら数えない）
	/// @return このフレームの変化
	ARM_PHASE_STEP Step(float dt, const float* seconds, int count, float tickSeconds = 0.0f);
	/// @brief 今の段階を取得する（待機は -1）
	int GetPhase() const { return m_Phase; }
	/// @brief 今の段階になってからの秒数を取得する
	float GetTime() const { return m_Time; }
	/// @brief 待機なら true を返す
	bool IsIdle() const { return m_Phase < 0; }

private:
	int m_Phase = -1; ///< 今の段階（待機は -1）
	float m_Time = 0.0f; ///< 今の段階になってからの秒数
	int m_TickCount = 0; ///< 今の段階で数えた刻みの数
};
