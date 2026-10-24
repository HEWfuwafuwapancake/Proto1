#pragma once
#include "physics/collision.h"
#include "physics/stagecalc.h"

/// @brief 触腕の状態
enum TENTACLE_STATE
{
	TENTACLE_IDLE, ///< 待機中
	TENTACLE_EXTENDING, ///< 敵を探して伸長中
	TENTACLE_HOLDING, ///< 敵を引き寄せ・保持中
	TENTACLE_RETURNING ///< 空の腕を収縮中
};

/// @brief 触腕の調整値
struct TENTACLE_PARAMS
{
	float MaxLength; ///< 最大の長さ
	float ExtendSpeed; ///< 伸びる速さ
	float ReturnSpeed; ///< 戻る速さ
	float HoldSeconds; ///< 命中から離すまでの秒数
	float Radius; ///< 腕の半径
};

/// @brief つかむ候補（番号は配列の添字）
struct TENTACLE_TARGET
{
	CAPSULE Body; ///< 現在の体
	bool Available; ///< 生存していて、ほかの腕につかまれていないか
};

/// @brief 片腕の計算状態
struct TENTACLE
{
	TENTACLE_STATE State; ///< 現在の状態
	VEC3 Tip; ///< 腕の先端、保持中は敵の中心
	VEC3 Direction; ///< 伸ばし始めの向き
	float Length; ///< 伸びた長さ
	float HoldTime; ///< つかんでからの時間
	int Target; ///< 保持する敵の番号（なしは -1）
	HIT_RECORD Hits; ///< この伸長で当たった相手
};

/// @brief 1フレームに発生したつかみ・解放
struct TENTACLE_EVENT
{
	int Grabbed; ///< 新たにつかんだ敵（なしは -1）
	int Released; ///< 離した敵（なしは -1）
	bool Throw; ///< true：投げる、false：その場で離す
};

/// @brief 腕を待機に戻す（保持中の敵の解放は呼び出し側で行う）
void TentacleCalcReset(TENTACLE& arm);
/// @brief 肩からターゲットへ伸ばす水平の単位ベクトルを求める（距離の制限なし）
/// @param origin 腕の肩の位置
/// @param target ターゲットの中心（高さは使わない）
/// @param hasTarget ターゲットの有無を指定する（true：あり、false：なし）
/// @param fallback 対象なし・水平距離がほぼ0のときの向き（高さを除いて正規化する）
/// @return 水平の単位ベクトル。fallback の水平成分もほぼ0なら Z の正方向
VEC3 TentacleCalcAim(const VEC3& origin, const VEC3& target, bool hasTarget, const VEC3& fallback);
/// @brief 浮島を通過しないところまで移動する。半径と半高で移動する物の大きさを指定する
VEC3 TentacleCalcClip(const VEC3& from, const VEC3& to, const STAGE& stage, float radius, float halfHeight);
/// @brief 触腕を進める。trigger は押した瞬間、cancel は場外復活などによる解除。dt が0なら停止する
/// @details targets は count 個。保持対象自身は Available が false でも有効。消失時は cancel で解除する
TENTACLE_EVENT TentacleCalcStep(TENTACLE& arm, const VEC3& origin, const VEC3& aim,
	const VEC3& holdPosition, bool trigger, bool cancel, float dt, const TENTACLE_PARAMS& params,
	const TENTACLE_TARGET* targets, int count, const STAGE& stage);
