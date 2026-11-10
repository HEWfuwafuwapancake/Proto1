#pragma once

#include "physics/capsulecalc.h"

class ArmManager;
class FighterManager;
class Stage;

/// @brief GAME で拾う腕カプセルの配置・装備・描画をまとめる
class ArmCapsule
{
public:
	/// @brief 配置先のステージを受け取る
	void Initialize(const Stage& stage);
	/// @brief 試合の種を受け取り、カプセルを最初の配置に戻す
	void Reset(uint32_t seed);
	/// @brief 配置を消し、ステージの参照を外す
	void Finalize();
	/// @brief カプセルを出現させ、触れた人の空いている手に腕を装備する
	void Update(const FighterManager& fighters, ArmManager* arms);
	/// @brief 有効なカプセルを種類ごとの色で描く
	void Draw() const;

private:
	const Stage* m_Stage = nullptr;
	ArmCapsuleCalc m_Calc;
};
