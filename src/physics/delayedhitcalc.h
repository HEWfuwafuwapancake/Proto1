#pragma once

#include "physics/vec3.h"

/// @file delayedhitcalc.h
/// @brief 当たりを覚えておき、決めた秒数のあとで効かせる計算（DirectX と Windows を使わない）

/// @brief 一度に待たせておける当たりの数
static constexpr int DELAYED_HIT_MAX = 8;

/// @brief あとから効かせる1回分の当たり
struct DELAYED_HIT
{
	int Target = -1; ///< 当たった相手のスロット番号
	VEC3 Direction = {}; ///< 吹っ飛ばす方向
	float Damage = 0.0f; ///< 足すダメージ %
	float Power = 0.0f; ///< 吹っ飛ばす強さ
	float Seconds = 0.0f; ///< 効くまでの残りの秒数
};

/// @brief 当たりを覚えて、時間が来たものを順に取り出すクラス
class DelayedHitCalc
{
public:
	/// @brief 待っている当たりをすべて消す
	void Reset();
	/// @brief 当たりを覚える
	/// @param hit 効かせる内容と、効くまでの秒数
	/// @return 覚えられたら true（いっぱいなら false）
	bool Add(const DELAYED_HIT& hit);
	/// @brief 時間を進め、時間が来た当たりを取り出す
	/// @param dt 経過秒
	/// @param out 取り出した当たりを入れる先
	/// @param maxCount out に入れられる数
	/// @return 取り出した数
	int Step(float dt, DELAYED_HIT* out, int maxCount);
	/// @brief 待っている当たりの数を取得する
	int GetCount() const { return m_Count; }
	/// @brief 待っている当たりを取得する
	/// @param index 0 から GetCount() 未満の番号
	const DELAYED_HIT& Get(int index) const { return m_Hits[index]; }

private:
	DELAYED_HIT m_Hits[DELAYED_HIT_MAX]; // 待っている当たり（前から詰める）。
	int m_Count = 0; // 待っている当たりの数。
};
