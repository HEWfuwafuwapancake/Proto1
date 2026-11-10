#pragma once

/// @brief 片手の腕に渡す1フレーム分の入力
struct ARM_INPUT
{
	bool Tap = false; ///< 長押しになる前に離したフレーム（腕が動作中なら、受け付けられるようになったフレーム）だけ true
	bool HoldStarted = false; ///< 長押しに達したフレーム（腕が動作中なら、受け付けられるようになったフレーム）だけ true
	bool Pressed = false; ///< ボタンを押している間は true
};

/// @brief 腕のボタンの押下から単押しと長押しを判定する（DirectX・Windows なし）
class ArmPressCalc
{
public:
	/// @brief 押下の判定と覚えている単押しを取り消し、次の押下に備える
	/// @param pressed 現在のボタンの状態（true：一度離すまで開始しない、false：次に押すと開始する）
	void Reset(bool pressed = false);
	/// @brief 片手分の入力を1フレーム進める
	/// @param pressed 現在のボタンの状態（true：押している、false：離している）
	/// @param dt 1フレームの秒数
	/// @param canTap 単押しの受け付けを設定する（true：すぐ渡す、false：少しの間覚えておき、受け付けられたら渡す）
	/// @param canHold 長押しの受け付けを設定する（true：すぐ渡す、false：押し続けている間は待ち、受け付けられたら渡す）
	ARM_INPUT Step(bool pressed, float dt, bool canTap = true, bool canHold = true);

private:
	enum class State { Idle, Timing, HoldWaiting, WaitForRelease };
	State m_State = State::Idle;
	double m_Seconds = 0.0;
	double m_TapBuffer = 0.0; // 渡せなかった単押しを覚えている残りの秒数。
};
