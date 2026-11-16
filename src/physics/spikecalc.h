#pragma once

#include "physics/stagecalc.h"
#include "physics/versuscalc.h"

/// @file spikecalc.h
/// @brief トゲの腕の計算：前へ手前から順に生えるトゲの列と、地面に残る茨（DirectX と Windows は使わない）

/// @brief 1列に並べるトゲの最大数
static constexpr int SPIKE_LINE_MAX = 16;
/// @brief 片手で同時に残せる茨の最大数（いっぱいなら残りの短いものから置き換える）
static constexpr int THORN_FIELD_MAX = 8;

/// @brief 前へ生えるトゲの列の設定
struct SPIKE_LINE_PARAMS
{
	float StartDistance; ///< 足元から1本目のトゲまでの水平の距離
	float Spacing; ///< トゲどうしの間隔
	int MaxCount; ///< 足場が続くときのトゲの本数（SPIKE_LINE_MAX まで）
	float SpreadSeconds; ///< 1本目が生え始めてから、最後の1本が生え始めるまでの秒数
	float RiseSeconds; ///< 1本が伸びきるまでの秒数
	float StaySeconds; ///< 伸びきったまま残る秒数
	float SinkSeconds; ///< 引っこむまでの秒数
};

/// @brief 列の1本のトゲが生え始める秒数を求める（手前ほど早い）
/// @param params 列の設定
/// @param index 手前から数えた番号（0 から）
/// @return 列を出してからの秒数
float SpikeCalcLineStart(const SPIKE_LINE_PARAMS& params, int index);

/// @brief 列の1本のトゲの伸び具合を求める
/// @param params 列の設定
/// @param index 手前から数えた番号（0 から）
/// @param elapsed 列を出してからの秒数
/// @return 0（出ていない・引っこんだ）〜 1（伸びきった）
float SpikeCalcLineGrowth(const SPIKE_LINE_PARAMS& params, int index, float elapsed);

/// @brief 列のトゲが全部引っこんだか調べる
/// @param params 列の設定
/// @param count 列のトゲの本数
/// @param elapsed 列を出してからの秒数
/// @return 全部引っこんだら true
bool SpikeCalcLineFinished(const SPIKE_LINE_PARAMS& params, int count, float elapsed);

/// @brief 列の1本のトゲの根元の位置を求める
/// @param feet 殴った人の足元
/// @param direction 列を伸ばす水平の単位方向
/// @param params 列の設定
/// @param index 手前から数えた番号（0 から）
/// @return 根元の位置（高さは足元と同じ）
VEC3 SpikeCalcLinePosition(const VEC3& feet, const VEC3& direction, const SPIKE_LINE_PARAMS& params, int index);

/// @brief 足場（浮島・高台）が続くところまでのトゲの本数を求める（足場が途切れたら、その先には生やさない）
/// @param stage ステージ
/// @param feet 殴った人の足元（この高さの足場の上に生やす）
/// @param direction 列を伸ばす水平の単位方向
/// @param params 列の設定
/// @return 生やす本数（0〜MaxCount）
int SpikeCalcLineCount(const STAGE& stage, const VEC3& feet, const VEC3& direction, const SPIKE_LINE_PARAMS& params);

/// @brief 地面に残る1つの茨
struct THORN_FIELD
{
	VEC3 Center; ///< 中心（置いた人の足元）
	float Remaining; ///< 消えるまでの残りの秒数
	float Age; ///< 置いてからの秒数（描画で生える動きに使う）
	bool Active; ///< 残っていれば true
};

/// @brief 片手の茨をまとめて管理し、中の相手が動いた距離を数える
class ThornFieldCalc
{
public:
	/// @brief 茨なし・誰も中にいない状態で初期化する
	ThornFieldCalc() = default;
	/// @brief 茨を全部消し、動いた距離の記録も消す
	void Reset();
	/// @brief 茨を置く（いっぱいなら残りの短いものと置き換える）
	/// @param center 中心（足元）
	/// @param seconds 残る秒数
	void Place(const VEC3& center, float seconds);
	/// @brief 残りの秒数を減らし、時間切れの茨を消す
	/// @param dt 進める秒数（0 以下なら何もしない）
	void Step(float dt);
	/// @brief 足元がどれかの茨の中にあるか調べる
	/// @param feet 足元
	/// @param radius 茨の水平の半径
	/// @param halfHeight 茨の中心から上下それぞれの幅
	/// @return 中にあれば true
	bool IsInside(const VEC3& feet, float radius, float halfHeight) const;
	/// @brief 相手の今の足元を記録し、前のフレームから茨の中で動いた水平の距離を返す
	/// @details 前のフレームと今のフレームの両方で中にいたときだけ数える（入った瞬間・吹っ飛び中は数えない）
	/// @param id 相手の番号（0〜FIGHTER_COUNT-1。範囲外は 0 を返す）
	/// @param feet 今の足元
	/// @param inside 今、茨の中にいて数えてよいかを設定する（true：数える、false：数えない）
	/// @return 動いた水平の距離
	float TrackMove(int id, const VEC3& feet, bool inside);
	/// @brief 茨を1つ取得する
	/// @param index 番号（0〜THORN_FIELD_MAX-1）
	const THORN_FIELD& Get(int index) const { return m_Fields[index]; }
	/// @brief 残っている茨の数を取得する
	int GetCount() const;

private:
	THORN_FIELD m_Fields[THORN_FIELD_MAX] = {}; // 茨の並び
	VEC3 m_Last[FIGHTER_COUNT] = {}; // 相手ごとの前のフレームの足元
	bool m_WasInside[FIGHTER_COUNT] = {}; // 相手ごとに、前のフレームで中にいたか
};
