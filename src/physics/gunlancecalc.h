#pragma once

#include "physics/armphasecalc.h"

/// @file gunlancecalc.h
/// @brief ガンランスの射撃と突き上げの時間の計算（DirectX と Windows は使わない）

/// @brief 1フレームの撃ち込みの結果
struct GUNLANCE_STEP
{
	int Shots = 0; ///< このフレームに撃つ弾の数
	bool Finished = false; ///< このフレームの最後の弾で撃ち込みが終わるか
};

/// @brief 単発射撃の間隔と、突進から撃ち込みまでの段階を管理する
class GunlanceCalc
{
public:
	/// @brief 待機に戻し、射撃の間隔も消す
	void Reset();
	/// @brief 今の突進・突き上げ・撃ち込みをやめて待機に戻す
	void StopAction();
	/// @brief このフレームに単発射撃ができるか調べる
	/// @param dt 入力判定から発射までに進める秒数（すでに進めた場合は 0）
	bool CanFire(float dt = 0.0f) const;
	/// @brief 弾を1発出せたとき、次に単発射撃できるまでの間隔を始める
	void RecordShot();
	/// @brief 長押しの突進を始める
	void StartRush();
	/// @brief 突進で相手をつかんだあと、残弾の範囲で突き上げと撃ち込みを始める
	/// @param remaining 1発以上の残弾（-1 は無制限）
	void StartImpale(int remaining);
	/// @brief 1フレーム進め、そのフレームに撃ち込む弾を返す
	/// @param dt 進める秒数
	/// @param pressed 長押しの状態を設定する（true：押している、false：離している）
	GUNLANCE_STEP Step(float dt, bool pressed);
	/// @brief 待機中なら true を返す
	bool IsIdle() const { return m_Phase.IsIdle(); }
	/// @brief 突進中なら true を返す
	bool IsRushing() const { return m_Phase.GetPhase() == Rush; }
	/// @brief 突き上げや撃ち込みで相手を保持する段階なら true を返す
	bool IsHolding() const { return m_Phase.GetPhase() == Lift || m_Phase.GetPhase() == Burst; }
	/// @brief 突進を始めてからの秒数を取得する（突進していないときは 0）
	float GetRushTime() const { return IsRushing() ? m_Phase.GetTime() : 0.0f; }
	/// @brief 突き上げ中の進み具合を 0〜1 で取得する（突き上げが済んだら 1）
	float GetLiftProgress() const;

private:
	enum Phase { Rush, Lift, Burst, PhaseCount };

	ArmPhaseCalc m_Phase;
	float m_ShotCooldown = 0.0f;
	int m_BurstRounds = 0;
};
