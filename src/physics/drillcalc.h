#pragma once

#include "physics/tentaclecalc.h"

/// @file drillcalc.h
/// @brief 固定長ドリルの突き・くい込み・押し出し（DirectX と Windows は使わない）

/// @brief ドリルの状態
enum DRILL_STATE
{
	DRILL_IDLE, ///< 待機
	DRILL_STRIKING, ///< ビットに当たる敵を探して突く
	DRILL_BORING, ///< 同じ敵にくい込んでダメージを足す
	DRILL_COOLDOWN ///< 次の突きまで休む
};

/// @brief ドリルの調整値（長さ・秒数・回数は正の値を指定する）
struct DRILL_PARAMS
{
	float BitStart; ///< 肩からビットの判定開始点までの距離
	float Reach; ///< 肩からビットの判定終点までの距離（伸びない）
	float Radius; ///< ビットの当たりの半径
	float StrikeSeconds; ///< 敵に当たらなくても突きを続ける秒数
	float BoreSeconds; ///< くい込みの上限秒数
	float TickSeconds; ///< ダメージを足す間隔（最初もこの秒数だけ待つ）
	int TickCount; ///< ダメージを足す最大回数
	float CooldownSeconds; ///< 休みの秒数
};

/// @brief 片手のドリルの計算状態
struct DRILL
{
	DRILL_STATE State; ///< 現在の状態
	VEC3 Direction; ///< 押した瞬間に決めた水平の単位方向
	float Time; ///< 今の状態になってからの秒数
	int Target; ///< くい込んだ敵の番号（なしは -1）
	int Ticks; ///< 今回のくい込みでダメージを足した回数
};

/// @brief 1フレームの出来事。状態が休みになったあとも対象を取得できる
struct DRILL_EVENT
{
	int Target; ///< ダメージ・押し出しの対象（なしは -1）
	int DamageTicks; ///< このフレームにダメージを足す回数（長いフレームでは複数）
	bool Push; ///< 最後のダメージを足したあとに押し出すか
};

/// @brief ドリルを待機に戻す（方向は +Z、敵の番号は -1）
void DrillCalcReset(DRILL& drill);

/// @brief 肩と固定した水平の向きから、ビット部分だけのカプセルを求める
/// @param origin 肩の位置
/// @param direction 向き（上下を除いて正規化。水平成分がゼロなら +Z）
/// @param params ビットの開始距離・終点距離・半径
/// @return 長さが変わらないビットのカプセル
CAPSULE DrillCalcBit(const VEC3& origin, const VEC3& direction, const DRILL_PARAMS& params);

/// @brief ドリルの突き・命中・加算回数・押し出し・休みを1フレーム進める
/// @details 命中フレームの終わりからくい込み時間を数える。くい込み中は毎フレーム同じ敵との重なりを確認する。
/// @param drill 片手の状態
/// @param origin 肩の位置（移動後の位置を使う）
/// @param aim 待機中に押した瞬間だけ使う向き（上下を除いて正規化）
/// @param trigger 入力を設定する（true：押した瞬間、false：入力なし。待機中だけ有効）
/// @param cancel 取りやめを設定する（true：復活などで押し出さず休む、false：通常更新）
/// @param dt 進める秒数（0 以下なら停止。ただし cancel は処理する）
/// @param params 調整値
/// @param targets 敵の配列（Available が true の敵だけ有効。nullptr は候補なし）
/// @param count 敵の数（敵の番号は配列の添字）
/// @return ダメージを足す回数・対象・押し出すか
DRILL_EVENT DrillCalcStep(DRILL& drill, const VEC3& origin, const VEC3& aim,
	bool trigger, bool cancel, float dt, const DRILL_PARAMS& params,
	const TENTACLE_TARGET* targets, int count);

/// @brief 水平の単位方向に上向き成分を足し、長さ 1 の押し出す向きを求める
/// @param direction ドリルの向き（上下と長さは使わない。水平成分がゼロなら +Z）
/// @param up 上向き成分（水平を 1 とした値）
VEC3 DrillCalcPushDirection(const VEC3& direction, float up);
