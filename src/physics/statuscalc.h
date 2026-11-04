#pragma once

/// @file statuscalc.h
/// @brief 1体の状態異常（しびれ・遅い・燃える・隠れる）の残り時間と効き目（DirectX と Windows は使わない）

/// @brief 状態異常の種類
enum STATUS_TYPE
{
	STATUS_STUN, ///< しびれ：歩けない・跳べない・腕を使えない・向きを変えない
	STATUS_SLOW, ///< 遅い：歩く速さに倍率を掛ける
	STATUS_BURN, ///< 燃える：1秒ごとに % が増える
	STATUS_HIDDEN, ///< 隠れる：ほかの人からロックオンされない（スモーク）
	STATUS_COUNT ///< 種類の数（状態異常ではない）
};

/// @brief 1フレームの状態異常の結果
struct STATUS_STEP
{
	float Damage; ///< このフレームに燃えた分のダメージ%
	int Attacker; ///< 燃やした人のスロット番号（燃えていなければ -1）
};

/// @brief 1体が持つ状態異常を管理する
class StatusCalc
{
public:
	/// @brief 状態異常なしで初期化する
	StatusCalc() = default;
	/// @brief すべての状態異常を消す
	void Reset();
	/// @brief 状態異常をかける。残り時間は今と比べて長い方にし、値は上書きする
	/// @param type 種類
	/// @param seconds 続く秒数（0 以下なら何もしない）
	/// @param value 遅いなら歩く速さの倍率、燃えるなら1秒あたりの %（ほかは使わない）
	/// @param attacker かけた人のスロット番号（燃えるときの得点に使う。なしは -1）
	void Apply(STATUS_TYPE type, float seconds, float value, int attacker);
	/// @brief 残り時間を減らし、燃えた分のダメージを返す
	/// @param dt 進める秒数（0 以下なら何もしない）
	/// @return このフレームの結果
	STATUS_STEP Step(float dt);
	/// @brief その状態異常がかかっているか取得する
	/// @param type 種類
	bool IsActive(STATUS_TYPE type) const;
	/// @brief 歩く速さの倍率を取得する（しびれは 0、遅いはその倍率、ほかは 1）
	float GetMoveScale() const;

private:
	float m_Time[STATUS_COUNT] = {}; ///< 種類ごとの残りの秒数
	float m_SlowScale = 1.0f; ///< 遅いときの歩く速さの倍率
	float m_BurnPerSecond = 0.0f; ///< 燃えるときの1秒あたりの %
	int m_BurnAttacker = -1; ///< 燃やした人のスロット番号
};
