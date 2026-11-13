#pragma once

/// @brief 片手に装備した腕の残り耐久を管理する（DirectX・Windows なし）
class ArmUseCalc
{
public:
	/// @brief 装備した腕の最大耐久で残り耐久を設定する
	void Reset(int maxUses);
	/// @brief 使用した耐久を減らす（残りは 0 で止まる）
	void Consume(int count);
	/// @brief 残り耐久を取得する
	int GetRemaining() const { return m_Remaining; }
	/// @brief 残りが 0 なら true を返す
	bool IsExhausted() const { return m_Remaining == 0; }

private:
	int m_Remaining = 0;
};
