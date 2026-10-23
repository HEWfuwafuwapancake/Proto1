#include "core/gametime.h"
#include "game/dummy.h"
#include "game/player.h"
#include "game/stage.h"
#include "physics/dummycalc.h"
#include "physics/playercalc.h"
#include "render/shape.h"

#ifdef _DEBUG
#include <iostream>
#include "core/debug_ostream.h"
#endif


//------------------------------------------------------------------------------
//	調整用の値
//------------------------------------------------------------------------------
// 最初の場所（浮島の真ん中からの位置。プレイヤーは真ん中で +Z を向いて始まるので、前と左右の奥に置く）
static constexpr float SPAWN_X[DUMMY_COUNT] = { 0.0f, -7.0f, 7.0f };
static constexpr float SPAWN_Z[DUMMY_COUNT] = { 8.0f, 5.0f, 5.0f };
static constexpr float RESPAWN_SECONDS = 2.0f;	// 場外に落ちてから、最初の場所に戻るまでの秒数
static constexpr float BODY_RADIUS = 0.5f;		// カプセルの半径（プレイヤーと同じ）
static constexpr float BODY_HEIGHT = 1.8f;		// カプセルの高さ（プレイヤーと同じ）
static constexpr float GRAVITY = 25.0f;			// 重力（プレイヤーと同じ）
static constexpr XMFLOAT4 BODY_COLOR = { 0.35f, 0.55f, 0.95f, 1.0f };	// 色（仮。青）


// ダミー1体
struct DUMMY
{
	PLAYER_BODY Body;	// 位置・速度・立っているか（プレイヤーと同じ計算で動かす）
	DUMMY_LIFE Life;	// 場外に落ちて復活する流れ
};

static DUMMY g_Dummies[DUMMY_COUNT] = {};


// 最初の場所に立たせる
static void PlaceAtSpawn(int index)
{
	const STAGE& stage = StageGet();
	PLAYER_BODY& body = g_Dummies[index].Body;
	body = {};
	body.X = stage.CenterX + SPAWN_X[index];
	body.Y = stage.TopY;
	body.Z = stage.CenterZ + SPAWN_Z[index];
	body.OnGround = true;
}


void DummyInitialize(void)
{
	for (int i = 0; i < DUMMY_COUNT; i++) {
		PlaceAtSpawn(i);
		DummyCalcResetLife(g_Dummies[i].Life);
	}
}

void DummyFinalize(void)
{

}

void DummyUpdate(void)
{
	const float dt = GameTimeGetDeltaTime();
	const STAGE& stage = StageGet();

	PLAYER_PARAMS params{};
	params.Gravity = GRAVITY;	// 自分では動かないので、歩く速さ・ジャンプは 0

	for (int i = 0; i < DUMMY_COUNT; i++) {
		DUMMY& dummy = g_Dummies[i];

		if (dummy.Life.State == DUMMY_STATE_ACTIVE) {
			// 重力・着地（浮島の外に押し出されたら落ちる）
			PlayerCalcStep(dummy.Body, 0.0f, 0.0f, false, dt, params, stage);

			// プレイヤーに押される（真上から乗られたときは、プレイヤーのほうが押し出される）
			const XMFLOAT3 p = PlayerGetPosition();
			VEC3 playerFeet = { p.x, p.y, p.z };
			VEC3 dummyFeet = { dummy.Body.X, dummy.Body.Y, dummy.Body.Z };
			if (DummyCalcPushByPlayer(playerFeet, dummyFeet, BODY_RADIUS, BODY_HEIGHT)) {
				dummy.Body.X = dummyFeet.X;
				dummy.Body.Z = dummyFeet.Z;
				PlayerSetPosition({ playerFeet.X, playerFeet.Y, playerFeet.Z });
			}
		}

		// 場外に落ちたら待って、最初の場所に戻る
		const DUMMY_EVENT event = DummyCalcUpdateLife(dummy.Life, StageCalcIsOut(stage, dummy.Body.Y), RESPAWN_SECONDS, dt);
		if (event == DUMMY_EVENT_RESPAWN) {
			PlaceAtSpawn(i);
		}
#ifdef _DEBUG
		if (event == DUMMY_EVENT_OUT) {
			hal::dout << "ダミー" << i << "：場外（合計 " << DummyGetTotalOutCount() << " 回）" << std::endl;
		}
		if (event == DUMMY_EVENT_RESPAWN) {
			hal::dout << "ダミー" << i << "：復活" << std::endl;
		}
#endif
	}
}

void DummyDraw(void)
{
	for (int i = 0; i < DUMMY_COUNT; i++) {
		const DUMMY& dummy = g_Dummies[i];
		if (dummy.Life.State != DUMMY_STATE_ACTIVE) {
			continue;	// 復活を待っている間は見えない
		}
		ShapeDrawCapsule({ dummy.Body.X, dummy.Body.Y, dummy.Body.Z }, BODY_RADIUS, BODY_HEIGHT, BODY_COLOR);
	}
}

int DummyGetTotalOutCount(void)
{
	int total = 0;
	for (int i = 0; i < DUMMY_COUNT; i++) {
		total += g_Dummies[i].Life.OutCount;
	}
	return total;
}
