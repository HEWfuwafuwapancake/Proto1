#pragma once

/// @file escapecalc.h
/// @brief 掴まれている間の連打で抜ける計算
/// @details DirectX と Windows を使わないので、単体テスト（tests/Proto1Tests）からそのまま使える

/// @brief ボタンとスティックの倒し直しを数え、ダメージ%に応じた回数で掴みから抜ける計算
class EscapeCalc
{
public:
	/// @brief 掴まれ始めの状態へ戻す（連打の回数を0にする）
	void Reset();
	/// @brief このフレームの連打を加算し、必要な回数に達したら true を返す
	/// @param presses ジャンプ・左右の腕・ダッシュを押した瞬間の数
	/// @param moveMagnitude 移動スティックの大きさ（0〜1）
	/// @param percent 今のダメージ%
	bool Step(int presses, float moveMagnitude, float percent);

private:
	int m_Count = 0; ///< 今回の掴みで数えた連打の回数
	bool m_StickReady = true; ///< スティックを倒して次の1回を数えられるか
};
