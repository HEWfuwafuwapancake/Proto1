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

/// @brief 1フレームに発生したつかみ・解放
struct TENTACLE_EVENT
{
	int Grabbed; ///< 新たにつかんだ敵（なしは -1）
	int Released; ///< 離した敵（なしは -1）
	bool Throw; ///< true：投げる、false：その場で離す
};

/// @brief モデルを配置する直交単位軸（-Y が腕の進む向き、-Z が下、X が横）
struct TENTACLE_PIECE_AXES
{
	VEC3 X; ///< モデルの X 軸
	VEC3 Y; ///< モデルの Y 軸
	VEC3 Z; ///< モデルの Z 軸
};

/// @brief 肩と先端を固定して、腕に直角な2方向へ揺らした描画用の点を求める
/// @param origin 肩
/// @param tip 先端（肩と同じなら肩を返す）
/// @param s 肩から先端までの割合（0〜1 に制限する）
/// @param phase 揺れの位相（ラジアン）
/// @param amplitude 横揺れの大きさ（縦は半分）
/// @param waves 腕全体に入れる波の数
VEC3 TentacleCalcWigglePoint(const VEC3& origin, const VEC3& tip, float s,
	float phase, float amplitude, float waves);
/// @brief -Y を始点から終点へ、-Z を指定方向へなるべく向ける直交単位軸を求める
/// @param start 始点
/// @param end 終点（始点と同じなら +Z へ向ける）
/// @param down 吸盤側へ向けたい方向。腕と平行・ゼロなら補助軸で姿勢を決める
TENTACLE_PIECE_AXES TentacleCalcPieceAxes(const VEC3& start, const VEC3& end, const VEC3& down);

/// @brief 肩からターゲットへ伸ばす水平の単位ベクトルを求める（距離の制限なし）
/// @param origin 腕の肩の位置
/// @param target ターゲットの中心（高さは使わない）
/// @param hasTarget ターゲットの有無を指定する（true：あり、false：なし）
/// @param fallback 対象なし・水平距離がほぼ0のときの向き（高さを除いて正規化する）
/// @return 水平の単位ベクトル。fallback の水平成分もほぼ0なら Z の正方向
VEC3 TentacleCalcAim(const VEC3& origin, const VEC3& target, bool hasTarget, const VEC3& fallback);

/// @brief 浮島を通過しないところまで移動する。半径と半高で移動する物の大きさを指定する
VEC3 TentacleCalcClip(const VEC3& from, const VEC3& to, const STAGE& stage, float radius, float halfHeight);

/// @brief 片腕の計算状態
class TentacleCalc
{
public:
	/// @brief 指定した計算状態で初期化する（省略時は従来のゼロ初期化と同じ）
	/// @param state 現在の状態
	/// @param tip 腕の先端、保持中は敵の中心
	/// @param direction 伸ばし始めの向き
	/// @param length 伸びた長さ
	/// @param holdTime つかんでからの時間
	/// @param target 保持する敵の番号（なしは -1）
	/// @param hits この伸長で当たった相手
	TentacleCalc(TENTACLE_STATE state = {}, VEC3 tip = {}, VEC3 direction = {}, float length = {}, float holdTime = {}, int target = {}, HitRecord hits = {})
		: m_State(state), m_Tip(tip), m_Direction(direction), m_Length(length), m_HoldTime(holdTime), m_Target(target), m_Hits(hits) {}

	/// @brief 腕を待機に戻す（保持中の敵の解放は呼び出し側で行う）
	void Reset();

	/// @brief 伸長中の腕が、最大の長さのどこまで伸びたかを求める（伸びるほど強くなる振動に使う）
	/// @param params 調整値（MaxLength を使う）
	/// @return 伸長中なら 0〜1（範囲外は切る）。伸長中でない・最大の長さが 0 以下なら 0
	float GetExtendRatio(const TENTACLE_PARAMS& params) const;

	/// @brief 触腕を進める。trigger は押した瞬間、cancel は場外復活などによる解除。dt が0なら停止する
	/// @details targets は count 個。保持対象自身は Available が false でも有効。消失時は cancel で解除する
	/// @param trigger 入力を設定する（true：押した瞬間、false：入力なし）
	/// @param cancel 解除を設定する（true：復活などで待機に戻す、false：通常更新）
	TENTACLE_EVENT Step(const VEC3& origin, const VEC3& aim,
		const VEC3& holdPosition, bool trigger, bool cancel, float dt, const TENTACLE_PARAMS& params,
		const TENTACLE_TARGET* targets, int count, const STAGE& stage);

	/// @brief 現在の状態を取得する
	TENTACLE_STATE GetState() const { return m_State; }

	/// @brief 腕の先端、保持中は敵の中心を取得する
	const VEC3& GetTip() const { return m_Tip; }

	/// @brief 伸ばし始めの向きを取得する
	const VEC3& GetDirection() const { return m_Direction; }

	/// @brief 伸びた長さを取得する
	float GetLength() const { return m_Length; }

	/// @brief つかんでからの時間を取得する
	float GetHoldTime() const { return m_HoldTime; }

	/// @brief 保持する敵の番号（なしは -1）を取得する
	int GetTarget() const { return m_Target; }

	/// @brief この伸長で当たった相手を取得する
	const HitRecord& GetHits() const { return m_Hits; }

private:
	TENTACLE_STATE m_State{}; ///< 現在の状態
	VEC3 m_Tip{}; ///< 腕の先端、保持中は敵の中心
	VEC3 m_Direction{}; ///< 伸ばし始めの向き
	float m_Length{}; ///< 伸びた長さ
	float m_HoldTime{}; ///< つかんでからの時間
	int m_Target{}; ///< 保持する敵の番号（なしは -1）
	HitRecord m_Hits{}; ///< この伸長で当たった相手
};
