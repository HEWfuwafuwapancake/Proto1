#pragma once

#include "physics/stagecalc.h"

/// @file risecalc.h
/// @brief 腕で場外から戻る動きの計算（近い足場の上の点・斜め上への突進・上への跳ね）
/// @details DirectX と Windows を使わないので、単体テスト（tests/Proto1Tests）からそのまま使える

/// @brief いちばん近い足場（浮島・高台）の上の点を探す（刀の長押しで場外から戻る先）
/// @param stage ステージ
/// @param position 今の足元の位置
/// @param inset ふちから内側へ入れる距離（足場がこれより小さければ中心）
/// @param maxDistance 上下も含めて、これより遠い点は選ばない
/// @param landing 見つかったときに書き込む足元の位置（上の面の高さ）
/// @return 見つかったら true
bool RiseCalcNearestLanding(const STAGE& stage, const VEC3& position, float inset, float maxDistance, VEC3& landing);

/// @brief 水平の向きを上へ傾けた、斜め上への移動量を求める（ナックルの空中の突進）
/// @param forward 水平の向き（長さはいくつでもよい。長さ 0 なら真上）
/// @param pitch 水平からの上向きの角度（ラジアン）
/// @param distance 進む距離
/// @return 移動量（長さが distance）
VEC3 RiseCalcSlopeOffset(const VEC3& forward, float pitch, float distance);

/// @brief 始めの位置から、決めた秒数で決めた移動量だけ自分を動かす（腕が体を直接動かすときに使う）
class RiseCalc
{
public:
	/// @brief 動いていない状態に戻す
	void Reset();
	/// @brief 動きを始める
	/// @param from 始めの足元の位置
	/// @param offset 終わりまでの移動量
	/// @param seconds かける秒数（0 以下ならすぐ終わりの位置になる）
	/// @param slowDown 進み方を設定する（true：だんだん遅くなって終わりで止まる、false：同じ速さで進む）
	void Start(const VEC3& from, const VEC3& offset, float seconds, bool slowDown);
	/// @brief 1フレーム進め、今の足元の位置を返す（終わりの位置に着いたら動きを終える）
	/// @param dt 1フレームの秒数
	/// @return 今の足元の位置
	VEC3 Step(float dt);
	/// @brief 動いている途中か取得する
	bool IsActive() const { return m_Active; }

private:
	VEC3 m_From = {}; ///< 始めの足元の位置
	VEC3 m_Offset = {}; ///< 終わりまでの移動量
	float m_Seconds = 0.0f; ///< かける秒数
	float m_Time = 0.0f; ///< 始めてからの秒数
	bool m_SlowDown = false; ///< だんだん遅くなるか
	bool m_Active = false; ///< 動いている途中か
};
