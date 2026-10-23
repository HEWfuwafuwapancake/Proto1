#pragma once
#include "physics/tentaclecalc.h"

/// @brief はたく動きの調整値
struct SLAP_PARAMS
{
	float Length; ///< 肩から先端までの長さ
	float Radius; ///< 腕の半径
	float Angle; ///< 中心から左右それぞれの振り角（ラジアン）
	float Duration; ///< 振る秒数（正の値）
	float Cooldown; ///< 振り終わってから待つ秒数
};

/// @brief 片腕のはたく状態。つかむ腕の状態とは別に持つ
struct SLAP
{
	bool Active; ///< 振っているか
	float Time; ///< 振り始めてからの秒数
	float Wait; ///< 次に腕を使えるまでの残り秒数
	HIT_RECORD Hits; ///< この振りで当たった相手
};

/// @brief 振っている途中と待ち時間を消す
void SlapCalcReset(SLAP& slap);
/// @brief 振っておらず、待ち時間も終わったか
bool SlapCalcCanStart(const SLAP& slap);
/// @brief 待機中なら振り始める。始められたら true
bool SlapCalcStart(SLAP& slap);
/// @brief 時間を進める。dt が0以下なら状態を変えない
void SlapCalcStep(SLAP& slap, float dt, const SLAP_PARAMS& params);
/// @brief 水平の向きを外側から内側へ回す。side は右腕で1、左腕で-1
VEC3 SlapCalcDirection(const SLAP& slap, const VEC3& forward, float side, const SLAP_PARAMS& params);
/// @brief 今の腕に初めて当たった相手の番号を1つ返す。なしは-1。繰り返し呼ぶと複数の相手に当たる
int SlapCalcHit(SLAP& slap, const CAPSULE& reach, const TENTACLE_TARGET* targets, int count);
