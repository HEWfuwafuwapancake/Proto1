#pragma once

#include "physics/tentaclecalc.h"

/// @file grapplecalc.h
/// @brief グラップリングの命中・引っぱり・着地点・モデル配置（DirectX と Windows は使わない）

/// @brief フックの状態
enum GRAPPLE_STATE
{
	GRAPPLE_IDLE, ///< 待機
	GRAPPLE_FLYING, ///< 発射した向きへ飛行中
	GRAPPLE_PULLING, ///< 敵または浮島へ引っぱられ中
	GRAPPLE_RETURNING ///< 空のフックが肩へ戻る途中
};

/// @brief グラップリングの調整値
struct GRAPPLE_PARAMS
{
	float FlySpeed; ///< 発射・回収の速さ
	float MaxDistance; ///< 発射して進める距離
	float Radius; ///< フックの当たりの半径
	float PullSpeed; ///< プレイヤーが引っぱられる速さ
	float EnemyDistance; ///< 敵の中心までの到着距離
	float StageDistance; ///< 浮島の刺さった所までの到着距離
	float MaxPullSeconds; ///< 引っぱりを続けられる秒数
};

/// @brief 1フレームの出来事（到着した敵・浮島は、戻り始めたあともこの値で分かる）
struct GRAPPLE_EVENT
{
	bool Attached; ///< このフレームで刺さったか
	bool Arrived; ///< このフレームで到着したか
	bool Released; ///< このフレームで解除したか（到着・再入力・時間切れ・取りやめ）
	int Target; ///< 刺さった、または離した敵の番号（なしは -1）
	bool OnStage; ///< 刺さった、または離した先が浮島か
	VEC3 Direction; ///< 到着したときの引っぱる向き（体当たり用）
};

/// @brief 撃つ向きを求める（ターゲットがいれば上下も含めて体の中心へ。浮島の下から撃ってもふちに刺さる）
/// @param origin 肩の位置
/// @param target ターゲットの体の中心
/// @param hasTarget ターゲットの有無を設定する（true：target へ向ける、false：fallback の水平の向き）
/// @param fallback ターゲットがいないときの向き（画面の正面。上下は使わない）
/// @return 単位方向（向きが決まらないときは +Z）
VEC3 GrappleCalcAim(const VEC3& origin, const VEC3& target, bool hasTarget, const VEC3& fallback);

/// @brief 体の中心を到着距離まで引っぱる（行き過ぎず、到着後はそれ以上動かさない）
/// @param center 体の中心（移動後の位置を書き戻す）
/// @param anchor 刺さった所
/// @param speed 1秒あたりの移動距離
/// @param distance 到着とする距離
/// @param dt 進める秒数（0 以下なら動かさない）
/// @return 到着距離以内なら true
bool GrappleCalcPull(VEC3& center, const VEC3& anchor, float speed, float distance, float dt);

/// @brief 刺さった所の XZ を浮島のふちの内側へ寄せ、上面に置いた足元の位置を求める
/// @param anchor 刺さった所
/// @param stage 浮島
/// @param inset ふちから内側へ寄せる距離（半幅・半奥行きまでに制限する）
VEC3 GrappleCalcLanding(const VEC3& anchor, const STAGE& stage, float inset);

/// @brief モデルの点を cm からワールドへ変換する（ArmDrawPiece と同じ軸・ひねり）
/// @param point OBJ 読み込み後と同じ左手系のモデル座標（cm。Maya からの Z 反転済み、X 鏡写し前）
/// @param start 付け根の原点
/// @param end 向ける先（-Y がこの方向になる）
/// @param scale cm からゲーム単位への倍率
/// @param mirrorX 鏡写しを設定する（true：X を反転、false：そのまま）
/// @param roll 長さ方向のひねり（ラジアン。描画と同じ値）
VEC3 GrappleCalcModelPoint(const VEC3& point, const VEC3& start, const VEC3& end,
	float scale, bool mirrorX, float roll);

