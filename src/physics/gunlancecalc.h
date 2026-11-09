#pragma once

/// @file gunlancecalc.h
/// @brief ガンランスの段階のためと、突進で同じ相手に当てる間隔の計算（DirectX と Windows は使わない）

/// @brief ためられる段階の最大
static constexpr int GUNLANCE_MAX_LEVEL = 3;

/// @brief 相手ごとの間隔を覚えられる相手の番号の数（番号は 0 から）
static constexpr int GUNLANCE_TARGET_MAX = 8;

/// @brief ガンランスの調整値
struct GUNLANCE_PARAMS
{
	float WaitSeconds; ///< 最後に押してからこの秒数押さないと発射する
	float RehitSeconds; ///< 同じ相手にもう一度当てられるまでの秒数
};

/// @brief 段階のためと、相手ごとの当たる間隔を管理する
class GunlanceCalc
{
public:
	/// @brief ためなしで初期化する
	GunlanceCalc() = default;
	/// @brief ためと相手ごとの間隔を消す
	void Reset();
	/// @brief 1フレーム進める。押すと1段階ため、最大の段階か、押さない時間が続くと発射する
	/// @param tap 入力を設定する（true：このフレームに押した、false：押していない）
	/// @param dt 1フレームの秒数
	/// @param params 調整値
	/// @return 発射した段階（1〜GUNLANCE_MAX_LEVEL）。発射しなければ 0
	int Step(bool tap, float dt, const GUNLANCE_PARAMS& params);
	/// @brief 相手に当ててよければ、間隔を始めて true を返す
	/// @param target 相手の番号（0〜GUNLANCE_TARGET_MAX-1）
	/// @param params 調整値
	/// @return 当ててよければ true
	bool TryHit(int target, const GUNLANCE_PARAMS& params);
	/// @brief 今ためている段階を取得する（0 はためなし）
	int GetLevel() const { return m_Level; }

private:
	int m_Level = 0; ///< 今ためている段階
	float m_Wait = 0.0f; ///< 最後に押してからの秒数
	float m_Rehit[GUNLANCE_TARGET_MAX] = {}; ///< 相手ごとの、もう一度当てられるまでの残りの秒数
};
