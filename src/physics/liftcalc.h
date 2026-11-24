#pragma once

#include "physics/stagecalc.h"
#include "physics/versuscalc.h"

/// @file liftcalc.h
/// @brief Ｇ（地上の始点の近くで△ / G を押すと、補助触腕で高所の着地点へ27更新で移る）の受付・着地点の予約・軌道
/// @details 設計書 第10節。60 Hz の対戦更新の数で進め、全員の PC で同じ位置に着く。DirectX と Windows を使わないので単体テストできる

/// @brief Ｇを使ってから、始点の真上へ浮き終わるまでの更新数
static constexpr int LIFT_RISE_FRAMES = 9;
/// @brief 着地点の真上へ移り終わる更新の番号
static constexpr int LIFT_TRAVEL_END = 24;
/// @brief Ｇで着地するまでの更新数
static constexpr int LIFT_TOTAL_FRAMES = 27;
/// @brief Ｇを使ってから次に使えるまでの更新数
static constexpr int LIFT_COOLDOWN_FRAMES = 72;
/// @brief Ｇの始点から受け付ける水平の距離
static constexpr float LIFT_ACCEPT_RADIUS = 2.0f;

/// @brief Ｇの受付と着地点の予約に使う、1人分の今の状態
struct LIFT_FIGHTER
{
	VEC3 Feet;		///< 足元の位置
	bool Present;	///< 場にいるかを設定する（true：体が場所をふさぐ、false：場外の復活待ち）
	bool Request;	///< このフレームで△ / G を押した瞬間かを設定する（true：押した、false：押していない）
	bool Eligible;	///< 起動できる状態かを設定する（true：接地して待機か通常の歩き、false：攻撃・硬直・つかまれ・ガード・ダッシュ・吹っ飛び中）
};

/// @brief 4人分のＧの動きを管理する
class LiftCalc
{
public:
	/// @brief 全員を待機に戻し、すぐ使えるようにする（試合の最初）
	void Reset();
	/// @brief 押した人の中から、使える人をスロットの順に受け付ける（1更新の最初、体を動かす前に呼ぶ）
	/// @details 受け付けた人は着地点を予約し、同じ更新の Step で n = 1 の位置へ動く
	/// @param stage ステージ（Ｇと地形）
	/// @param frame 試合の更新番号
	/// @param fighters FIGHTER_COUNT 人分の状態
	/// @return 受け付けたスロットのビット（このフレームのジャンプ・ダッシュ・攻撃などの入力は使わない）
	int Accept(const STAGE& stage, int frame, const LIFT_FIGHTER* fighters);
	/// @brief 動いている人を1更新進める（1更新の終わりに呼ぶ）
	/// @details n = 24 から 25 へ進むとき、予約した点にほかの人が入っていれば選び直し、全部ふさがれていれば落とす
	/// @param stage ステージ
	/// @param fighters FIGHTER_COUNT 人分の状態（着地点の確認に使う）
	void Step(const STAGE& stage, const LIFT_FIGHTER* fighters);
	/// @brief 被弾・つかまれで止める（その位置で予約を消し、通常の処理へ戻す）
	/// @param slot スロット番号
	void Cancel(int slot);
	/// @brief Ｇで動いている途中か取得する
	bool IsMoving(int slot) const { return m_Moving[slot]; }
	/// @brief 動いている途中の今の更新の番号 n を取得する（受け付けた更新の終わりが 1、着地が 27。待機中は 0）
	int GetStep(int slot) const { return m_Moving[slot] ? m_Step[slot] : 0; }
	/// @brief 予約した着地点を取得する（動いている途中だけ意味がある）
	VEC3 GetTarget(int slot) const { return m_Target[slot]; }
	/// @brief 今の更新の足元の位置を取得する（動いている途中だけ意味がある）
	VEC3 GetFeet(int slot) const { return m_Feet[slot]; }
	/// @brief このフレームで着地したか取得する（n = 27 に着いた。true なら接地させる）
	bool DidLand(int slot) const { return m_Landed[slot]; }
	/// @brief このフレームで、着地点が全部ふさがれて落ちたか取得する（true なら速さ 0 で通常の落下へ戻す）
	bool DidDrop(int slot) const { return m_Dropped[slot]; }
	/// @brief 使える更新になっているか取得する
	/// @param slot スロット番号
	/// @param frame 試合の更新番号
	bool IsReady(int slot, int frame) const { return frame >= m_ReadyFrame[slot]; }
	/// @brief 足元から受付の距離にあるＧを探す
	/// @param stage ステージ
	/// @param feet 足元の位置
	/// @return Ｇの番号（なければ -1。重なっていれば番号の若いほう）
	static int FindStation(const STAGE& stage, const VEC3& feet);
	/// @brief Ｇの着地点を、名目点・横・前・後ろの順に、ふさがれていない最初の点で選ぶ
	/// @param stage ステージ
	/// @param station Ｇの番号
	/// @param slot 選ぶ人のスロット番号（自分の体と予約は数えない）
	/// @param fighters FIGHTER_COUNT 人分の状態
	/// @param landing 見つかったときに着地点を書き込む先
	/// @return 見つかったら true
	bool ChooseLanding(const STAGE& stage, int station, int slot, const LIFT_FIGHTER* fighters, VEC3& landing) const;

private:
	bool IsOccupied(const VEC3& point, int slot, const LIFT_FIGHTER* fighters) const;
	VEC3 PositionAt(int slot, int step) const;

	bool m_Moving[FIGHTER_COUNT] = {};		// Ｇで動いている途中か
	int m_Step[FIGHTER_COUNT] = {};			// 今の更新の番号 n（受け付けた直後は 0）
	int m_Station[FIGHTER_COUNT] = {};		// 使っているＧ
	int m_ReadyFrame[FIGHTER_COUNT] = {};	// 次に使える更新番号
	VEC3 m_Start[FIGHTER_COUNT] = {};		// 受け付けた直前の足元 S
	VEC3 m_Target[FIGHTER_COUNT] = {};		// 予約した着地点
	VEC3 m_Feet[FIGHTER_COUNT] = {};		// 今の更新の足元
	bool m_Landed[FIGHTER_COUNT] = {};		// このフレームで着地した
	bool m_Dropped[FIGHTER_COUNT] = {};		// このフレームで落ちた
};
