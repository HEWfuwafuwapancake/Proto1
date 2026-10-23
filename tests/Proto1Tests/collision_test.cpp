// physics/collision.h（球・カプセル・箱の当たり判定、HIT_RECORD）の単体テスト
#include "CppUnitTest.h"
#include "physics/collision.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace Proto1Tests
{
	static constexpr float COLLISION_EPSILON = 1.0e-4f;	// float の比較で許す誤差

	static void AssertVec3(const VEC3& expected, const VEC3& actual)
	{
		Assert::AreEqual(expected.X, actual.X, COLLISION_EPSILON);
		Assert::AreEqual(expected.Y, actual.Y, COLLISION_EPSILON);
		Assert::AreEqual(expected.Z, actual.Z, COLLISION_EPSILON);
	}

	TEST_CLASS(CollisionTest)
	{
	public:
		// ---- 線分の上のいちばん近い点 ----

		// 線分の途中・端より外（手前・奥）
		TEST_METHOD(ClosestPointOnSegment_MiddleAndEnds)
		{
			const VEC3 a = { 0.0f, 0.0f, 0.0f }, b = { 10.0f, 0.0f, 0.0f };
			AssertVec3({ 3.0f, 0.0f, 0.0f }, CollisionClosestPointOnSegment({ 3.0f, 5.0f, 0.0f }, a, b));
			AssertVec3(a, CollisionClosestPointOnSegment({ -4.0f, 1.0f, 0.0f }, a, b));
			AssertVec3(b, CollisionClosestPointOnSegment({ 14.0f, -1.0f, 2.0f }, a, b));
		}

		// 長さ 0 の線分（点）でも落ちない
		TEST_METHOD(ClosestPointOnSegment_ZeroLength)
		{
			const VEC3 a = { 1.0f, 2.0f, 3.0f };
			AssertVec3(a, CollisionClosestPointOnSegment({ 5.0f, 5.0f, 5.0f }, a, a));
		}

		// ---- 2つの線分のいちばん近い点どうし ----

		// ねじれの位置（X 向きの線分と、上にある Z 向きの線分）：交わりそうな点どうし
		TEST_METHOD(ClosestPointsOfSegments_Skew)
		{
			VEC3 onA, onB;
			CollisionClosestPointsOfSegments({ -5.0f, 0.0f, 0.0f }, { 5.0f, 0.0f, 0.0f },
				{ 2.0f, 3.0f, -5.0f }, { 2.0f, 3.0f, 5.0f }, onA, onB);
			AssertVec3({ 2.0f, 0.0f, 0.0f }, onA);
			AssertVec3({ 2.0f, 3.0f, 0.0f }, onB);
		}

		// 端どうしがいちばん近い（一直線に並んでいて、間があいている）
		TEST_METHOD(ClosestPointsOfSegments_EndToEnd)
		{
			VEC3 onA, onB;
			CollisionClosestPointsOfSegments({ 0.0f, 0.0f, 0.0f }, { 2.0f, 0.0f, 0.0f },
				{ 5.0f, 0.0f, 0.0f }, { 9.0f, 0.0f, 0.0f }, onA, onB);
			AssertVec3({ 2.0f, 0.0f, 0.0f }, onA);
			AssertVec3({ 5.0f, 0.0f, 0.0f }, onB);
		}

		// 平行（縦の線分が2本並ぶ）：いちばん近い点どうしの距離が、2本の間の距離になる
		TEST_METHOD(ClosestPointsOfSegments_Parallel)
		{
			VEC3 onA, onB;
			CollisionClosestPointsOfSegments({ 0.0f, 0.0f, 0.0f }, { 0.0f, 4.0f, 0.0f },
				{ 3.0f, 1.0f, 0.0f }, { 3.0f, 6.0f, 0.0f }, onA, onB);
			Assert::AreEqual(3.0f, Vec3Length(Vec3Sub(onA, onB)), COLLISION_EPSILON);
		}

		// ---- 球どうし ----

		// 離れている・ちょうど触れている（重なっていない扱い）・重なっている
		TEST_METHOD(SphereSphere_ApartTouchingOverlapping)
		{
			const SPHERE b = { { 0.0f, 0.0f, 0.0f }, 1.0f };
			Assert::IsFalse(CollisionSphereSphere({ { 3.0f, 0.0f, 0.0f }, 1.0f }, b).Hit);
			Assert::IsFalse(CollisionSphereSphere({ { 2.0f, 0.0f, 0.0f }, 1.0f }, b).Hit);

			HIT_RESULT hit = CollisionSphereSphere({ { 1.5f, 0.0f, 0.0f }, 1.0f }, b);
			Assert::IsTrue(hit.Hit);
			AssertVec3({ 1.0f, 0.0f, 0.0f }, hit.Normal);	// a を +X へ押し出す
			Assert::AreEqual(0.5f, hit.Depth, COLLISION_EPSILON);
		}

		// 中心が同じ点なら、真上に押し出す
		TEST_METHOD(SphereSphere_SameCenterPushesUp)
		{
			HIT_RESULT hit = CollisionSphereSphere({ { 1.0f, 1.0f, 1.0f }, 0.5f }, { { 1.0f, 1.0f, 1.0f }, 0.5f });
			Assert::IsTrue(hit.Hit);
			AssertVec3({ 0.0f, 1.0f, 0.0f }, hit.Normal);
			Assert::AreEqual(1.0f, hit.Depth, COLLISION_EPSILON);
		}

		// ---- 球とカプセル ----

		// 縦のカプセル（足元 y = 0.5、頭 y = 1.3、半径 0.5）の横・上・離れたところ
		TEST_METHOD(SphereCapsule_SideTopAndApart)
		{
			const CAPSULE body = { { 0.0f, 0.5f, 0.0f }, { 0.0f, 1.3f, 0.0f }, 0.5f };

			HIT_RESULT side = CollisionSphereCapsule({ { 0.8f, 1.0f, 0.0f }, 0.5f }, body);
			Assert::IsTrue(side.Hit);
			AssertVec3({ 1.0f, 0.0f, 0.0f }, side.Normal);
			Assert::AreEqual(0.2f, side.Depth, COLLISION_EPSILON);

			HIT_RESULT top = CollisionSphereCapsule({ { 0.0f, 2.0f, 0.0f }, 0.5f }, body);
			Assert::IsTrue(top.Hit);
			AssertVec3({ 0.0f, 1.0f, 0.0f }, top.Normal);
			Assert::AreEqual(0.3f, top.Depth, COLLISION_EPSILON);

			Assert::IsFalse(CollisionSphereCapsule({ { 0.0f, 1.0f, 1.0f }, 0.5f }, body).Hit);	// ちょうど触れている
			Assert::IsFalse(CollisionSphereCapsule({ { 3.0f, 1.0f, 0.0f }, 0.5f }, body).Hit);
		}

		// ---- カプセルどうし ----

		// 縦のカプセルが2本並ぶ（プレイヤーとダミー）：重なっていれば横に押し出す
		TEST_METHOD(CapsuleCapsule_StandingSideBySide)
		{
			const CAPSULE dummy = { { 0.0f, 0.5f, 0.0f }, { 0.0f, 1.3f, 0.0f }, 0.5f };
			HIT_RESULT hit = CollisionCapsuleCapsule({ { 0.0f, 0.5f, 0.6f }, { 0.0f, 1.3f, 0.6f }, 0.5f }, dummy);
			Assert::IsTrue(hit.Hit);
			AssertVec3({ 0.0f, 0.0f, 1.0f }, hit.Normal);
			Assert::AreEqual(0.4f, hit.Depth, COLLISION_EPSILON);

			Assert::IsFalse(CollisionCapsuleCapsule({ { 0.0f, 0.5f, 1.0f }, { 0.0f, 1.3f, 1.0f }, 0.5f }, dummy).Hit);
		}

		// 斜めのカプセル（伸ばした触腕）が、縦のカプセルに横から届くか
		TEST_METHOD(CapsuleCapsule_ArmReachesBody)
		{
			const CAPSULE body = { { 5.0f, 0.5f, 0.0f }, { 5.0f, 1.3f, 0.0f }, 0.5f };
			const CAPSULE shortArm = { { 0.0f, 1.0f, 0.0f }, { 4.0f, 1.0f, 0.0f }, 0.3f };
			const CAPSULE longArm = { { 0.0f, 1.0f, 0.0f }, { 4.5f, 1.0f, 0.0f }, 0.3f };
			Assert::IsFalse(CollisionCapsuleCapsule(shortArm, body).Hit);	// 4.0 + 0.3 + 0.5 = 4.8 < 5.0
			HIT_RESULT hit = CollisionCapsuleCapsule(longArm, body);
			Assert::IsTrue(hit.Hit);
			Assert::AreEqual(0.3f, hit.Depth, COLLISION_EPSILON);
		}

		// ---- 球と箱 ----

		// 浮島（30 × 2 × 30）の上：離れている・ちょうど触れている・めり込んでいる（上へ押し出す）
		TEST_METHOD(SphereBox_AboveStage)
		{
			const BOX stage = { { -15.0f, -2.0f, -15.0f }, { 15.0f, 0.0f, 15.0f } };
			Assert::IsFalse(CollisionSphereBox({ { 0.0f, 1.0f, 0.0f }, 0.5f }, stage).Hit);
			Assert::IsFalse(CollisionSphereBox({ { 0.0f, 0.5f, 0.0f }, 0.5f }, stage).Hit);

			HIT_RESULT hit = CollisionSphereBox({ { 3.0f, 0.2f, 4.0f }, 0.5f }, stage);
			Assert::IsTrue(hit.Hit);
			AssertVec3({ 0.0f, 1.0f, 0.0f }, hit.Normal);
			Assert::AreEqual(0.3f, hit.Depth, COLLISION_EPSILON);
		}

		// 角の外：角に向かって斜めに押し出す
		TEST_METHOD(SphereBox_Corner)
		{
			const BOX box = { { 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f } };
			HIT_RESULT hit = CollisionSphereBox({ { 1.3f, 1.4f, 0.5f }, 1.0f }, box);
			Assert::IsTrue(hit.Hit);
			AssertVec3({ 0.6f, 0.8f, 0.0f }, hit.Normal);	// (0.3, 0.4) の向き
			Assert::AreEqual(0.5f, hit.Depth, COLLISION_EPSILON);
		}

		// 中心が箱の中：いちばん近い面（この場合は右の面）の外へ押し出す
		TEST_METHOD(SphereBox_CenterInside)
		{
			const BOX box = { { 0.0f, 0.0f, 0.0f }, { 10.0f, 4.0f, 10.0f } };
			HIT_RESULT hit = CollisionSphereBox({ { 9.0f, 2.0f, 5.0f }, 0.5f }, box);
			Assert::IsTrue(hit.Hit);
			AssertVec3({ 1.0f, 0.0f, 0.0f }, hit.Normal);
			Assert::AreEqual(1.5f, hit.Depth, COLLISION_EPSILON);
		}

		// ---- HIT_RECORD ----

		// 同じ相手には2回目は当たらない。空にするとまた当たる
		TEST_METHOD(HitRecord_OncePerAttack)
		{
			HIT_RECORD record;
			HitRecordClear(record);
			Assert::IsTrue(HitRecordTryAdd(record, 2));
			Assert::IsTrue(HitRecordTryAdd(record, 5));
			Assert::IsFalse(HitRecordTryAdd(record, 2));
			Assert::IsTrue(HitRecordContains(record, 5));
			Assert::IsFalse(HitRecordContains(record, 7));

			HitRecordClear(record);
			Assert::IsFalse(HitRecordContains(record, 2));
			Assert::IsTrue(HitRecordTryAdd(record, 2));
		}

		// いっぱいになったら、新しい相手にも当てない（覚えられない相手に何度も当たらないように）
		TEST_METHOD(HitRecord_FullRejects)
		{
			HIT_RECORD record;
			HitRecordClear(record);
			for (int i = 0; i < HIT_RECORD_MAX; i++) {
				Assert::IsTrue(HitRecordTryAdd(record, i));
			}
			Assert::IsFalse(HitRecordTryAdd(record, 100));
			Assert::AreEqual(HIT_RECORD_MAX, record.Count);
		}
	};
}
