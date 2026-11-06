// physics/dummycalc.h（ダミーの復活の流れ・プレイヤーに押される）の単体テスト
#include <gtest/gtest.h>
#include "physics/dummycalc.h"

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

	// ---- 復活の流れ ----
	
	
	
	
	
	
	
	// ---- 体の形 ----
	
	
	
	// ---- プレイヤーに押される ----

	// 場外でなければ、何も起きない
	TEST(DummyCalcTest, Life_StaysActive)
	{
		DummyLifeCalc life;
		life.Reset();
		EXPECT_EQ((int)DUMMY_EVENT_NONE, (int)life.Update(false, 2.0f, DUMMY_DT));
		EXPECT_EQ((int)DUMMY_STATE_ACTIVE, (int)life.GetState());
		EXPECT_EQ(0, life.GetOutCount());
	}

	// 場外に落ちる → 2 秒待つ → 復活する。落ちた回数が 1 増える
	TEST(DummyCalcTest, Life_OutWaitRespawn)
	{
		DummyLifeCalc life;
		life.Reset();
		EXPECT_EQ((int)DUMMY_EVENT_OUT, (int)life.Update(true, 2.0f, DUMMY_DT));
		EXPECT_EQ((int)DUMMY_STATE_WAITING, (int)life.GetState());
		EXPECT_EQ(1, life.GetOutCount());

		// 1.9 秒（114 フレーム）たっても、まだ待っている
		for (int i = 0; i < 114; i++) {
			EXPECT_EQ((int)DUMMY_EVENT_NONE, (int)life.Update(true, 2.0f, DUMMY_DT));
		}
		EXPECT_EQ((int)DUMMY_STATE_WAITING, (int)life.GetState());

		// 2 秒をすぎたところで復活する
		DUMMY_EVENT event = DUMMY_EVENT_NONE;
		for (int i = 0; i < 10 && event == DUMMY_EVENT_NONE; i++) {
			event = life.Update(true, 2.0f, DUMMY_DT);
		}
		EXPECT_EQ((int)DUMMY_EVENT_RESPAWN, (int)event);
		EXPECT_EQ((int)DUMMY_STATE_ACTIVE, (int)life.GetState());
		EXPECT_EQ(1, life.GetOutCount());	// 待っている間は数えない
	}

	// 2回落ちると、落ちた回数は 2
	TEST(DummyCalcTest, Life_CountsEachFall)
	{
		DummyLifeCalc life;
		life.Reset();
		for (int fall = 0; fall < 2; fall++) {
			life.Update(true, 0.5f, DUMMY_DT);
			for (int i = 0; i < 60 && life.GetState() == DUMMY_STATE_WAITING; i++) {
				life.Update(false, 0.5f, DUMMY_DT);
			}
		}
		EXPECT_EQ(2, life.GetOutCount());
	}

	TEST(DummyCalcTest, BodyCapsule_FromFeet)
	{
		CAPSULE capsule = DummyCalcBodyCapsule({ 1.0f, 2.0f, 3.0f }, BODY_RADIUS, BODY_HEIGHT);
		EXPECT_NEAR(2.5f, capsule.Start.Y, DUMMY_EPSILON);
		EXPECT_NEAR(3.3f, capsule.End.Y, DUMMY_EPSILON);
		EXPECT_NEAR(1.0f, capsule.Start.X, DUMMY_EPSILON);
		EXPECT_NEAR(BODY_RADIUS, capsule.Radius, DUMMY_EPSILON);
	}

	// 横から重なると、ダミーだけが押されて、重ならなくなる（プレイヤーは動かない）
	TEST(DummyCalcTest, Push_FromSideMovesDummyOnly)
	{
		VEC3 player = { 0.0f, 0.0f, 0.0f };
		VEC3 dummy = { 0.0f, 0.0f, 0.7f };
		EXPECT_TRUE(DummyCalcPushByPlayer(player, dummy, BODY_RADIUS, BODY_HEIGHT));
		EXPECT_NEAR(1.0f, dummy.Z, PUSH_EPSILON);	// 半径の合計（1.0）まで離れる
		EXPECT_NEAR(0.0f, dummy.Y, DUMMY_EPSILON);	// 上下には動かない
		EXPECT_NEAR(0.0f, player.X, DUMMY_EPSILON);
		EXPECT_NEAR(0.0f, player.Z, DUMMY_EPSILON);
		EXPECT_FALSE(BodiesOverlap(player, dummy));
	}

	// 少し高さがずれていても（ジャンプ中にぶつかる）、ダミーは横に押されて重ならなくなる
	TEST(DummyCalcTest, Push_SlightlyAboveStillPushesSideways)
	{
		VEC3 player = { 0.0f, 1.0f, 0.0f };
		VEC3 dummy = { 0.6f, 0.0f, 0.0f };
		EXPECT_TRUE(DummyCalcPushByPlayer(player, dummy, BODY_RADIUS, BODY_HEIGHT));
		EXPECT_NEAR(0.0f, dummy.Y, DUMMY_EPSILON);
		EXPECT_TRUE(dummy.X > 0.6f);
		EXPECT_FALSE(BodiesOverlap(player, dummy));
	}

	// 真上から重なった（ダミーの頭に乗った）ときは、プレイヤーが上に押し出される
	TEST(DummyCalcTest, Push_FromAbovePushesPlayerUp)
	{
		VEC3 player = { 0.0f, 1.5f, 0.0f };
		VEC3 dummy = { 0.0f, 0.0f, 0.0f };
		EXPECT_TRUE(DummyCalcPushByPlayer(player, dummy, BODY_RADIUS, BODY_HEIGHT));
		EXPECT_NEAR(0.0f, dummy.X, DUMMY_EPSILON);
		EXPECT_NEAR(0.0f, dummy.Y, DUMMY_EPSILON);
		EXPECT_NEAR(1.8f, player.Y, PUSH_EPSILON);	// 頭（1.3）＋半径の合計（1.0）− 足元からの半径（0.5）
	}
}
