#pragma once

/// @brief 片手の腕に渡す1フレーム分の入力
struct ARM_INPUT
{
	bool Tap = false; ///< 長押しになる前に離したフレームだけ true
	bool HoldStarted = false; ///< 長押しに達したフレームだけ true
	bool Pressed = false; ///< ボタンを押している間は true
};

/// @brief 腕のボタンの押下から単押しと長押しを判定する（DirectX・Windows なし）
class ArmPressCalc
{
public:
	/// @brief 押下の判定を取り消し、次の押下に備える
	/// @param pressed 現在のボタンの状態（true：一度離すまで開始しない、false：次に押すと開始する）
	void Reset(bool pressed = false);
	/// @brief 片手分の入力を1フレーム進める
	/// @param pressed 現在のボタンの状態（true：押している、false：離している）
	/// @param dt 1フレームの秒数
	ARM_INPUT Step(bool pressed, float dt);

private:
	enum class State { Idle, Timing, WaitForRelease };
	State m_State = State::Idle;
	double m_Seconds = 0.0;
};
