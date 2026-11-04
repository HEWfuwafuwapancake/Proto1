#pragma once

/// @file bladecalc.h
/// @brief ブレードの連続斬り（溜め → 振り → 次を受け付ける間 → 休み）の流れ（DirectX と Windows は使わない）

/// @brief ブレードの段階
enum BLADE_STATE
{
	BLADE_IDLE, ///< 待機
	BLADE_WINDUP, ///< 振る前の溜め
	BLADE_SWING, ///< 振っている
	BLADE_WINDOW, ///< 次の振りを受け付ける間
	BLADE_COOLDOWN ///< 次の入力まで休む
};

/// @brief ブレードの調整値（秒数は正の値を指定する）
struct BLADE_PARAMS
{
	float WindupSeconds; ///< 振る前の溜めの秒数
	float SwingSeconds; ///< 振っている秒数
	float WindowSeconds; ///< 振り終わってから次を受け付ける秒数
	float CooldownSeconds; ///< 最後の振りのあと・受け付けが切れたあとの休みの秒数
	int MaxSlash; ///< 続けて振れる回数
};

/// @brief 片手のブレードの連続斬りを管理する
class BladeCalc
{
public:
	/// @brief 待機で初期化する
	BladeCalc() = default;
	/// @brief 待機へ戻す
	void Reset();
	/// @brief 1フレーム進める。溜め・振りの間に押した分は覚えておき、受け付ける間に入ったらすぐ次を振る
	/// @param trigger 入力を設定する（true：押した瞬間、false：入力なし）
	/// @param cancel 取りやめを設定する（true：待機へ戻す、false：通常更新）
	/// @param dt 進める秒数（0 以下なら止まる。ただし cancel は処理する）
	/// @param params 調整値
	/// @return 振りが当たるフレームなら何回目の振りか（1〜MaxSlash）、ほかは 0
	int Step(bool trigger, bool cancel, float dt, const BLADE_PARAMS& params);
	/// @brief 今の段階を取得する
	BLADE_STATE GetState() const { return m_State; }
	/// @brief 今または最後の振りが何回目か取得する（待機は 0）
	int GetSlash() const { return m_Slash; }
	/// @brief 今の段階になってからの秒数を取得する
	float GetTime() const { return m_Time; }

private:
	void Enter(BLADE_STATE state);
	BLADE_STATE m_State = BLADE_IDLE; ///< 今の段階
	int m_Slash = 0; ///< 何回目の振りか（待機は 0）
	float m_Time = 0.0f; ///< 今の段階になってからの秒数
	bool m_Queued = false; ///< 溜め・振りの間に次の入力があったか
};
