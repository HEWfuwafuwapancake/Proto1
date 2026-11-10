#pragma once

/// @brief 片手に装備した腕の残り使用回数を管理する（DirectX・Windows なし）
class ArmUseCalc
{
public:
	/// @brief 装備した腕の最大使用回数で残り回数を設定する
	void Reset(int maxUses);
	/// @brief 使用した回数を減らす（残りは 0 で止まる）
	void Consume(int count);
	/// @brief 残り使用回数を取得する
	int GetRemaining() const { return m_Remaining; }
	/// @brief 残りが 0 なら true を返す
	bool IsExhausted() const { return m_Remaining == 0; }

private:
	int m_Remaining = 0;
};
