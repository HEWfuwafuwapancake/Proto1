#pragma once

/// @file parrycalc.h
/// @brief ガードで攻撃を受けたときに、パリィか腕を失うかを決める計算
/// @details DirectX と Windows を使わないので、単体テスト（tests/Proto1Tests）からそのまま使える

/// @brief 構えた手で攻撃を防いだ結果
enum PARRY_RESULT
{
	PARRY_NONE, ///< 構えていない、または攻撃を防いでいない
	PARRY_SUCCESS, ///< パリィに成功し、腕を残す
	PARRY_BREAK_ARM ///< パリィできずに防ぎ、耐久が減る
};

/// @brief 片手のガード開始からの受付時間と、構え直すまでの待ち時間を管理する
class ParryCalc
{
public:
	/// @brief 最初のパリィを受け付けられる、構えていない状態で初期化する
	ParryCalc();
	/// @brief 試合開始前の状態へ戻す
	void Reset();
	/// @brief 構えと時間を1フレーム進める
	/// @param guarding 構えを設定する（true：構える、false：解く）
	/// @param dt このフレームの秒数
	void Step(bool guarding, float dt);
	/// @brief ガードを構えているかを取得する
	bool IsGuarding() const { return m_Guarding; }
	/// @brief 構えてからの秒数を取得する（描画用。構えていなければ0）
	float GetGuardSeconds() const { return static_cast<float>(m_GuardSeconds); }
	/// @brief 今防ぐとパリィになるかを取得する
	bool IsParryWindow() const;
	/// @brief 攻撃を防いだ結果を返す（構えは変えない）
	/// @param blocked 防いだかを設定する（true：防いだ、false：防いでいない）
	PARRY_RESULT OnHit(bool blocked) const;

private:
	bool m_Guarding = false; ///< 構えているか
	bool m_Eligible = false; ///< 今の構えでパリィを受け付けるか
	double m_GuardSeconds = 0.0; ///< 構えてからの秒数
	double m_ReleasedSeconds = 0.0; ///< 解いてからの秒数
};
