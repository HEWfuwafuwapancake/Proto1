#pragma once

#include "physics/vec3.h"

/// @file collision.h
/// @brief 当たり判定の共通部品（球・カプセル・箱どうしが重なっているか、押し出す向きと深さ）と、
///        1回の攻撃で同じ相手に何度も当たらないようにする記録（HitRecord）
/// @details DirectX と Windows を使わないので、単体テスト（tests/Proto1Tests）からそのまま使える。
///          「ちょうど触れている」（距離 = 半径の合計）は重なっていない扱いにする

/// @brief 球
struct SPHERE
{
	VEC3 Center;	///< 中心
	float Radius;	///< 半径
};

/// @brief カプセル（線分のまわりに半径をつけた形。縦向きでも斜めでもよい。プレイヤー・ダミー・触腕に使う）
struct CAPSULE
{
	VEC3 Start;		///< 線分の片方の端（半球の中心）
	VEC3 End;		///< 線分のもう片方の端（半球の中心）
	float Radius;	///< 半径
};

/// @brief 箱（向きなし。各辺が X・Y・Z の軸にそろった直方体。浮島など）
struct BOX
{
	VEC3 Min;	///< いちばん小さい角（X・Y・Z それぞれの最小）
	VEC3 Max;	///< いちばん大きい角（X・Y・Z それぞれの最大）
};

/// @brief 当たり判定の結果
struct HIT_RESULT
{
	bool Hit;		///< 重なっているか
	VEC3 Normal;	///< 1つ目の形を2つ目の形から押し出す向き（長さ 1。重なっていないときは 0）
	float Depth;	///< 重なっている深さ（Normal の向きにこれだけ動かすと離れる。重なっていないときは 0）
};

/// @brief 線分の上で、ある点にいちばん近い点を求める
/// @param p 点
/// @param a 線分の片方の端
/// @param b 線分のもう片方の端（a と同じ点なら a を返す）
/// @return 線分の上でいちばん近い点
VEC3 CollisionClosestPointOnSegment(const VEC3& p, const VEC3& a, const VEC3& b);

/// @brief 2つの線分の、いちばん近い点どうしを求める
/// @details 平行なときは、いちばん近い点の組がたくさんあるので、そのうちの1組を返す
/// @param a0 1つ目の線分の片方の端
/// @param a1 1つ目の線分のもう片方の端
/// @param b0 2つ目の線分の片方の端
/// @param b1 2つ目の線分のもう片方の端
/// @param[out] onA 1つ目の線分の上の点
/// @param[out] onB 2つ目の線分の上の点
void CollisionClosestPointsOfSegments(const VEC3& a0, const VEC3& a1, const VEC3& b0, const VEC3& b1, VEC3& onA, VEC3& onB);

/// @brief 球どうしが重なっているか調べる
/// @param a 1つ目の球（押し出される方）
/// @param b 2つ目の球
/// @return 結果（中心が同じ点のときは、真上に押し出す）
HIT_RESULT CollisionSphereSphere(const SPHERE& a, const SPHERE& b);

/// @brief 球とカプセルが重なっているか調べる
/// @param a 球（押し出される方）
/// @param b カプセル
/// @return 結果（球の中心がカプセルの線分の上にあるときは、真上に押し出す）
HIT_RESULT CollisionSphereCapsule(const SPHERE& a, const CAPSULE& b);

/// @brief カプセルどうしが重なっているか調べる
/// @param a 1つ目のカプセル（押し出される方）
/// @param b 2つ目のカプセル
/// @return 結果（線分どうしが交わっているときは、真上に押し出す）
HIT_RESULT CollisionCapsuleCapsule(const CAPSULE& a, const CAPSULE& b);

/// @brief 球と箱が重なっているか調べる
/// @details 球の中心が箱の中にあるときは、いちばん近い面の外へ押し出す
/// @param a 球（押し出される方）
/// @param b 箱
/// @return 結果
HIT_RESULT CollisionSphereBox(const SPHERE& a, const BOX& b);

/// @brief HitRecord に覚えておける相手の数
static constexpr int HIT_RECORD_MAX = 8;

/// @brief 1回の攻撃で、もう当たった相手の記録（同じ相手に何度も当たらないようにする）
class HitRecord
{
public:
	/// @brief 記録を空にする（攻撃を始めるときに呼ぶ）
	void Clear();

	/// @brief 相手にもう当たったか調べる
	/// @param id 相手の番号
	/// @return もう当たっていれば true
	bool Contains(int id) const;

	/// @brief 相手に当てる（初めてなら覚えて true を返す）
	/// @details もう当たっている相手と、記録がいっぱいのときは false（当てない）
	/// @param id 相手の番号
	/// @return この攻撃で初めて当たったなら true（ダメージや吹っ飛びを与えてよい）
	bool TryAdd(int id);

	/// @brief 覚えている相手の数（0〜HIT_RECORD_MAX）を取得する
	int GetCount() const { return m_Count; }

private:
	int m_Count{}; ///< 覚えている相手の数（0〜HIT_RECORD_MAX）
	int m_Ids[HIT_RECORD_MAX]{}; ///< 覚えている相手の番号（Count 個だけ使う）
};
