#pragma once

/// @file knucklecalc.h
/// @brief ナックルの「普通の拳」と「至近の杭」の選び方（DirectX と Windows は使わない）

/// @brief ナックルの一撃の強さ
struct KNUCKLE_HIT
{
	float Damage; ///< 足すダメージ%
	float Power; ///< 吹っ飛ばす強さ
	bool Pile; ///< 杭か（true：至近の杭、false：普通の拳）
};

/// @brief ナックルの調整値
struct KNUCKLE_PARAMS
{
	float PileRange; ///< 肩から体の中心までの水平の距離がこれ以下なら杭
	KNUCKLE_HIT Normal; ///< 普通の拳の強さ（Pile は false にする）
	KNUCKLE_HIT PileHit; ///< 杭の強さ（Pile は true にする）
};

/// @brief 肩から相手の体の中心までの水平の距離で、普通の拳か杭かを選ぶ
/// @param distance 水平の距離
/// @param params 調整値
/// @return 選んだ強さ
KNUCKLE_HIT KnuckleCalcPick(float distance, const KNUCKLE_PARAMS& params);