/// @brief 片手のフックの計算状態
class GrappleCalc
{
public:
	/// @brief 指定した計算状態で初期化する（省略時は従来のゼロ初期化と同じ）
	/// @param state 現在の状態
	/// @param hook フックの原点のワールド座標
	/// @param direction 発射時に決めた単位方向
	/// @param distance 発射して進んだ距離
	/// @param target 刺さった敵の番号（なしは -1）
	/// @param onStage 浮島に刺さっているかを設定する（true：浮島、false：敵または未接続）
	/// @param anchor 刺さった所（敵の場合は毎フレーム体の中心を使う）
	/// @param pullTime 引っぱり始めてからの秒数
	GrappleCalc(GRAPPLE_STATE state = {}, VEC3 hook = {}, VEC3 direction = {}, float distance = {}, int target = {}, bool onStage = {}, VEC3 anchor = {}, float pullTime = {})
		: m_State(state), m_Hook(hook), m_Direction(direction), m_Distance(distance), m_Target(target), m_OnStage(onStage), m_Anchor(anchor), m_PullTime(pullTime) {}

	/// @brief フックを待機に戻す
	void Reset();

	/// @brief 発射・飛行・命中・引っぱり・回収を1フレーム進める。敵を先に、浮島を後に判定する
	/// @param origin 肩の位置（発射点・回収先）
	/// @param aim 発射方向（開始時だけ使い、正規化する）
	/// @param center プレイヤーの体の中心（引っぱり後の位置を書き戻す）
	/// @param trigger 入力を設定する（true：押した瞬間、false：入力なし）
	/// @param cancel 取りやめを設定する（true：復活などで即座に待機へ、false：通常更新）
	/// @param dt 進める秒数（0 以下なら停止。ただし cancel は処理する）
	/// @param params 調整値
	/// @param targets 敵の体（新しく刺さるのは Available が true の敵だけ。刺さったあとは保持状態に関係なく中心を追う）
	/// @param count 敵の数
	/// @param stage 浮島
	/// @return 刺さった・着いた・離した出来事
	GRAPPLE_EVENT Step(const VEC3& origin, const VEC3& aim, VEC3& center,
		bool trigger, bool cancel, float dt, const GRAPPLE_PARAMS& params,
		const TENTACLE_TARGET* targets, int count, const STAGE& stage);

	/// @brief 現在の状態を取得する
	GRAPPLE_STATE GetState() const { return m_State; }

	/// @brief フックの原点のワールド座標を取得する
	const VEC3& GetHook() const { return m_Hook; }

	/// @brief 発射時に決めた単位方向を取得する
	const VEC3& GetDirection() const { return m_Direction; }

	/// @brief 発射して進んだ距離を取得する
	float GetDistance() const { return m_Distance; }

	/// @brief 刺さった敵の番号（なしは -1）を取得する
	int GetTarget() const { return m_Target; }

	/// @brief 浮島に刺さっているかを取得する
	bool GetOnStage() const { return m_OnStage; }

	/// @brief 刺さった所（敵の場合は毎フレーム体の中心を使う）を取得する
	const VEC3& GetAnchor() const { return m_Anchor; }

	/// @brief 引っぱり始めてからの秒数を取得する
	float GetPullTime() const { return m_PullTime; }

private:
	void ReturnHook();
	GRAPPLE_STATE m_State{}; ///< 現在の状態
	VEC3 m_Hook{}; ///< フックの原点のワールド座標
	VEC3 m_Direction{}; ///< 発射時に決めた単位方向
	float m_Distance{}; ///< 発射して進んだ距離
	int m_Target{}; ///< 刺さった敵の番号（なしは -1）
	bool m_OnStage{}; ///< 浮島に刺さっているか
	VEC3 m_Anchor{}; ///< 刺さった所（敵の場合は毎フレーム体の中心を使う）
	float m_PullTime{}; ///< 引っぱり始めてからの秒数
};
