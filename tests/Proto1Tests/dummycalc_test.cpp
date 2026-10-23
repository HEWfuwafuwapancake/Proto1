// physics/dummycalc.h（ダミーの復活の流れ・プレイヤーに押される）の単体テスト
#include "CppUnitTest.h"
#include "physics/dummycalc.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace Proto1Tests
{
	static constexpr float DUMMY_EPSILON = 1.0e-4f;	// float の比較で許す誤差
	static constexpr float PUSH_EPSILON = 0.01f;	// 押し出したあとの位置の比較で許す誤差（余分に少し離すため）
	static constexpr float DUMMY_DT = 1.0f / 60.0f;	// 1フレームの秒数（ゲームと同じ）
	static constexpr float BODY_RADIUS = 0.5f;		// 体の半径（ゲームと同じ）
	static constexpr float BODY_HEIGHT = 1.8f;		// 体の高さ（ゲームと同じ）

	// プレイヤーとダミーが重なっているか
	static bool BodiesOverlap(const VEC3& a, const VEC3& b)
	{
		return CollisionCapsuleCapsule(DummyCalcBodyCapsule(a, BODY_RADIUS, BODY_HEIGHT),
			DummyCalcBodyCapsule(b, BODY_RADIUS, BODY_HEIGHT)).Hit;
	}

	TEST_CLASS(DummyCalcTest)
	{
	public:
		// ---- 復活の流れ ----

		// 場外でなければ、何も起きない
		TEST_METHOD(Life_StaysActive)
		{
			DUMMY_LIFE life;
			DummyCalcResetLife(life);
			Assert::AreEqual((int)DUMMY_EVENT_NONE, (int)DummyCalcUpdateLife(life, false, 2.0f, DUMMY_DT));
			Assert::AreEqual((int)DUMMY_STATE_ACTIVE, (int)life.State);
			Assert::AreEqual(0, life.OutCount);
		}

		// 場外に落ちる → 2 秒待つ → 復活する。落ちた回数が 1 増える
		TEST_METHOD(Life_OutWaitRespawn)
		{
			DUMMY_LIFE life;
			DummyCalcResetLife(life);
			Assert::AreEqual((int)DUMMY_EVENT_OUT, (int)DummyCalcUpdateLife(life, true, 2.0f, DUMMY_DT));
			Assert::AreEqual((int)DUMMY_STATE_WAITING, (int)life.State);
			Assert::AreEqual(1, life.OutCount);

			// 1.9 秒（114 フレーム）たっても、まだ待っている
			for (int i = 0; i < 114; i++) {
				Assert::AreEqual((int)DUMMY_EVENT_NONE, (int)DummyCalcUpdateLife(life, true, 2.0f, DUMMY_DT));
			}
			Assert::AreEqual((int)DUMMY_STATE_WAITING, (int)life.State);

			// 2 秒をすぎたところで復活する
			DUMMY_EVENT event = DUMMY_EVENT_NONE;
			for (int i = 0; i < 10 && event == DUMMY_EVENT_NONE; i++) {
				event = DummyCalcUpdateLife(life, true, 2.0f, DUMMY_DT);
			}
			Assert::AreEqual((int)DUMMY_EVENT_RESPAWN, (int)event);
			Assert::AreEqual((int)DUMMY_STATE_ACTIVE, (int)life.State);
			Assert::AreEqual(1, life.OutCount);	// 待っている間は数えない
		}

		// 2回落ちると、落ちた回数は 2
		TEST_METHOD(Life_CountsEachFall)
		{
			DUMMY_LIFE life;
			DummyCalcResetLife(life);
			for (int fall = 0; fall < 2; fall++) {
				DummyCalcUpdateLife(life, true, 0.5f, DUMMY_DT);
				for (int i = 0; i < 60 && life.State == DUMMY_STATE_WAITING; i++) {
					DummyCalcUpdateLife(life, false, 0.5f, DUMMY_DT);
				}
			}
			Assert::AreEqual(2, life.OutCount);
		}

		// ---- 体の形 ----

		TEST_METHOD(BodyCapsule_FromFeet)
		{
			CAPSULE capsule = DummyCalcBodyCapsule({ 1.0f, 2.0f, 3.0f }, BODY_RADIUS, BODY_HEIGHT);
			Assert::AreEqual(2.5f, capsule.Start.Y, DUMMY_EPSILON);
			Assert::AreEqual(3.3f, capsule.End.Y, DUMMY_EPSILON);
			Assert::AreEqual(1.0f, capsule.Start.X, DUMMY_EPSILON);
			Assert::AreEqual(BODY_RADIUS, capsule.Radius, DUMMY_EPSILON);
		}

		// ---- プレイヤーに押される ----

		// 離れていれば、どちらも動かない
		TEST_METHOD(Push_ApartDoesNothing)
		{
			VEC3 player = { 0.0f, 0.0f, 0.0f };
			VEC3 dummy = { 0.0f, 0.0f, 2.0f };
			Assert::IsFalse(DummyCalcPushByPlayer(player, dummy, BODY_RADIUS, BODY_HEIGHT));
			Assert::AreEqual(2.0f, dummy.Z, DUMMY_EPSILON);
			Assert::AreEqual(0.0f, player.Z, DUMMY_EPSILON);
		}

		// 横から重なると、ダミーだけが押されて、重ならなくなる（プレイヤーは動かない）
		TEST_METHOD(Push_FromSideMovesDummyOnly)
		{
			VEC3 player = { 0.0f, 0.0f, 0.0f };
			VEC3 dummy = { 0.0f, 0.0f, 0.7f };
			Assert::IsTrue(DummyCalcPushByPlayer(player, dummy, BODY_RADIUS, BODY_HEIGHT));
			Assert::AreEqual(1.0f, dummy.Z, PUSH_EPSILON);	// 半径の合計（1.0）まで離れる
			Assert::AreEqual(0.0f, dummy.Y, DUMMY_EPSILON);	// 上下には動かない
			Assert::AreEqual(0.0f, player.X, DUMMY_EPSILON);
			Assert::AreEqual(0.0f, player.Z, DUMMY_EPSILON);
			Assert::IsFalse(BodiesOverlap(player, dummy));
		}

		// 少し高さがずれていても（ジャンプ中にぶつかる）、ダミーは横に押されて重ならなくなる
		TEST_METHOD(Push_SlightlyAboveStillPushesSideways)
		{
			VEC3 player = { 0.0f, 1.0f, 0.0f };
			VEC3 dummy = { 0.6f, 0.0f, 0.0f };
			Assert::IsTrue(DummyCalcPushByPlayer(player, dummy, BODY_RADIUS, BODY_HEIGHT));
			Assert::AreEqual(0.0f, dummy.Y, DUMMY_EPSILON);
			Assert::IsTrue(dummy.X > 0.6f);
			Assert::IsFalse(BodiesOverlap(player, dummy));
		}

		// 真上から重なった（ダミーの頭に乗った）ときは、プレイヤーが上に押し出される
		TEST_METHOD(Push_FromAbovePushesPlayerUp)
		{
			VEC3 player = { 0.0f, 1.5f, 0.0f };
			VEC3 dummy = { 0.0f, 0.0f, 0.0f };
			Assert::IsTrue(DummyCalcPushByPlayer(player, dummy, BODY_RADIUS, BODY_HEIGHT));
			Assert::AreEqual(0.0f, dummy.X, DUMMY_EPSILON);
			Assert::AreEqual(0.0f, dummy.Y, DUMMY_EPSILON);
			Assert::AreEqual(1.8f, player.Y, PUSH_EPSILON);	// 頭（1.3）＋半径の合計（1.0）− 足元からの半径（0.5）
		}
	};
}
