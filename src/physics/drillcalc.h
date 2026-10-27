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

/// @brief 元の Maya モデルの握り（OBJ 読み込み時の Z 反転前、cm）
struct DRILL_GRIP
{
	VEC3 BitRear; ///< 肩 pivot からビットの軸後端へのモデル座標
	VEC3 AxisDirection; ///< 後端から先端へのモデル単位方向（X は 0、Y は負、Z は正）
	float Scale; ///< モデルの cm からワールド座標への倍率
};

/// @brief 描画と当たり判定で共有するビットの軸
struct DRILL_AXIS
{
	VEC3 Rear; ///< ビットの軸後端のワールド座標
	VEC3 Direction; ///< 後端から先端へ向かうワールド単位方向
};

/// @brief ドリルの調整値（長さ・秒数・回数は正の値を指定する）
struct DRILL_PARAMS
{
	DRILL_GRIP Grip; ///< 元のモデルの握りと大きさ
	float HitStart; ///< ビットの軸後端から判定開始点までの cm
	float HitEnd; ///< ビットの軸後端から判定終点までの cm（伸びない）
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

/// @brief 付け根を 180° ひねった描画と同じ座標変換でビットの軸を求める
/// @param shoulder 肩のワールド座標
/// @param armDirection 肩から腕が伸びる向き
/// @param grip Z 反転前の握りと大きさ
/// @param mirrored 鏡写しを設定する（true：右手として X を反転、false：左手）
/// @return ビットの後端と先端へ向かう単位方向
DRILL_AXIS DrillCalcAxis(const VEC3& shoulder, const VEC3& armDirection,
	const DRILL_GRIP& grip, bool mirrored);

/// @brief ビットの軸を指定した水平方向へ向ける腕の単位方向を求める
/// @param direction ドリルの水平の向き（Y は使わない。水平成分がゼロなら +Z）
/// @param grip 元のモデルの握り（軸の Y・Z から腕の傾きを求める）
/// @return 握りの角度だけ下へ傾けた腕の単位方向
VEC3 DrillCalcThrustArmDirection(const VEC3& direction, const DRILL_GRIP& grip);

/// @brief 元の握りで配置したビットの軸上に、判定用カプセルを求める
/// @param shoulder 肩の位置
/// @param armDirection 肩から腕が伸びる向き
/// @param mirrored 鏡写しを設定する（true：右手、false：左手）
/// @param params 握り・軸後端からの開始 cm・終点 cm・ワールド半径
/// @return 長さが変わらないビットのカプセル
CAPSULE DrillCalcBit(const VEC3& shoulder, const VEC3& armDirection, bool mirrored,
	const DRILL_PARAMS& params);

/// @brief ドリルの突き・命中・加算回数・押し出し・休みを1フレーム進める
/// @details 命中フレームの終わりからくい込み時間を数える。くい込み中は毎フレーム同じ敵との重なりを確認する。
/// @param drill 片手の状態
/// @param shoulder 肩の位置（移動後の位置を使う）
/// @param aim 待機中に押した瞬間だけ使う向き（上下を除いて正規化）
/// @param mirrored 鏡写しを設定する（true：右手、false：左手）
/// @param trigger 入力を設定する（true：押した瞬間、false：入力なし。待機中だけ有効）
/// @param cancel 取りやめを設定する（true：復活などで押し出さず休む、false：通常更新）
/// @param dt 進める秒数（0 以下なら停止。ただし cancel は処理する）
/// @param params 調整値
/// @param targets 敵の配列（Available が true の敵だけ有効。nullptr は候補なし）
/// @param count 敵の数（敵の番号は配列の添字）
/// @return ダメージを足す回数・対象・押し出すか
DRILL_EVENT DrillCalcStep(DRILL& drill, const VEC3& shoulder, const VEC3& aim, bool mirrored,
	bool trigger, bool cancel, float dt, const DRILL_PARAMS& params,
	const TENTACLE_TARGET* targets, int count);

/// @brief 水平の単位方向に上向き成分を足し、長さ 1 の押し出す向きを求める
/// @param direction ドリルの向き（上下と長さは使わない。水平成分がゼロなら +Z）
/// @param up 上向き成分（水平を 1 とした値）
VEC3 DrillCalcPushDirection(const VEC3& direction, float up);
