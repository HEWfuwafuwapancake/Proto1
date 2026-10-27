#pragma once

/// @brief フレームの間に押して離したキーも、次の更新まで記憶する（Windows 不使用）
class KeyboardTriggerCalc
{
public:
	/// @brief キーを押したメッセージを記憶する。長押しの自動リピートは無視する
	/// @param repeat 自動リピートか（true：リピート、false：新しい押下）
	/// @param shift 押した時点の Shift（true：押している、false：押していない）
	void Press(bool repeat, bool shift)
	{
		if (repeat || m_Pending) return;
		m_Pending = true;
		m_PendingShift = shift;
	}

	/// @brief 記憶した押下を、このフレームだけのトリガーにする
	void Update()
	{
		m_Triggered = m_Pending;
		m_Shift = m_PendingShift;
		m_Pending = false;
		m_PendingShift = false;
	}

	/// @brief 非選択になったときなどに、記憶とトリガーを消す
	void Reset() { m_Pending = m_PendingShift = m_Triggered = m_Shift = false; }
	/// @brief このフレームに新しい押下があるかを返す
	bool IsTriggered() const { return m_Triggered; }
	/// @brief 記憶した押下の時点で Shift を押していたかを返す
	bool HadShift() const { return m_Shift; }

private:
	bool m_Pending = false;
	bool m_PendingShift = false;
	bool m_Triggered = false;
	bool m_Shift = false;
};
