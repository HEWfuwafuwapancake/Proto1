// physics/collision.h（球・カプセル・箱の当たり判定、HitRecord）の単体テスト
#include <gtest/gtest.h>
#include "physics/collision.h"

namespace Proto1Tests
{
	static constexpr float COLLISION_EPSILON = 1.0e-4f;	// float の比較で許す誤差

	static void AssertVec3(const VEC3& expected, const VEC3& actual)
	{
		EXPECT_NEAR(expected.X, actual.X, COLLISION_EPSILON);
		EXPECT_NEAR(expected.Y, actual.Y, COLLISION_EPSILON);
		EXPECT_NEAR(expected.Z, actual.Z, COLLISION_EPSILON);
	}

	// ---- 線分の上のいちばん近い点 ----
	
	
	
	// ---- 2つの線分のいちばん近い点どうし ----
	
	
	
	
	
	// ---- 球どうし ----
	
	
	
	// ---- 球とカプセル ----
	
	
	
	// ---- カプセルどうし ----
	
	
	
	
	
	// ---- 球と箱 ----
	
	
	
	
	
	
	
	// ---- HitRecord ----

	// 線分の途中・端より外（手前・奥）
	TEST(CollisionTest, ClosestPointOnSegment_MiddleAndEnds)
	{
		const VEC3 a = { 0.0f, 0.0f, 0.0f }, b = { 10.0f, 0.0f, 0.0f };
		AssertVec3({ 3.0f, 0.0f, 0.0f }, CollisionClosestPointOnSegment({ 3.0f, 5.0f, 0.0f }, a, b));
		AssertVec3(a, CollisionClosestPointOnSegment({ -4.0f, 1.0f, 0.0f }, a, b));
		AssertVec3(b, CollisionClosestPointOnSegment({ 14.0f, -1.0f, 2.0f }, a, b));
	}

	// ねじれの位置（X 向きの線分と、上にある Z 向きの線分）：交わりそうな点どうし
	TEST(CollisionTest, ClosestPointsOfSegments_Skew)
	{
		VEC3 onA, onB;
		CollisionClosestPointsOfSegments({ -5.0f, 0.0f, 0.0f }, { 5.0f, 0.0f, 0.0f },
			{ 2.0f, 3.0f, -5.0f }, { 2.0f, 3.0f, 5.0f }, onA, onB);
		AssertVec3({ 2.0f, 0.0f, 0.0f }, onA);
		AssertVec3({ 2.0f, 3.0f, 0.0f }, onB);
	}

	// 平行（縦の線分が2本並ぶ）：いちばん近い点どうしの距離が、2本の間の距離になる
	TEST(CollisionTest, ClosestPointsOfSegments_Parallel)
	{
		VEC3 onA, onB;
		CollisionClosestPointsOfSegments({ 0.0f, 0.0f, 0.0f }, { 0.0f, 4.0f, 0.0f },
			{ 3.0f, 1.0f, 0.0f }, { 3.0f, 6.0f, 0.0f }, onA, onB);
		EXPECT_NEAR(3.0f, Vec3Length(Vec3Sub(onA, onB)), COLLISION_EPSILON);
	}

	// 離れている・ちょうど触れている（重なっていない扱い）・重なっている
	TEST(CollisionTest, SphereSphere_ApartTouchingOverlapping)
	{
		const SPHERE b = { { 0.0f, 0.0f, 0.0f }, 1.0f };
		EXPECT_FALSE(CollisionSphereSphere({ { 3.0f, 0.0f, 0.0f }, 1.0f }, b).Hit);
		EXPECT_FALSE(CollisionSphereSphere({ { 2.0f, 0.0f, 0.0f }, 1.0f }, b).Hit);

		HIT_RESULT hit = CollisionSphereSphere({ { 1.5f, 0.0f, 0.0f }, 1.0f }, b);
		EXPECT_TRUE(hit.Hit);
		AssertVec3({ 1.0f, 0.0f, 0.0f }, hit.Normal);	// a を +X へ押し出す
		EXPECT_NEAR(0.5f, hit.Depth, COLLISION_EPSILON);
	}

	// 縦のカプセル（足元 y = 0.5、頭 y = 1.3、半径 0.5）の横・上・離れたところ
	TEST(CollisionTest, SphereCapsule_SideTopAndApart)
	{
		const CAPSULE body = { { 0.0f, 0.5f, 0.0f }, { 0.0f, 1.3f, 0.0f }, 0.5f };

		HIT_RESULT side = CollisionSphereCapsule({ { 0.8f, 1.0f, 0.0f }, 0.5f }, body);
		EXPECT_TRUE(side.Hit);
		AssertVec3({ 1.0f, 0.0f, 0.0f }, side.Normal);
		EXPECT_NEAR(0.2f, side.Depth, COLLISION_EPSILON);

		HIT_RESULT top = CollisionSphereCapsule({ { 0.0f, 2.0f, 0.0f }, 0.5f }, body);
		EXPECT_TRUE(top.Hit);
		AssertVec3({ 0.0f, 1.0f, 0.0f }, top.Normal);
		EXPECT_NEAR(0.3f, top.Depth, COLLISION_EPSILON);

		EXPECT_FALSE(CollisionSphereCapsule({ { 0.0f, 1.0f, 1.0f }, 0.5f }, body).Hit);	// ちょうど触れている
		EXPECT_FALSE(CollisionSphereCapsule({ { 3.0f, 1.0f, 0.0f }, 0.5f }, body).Hit);
	}

