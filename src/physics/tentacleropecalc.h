#pragma once
#include "physics/vec3.h"

/// @file tentacleropecalc.h
/// @brief 触腕の節をつないだ鎖の簡単な物理（重力・慣性・空気抵抗・節の長さ・体へのめり込み防止）
/// @details 描画だけに使い、当たり判定やオンラインの同期には使わない。
///          DirectX と Windows を使わないので、単体テスト（tests/Proto1Tests）からそのまま使える

/// @brief 鎖の物理の調整用の値
struct TENTACLE_ROPE_PARAMS
{
	float Gravity = 9.8f; ///< 重力（1秒ごとに下向きの速さがこれだけ増える）
	float Drag = 2.5f; ///< 空気抵抗（1秒あたりに速さを減らす割合。走ると後ろへなびく強さ）
	float Stiffness = 0.15f; ///< 曲がりにくさ（0：ひものように曲がる、1：まっすぐのまま）
	VEC3 BodyBottom{}; ///< めり込ませない体の円柱の下端
	VEC3 BodyTop{}; ///< めり込ませない体の円柱の上端
	float BodyRadius = 0.0f; ///< めり込ませない体の半径（0 以下なら体を見ない）
};

/// @brief 根元を固定した節の鎖（触腕1本分）
class TentacleRopeCalc
{
public:
	/// @brief 鎖の点の数の上限（根元を含む）
	static constexpr int MAX_POINTS = 16;

	/// @brief 根元から指定方向へまっすぐ並べ、止まった状態にする
	/// @param anchor 根元（付け根の下端）
	/// @param direction 並べる単位方向
	/// @param lengths 根元から先へ順に並べた節の長さ
	/// @param count 節の数（MAX_POINTS - 1 までに制限する）
	void Reset(const VEC3& anchor, const VEC3& direction, const float* lengths, int count);
	/// @brief 根元を動かして1フレーム進める（dt が 0 以下なら根元だけ動かす）
	/// @param anchor 今の根元
	/// @param params 調整用の値
	/// @param dt 1フレームの秒数
	void Step(const VEC3& anchor, const TENTACLE_ROPE_PARAMS& params, float dt);
	/// @brief 根元を含む点の数を取得する（Reset 前は 0）
	int GetPointCount() const { return m_Count > 0 ? m_Count + 1 : 0; }
	/// @brief 点の位置を取得する（0 が根元。範囲外は根元か先）
	VEC3 GetPoint(int index) const;
	/// @brief 根元から先までの長さの割合（0〜1）の位置を、節の上で求める
	VEC3 GetPointAt(float ratio) const;

private:
	VEC3 m_Points[MAX_POINTS] = {};
	VEC3 m_Previous[MAX_POINTS] = {};
	float m_Lengths[MAX_POINTS] = {};
	float m_Total = 0.0f;
	int m_Count = 0;
};