	// 縦のカプセルが2本並ぶ（プレイヤーとダミー）：重なっていれば横に押し出す
	TEST(CollisionTest, CapsuleCapsule_StandingSideBySide)
	{
		const CAPSULE dummy = { { 0.0f, 0.5f, 0.0f }, { 0.0f, 1.3f, 0.0f }, 0.5f };
		HIT_RESULT hit = CollisionCapsuleCapsule({ { 0.0f, 0.5f, 0.6f }, { 0.0f, 1.3f, 0.6f }, 0.5f }, dummy);
		EXPECT_TRUE(hit.Hit);
		AssertVec3({ 0.0f, 0.0f, 1.0f }, hit.Normal);
		EXPECT_NEAR(0.4f, hit.Depth, COLLISION_EPSILON);

		EXPECT_FALSE(CollisionCapsuleCapsule({ { 0.0f, 0.5f, 1.0f }, { 0.0f, 1.3f, 1.0f }, 0.5f }, dummy).Hit);
	}

	// 斜めのカプセル（伸ばした触腕）が、縦のカプセルに横から届くか
	TEST(CollisionTest, CapsuleCapsule_ArmReachesBody)
	{
		const CAPSULE body = { { 5.0f, 0.5f, 0.0f }, { 5.0f, 1.3f, 0.0f }, 0.5f };
		const CAPSULE shortArm = { { 0.0f, 1.0f, 0.0f }, { 4.0f, 1.0f, 0.0f }, 0.3f };
		const CAPSULE longArm = { { 0.0f, 1.0f, 0.0f }, { 4.5f, 1.0f, 0.0f }, 0.3f };
		EXPECT_FALSE(CollisionCapsuleCapsule(shortArm, body).Hit);	// 4.0 + 0.3 + 0.5 = 4.8 < 5.0
		HIT_RESULT hit = CollisionCapsuleCapsule(longArm, body);
		EXPECT_TRUE(hit.Hit);
		EXPECT_NEAR(0.3f, hit.Depth, COLLISION_EPSILON);
	}

	// 浮島（30 × 2 × 30）の上：離れている・ちょうど触れている・めり込んでいる（上へ押し出す）
	TEST(CollisionTest, SphereBox_AboveStage)
	{
		const BOX stage = { { -15.0f, -2.0f, -15.0f }, { 15.0f, 0.0f, 15.0f } };
		EXPECT_FALSE(CollisionSphereBox({ { 0.0f, 1.0f, 0.0f }, 0.5f }, stage).Hit);
		EXPECT_FALSE(CollisionSphereBox({ { 0.0f, 0.5f, 0.0f }, 0.5f }, stage).Hit);

		HIT_RESULT hit = CollisionSphereBox({ { 3.0f, 0.2f, 4.0f }, 0.5f }, stage);
		EXPECT_TRUE(hit.Hit);
		AssertVec3({ 0.0f, 1.0f, 0.0f }, hit.Normal);
		EXPECT_NEAR(0.3f, hit.Depth, COLLISION_EPSILON);
	}

	// 角の外：角に向かって斜めに押し出す
	TEST(CollisionTest, SphereBox_Corner)
	{
		const BOX box = { { 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f } };
		HIT_RESULT hit = CollisionSphereBox({ { 1.3f, 1.4f, 0.5f }, 1.0f }, box);
		EXPECT_TRUE(hit.Hit);
		AssertVec3({ 0.6f, 0.8f, 0.0f }, hit.Normal);	// (0.3, 0.4) の向き
		EXPECT_NEAR(0.5f, hit.Depth, COLLISION_EPSILON);
	}

	// 中心が箱の中：いちばん近い面（この場合は右の面）の外へ押し出す
	TEST(CollisionTest, SphereBox_CenterInside)
	{
		const BOX box = { { 0.0f, 0.0f, 0.0f }, { 10.0f, 4.0f, 10.0f } };
		HIT_RESULT hit = CollisionSphereBox({ { 9.0f, 2.0f, 5.0f }, 0.5f }, box);
		EXPECT_TRUE(hit.Hit);
		AssertVec3({ 1.0f, 0.0f, 0.0f }, hit.Normal);
		EXPECT_NEAR(1.5f, hit.Depth, COLLISION_EPSILON);
	}

	// 同じ相手には2回目は当たらない。空にするとまた当たる
	TEST(CollisionTest, HitRecord_OncePerAttack)
	{
		HitRecord record;
		record.Clear();
		EXPECT_TRUE(record.TryAdd(2));
		EXPECT_TRUE(record.TryAdd(5));
		EXPECT_FALSE(record.TryAdd(2));
		EXPECT_TRUE(record.Contains(5));
		EXPECT_FALSE(record.Contains(7));

		record.Clear();
		EXPECT_FALSE(record.Contains(2));
		EXPECT_TRUE(record.TryAdd(2));
	}
}
