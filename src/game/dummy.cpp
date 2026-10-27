#include "core/gametime.h"
#include "game/camera.h"
#include "game/dummy.h"
#include "game/player.h"
#include "game/stage.h"
#include "physics/dummycalc.h"
#include "physics/knockbackcalc.h"
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
static constexpr XMFLOAT4 BODY_COLOR = { 0.35f, 0.55f, 0.95f, 1.0f };		// 色（仮。青）
static constexpr XMFLOAT4 TARGET_COLOR = { 1.0f, 0.55f, 0.15f, 1.0f };		// ターゲットの色（明るいオレンジ）
static constexpr XMFLOAT4 LAUNCHED_COLOR = { 0.95f, 0.85f, 0.3f, 1.0f };	// 吹っ飛ばされている間の色（仮。黄色）

// 吹っ飛び（スマブラ式：% が高いほど速く＝遠くへ飛ぶ。No.19 で試しながら調整する）
static constexpr float KNOCKBACK_BASE_SPEED = 4.0f;		// 0% のときに吹っ飛ぶ速さ（攻撃の強さ 1 のとき。1秒に進む距離）
static constexpr float KNOCKBACK_GROWTH = 0.12f;		// ダメージ 1% ごとに増える、吹っ飛ぶ速さ
static constexpr float KNOCKBACK_ANGLE = XMConvertToRadians(30.0f);	// 吹っ飛ぶ向きの、水平からの上向きの角度
static constexpr float KNOCKBACK_AIR_DRAG = 0.5f;		// 空気の抵抗（空中で、横の速さが 1秒ごとに e^(-0.5) ＝ 約 0.6 倍）
static constexpr float KNOCKBACK_FRICTION = 20.0f;		// 地面の摩擦（浮島の上をすべる間、横の速さが 1秒ごとにこれだけ減る）
static constexpr float KNOCKBACK_STOP_SPEED = 0.5f;		// 浮島の上でこれより遅くなったら止まる



// ダミー1体
struct DUMMY
{
	PlayerBodyCalc Body;	// 位置・速度・立っているか（立っている間はプレイヤーと同じ計算で動かす）
	DummyLifeCalc Life;	// 場外に落ちて復活する流れ
	float Percent;		// ダメージ%（復活したら 0 に戻る）
	bool Launched;		// 吹っ飛ばされているか（自分では止まれない。浮島の上で止まったら false）
	bool Held;          // 触腕が保持しているか
};

static DUMMY g_Dummies[DUMMY_COUNT] = {};


// 吹っ飛びの調整用の値
static KNOCKBACK_PARAMS KnockbackParams(void)
{
	KNOCKBACK_PARAMS params{};
	params.BaseSpeed = KNOCKBACK_BASE_SPEED;
	params.Growth = KNOCKBACK_GROWTH;
	params.LaunchAngle = KNOCKBACK_ANGLE;
	params.AirDrag = KNOCKBACK_AIR_DRAG;
	params.GroundFriction = KNOCKBACK_FRICTION;
	params.StopSpeed = KNOCKBACK_STOP_SPEED;
	params.Gravity = GRAVITY;
	return params;
}

// 最初の場所に立たせ、ダメージ%を 0 に戻す
static void PlaceAtSpawn(int index)
{
	const STAGE& stage = StageGet();
	DUMMY& dummy = g_Dummies[index];
	dummy.Body = {};
	dummy.Body.SetPosition({ stage.CenterX + SPAWN_X[index], stage.TopY, stage.CenterZ + SPAWN_Z[index] });
	dummy.Body.SetOnGround(true);
	dummy.Percent = 0.0f;
	dummy.Launched = false;
	dummy.Held = false;
}



void DummyInitialize(void)
{
	for (int i = 0; i < DUMMY_COUNT; i++) {
		PlaceAtSpawn(i);
		g_Dummies[i].Life.Reset();
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

		if (dummy.Life.GetState() == DUMMY_STATE_ACTIVE && !dummy.Held) {
			if (dummy.Launched) {
				// 吹っ飛ばされている：自分では止まれない（プレイヤーにも押されない）
				dummy.Launched = KnockbackCalcStep(dummy.Body, dt, KnockbackParams(), stage);
			}
			else {
				// 立っている：重力・着地（浮島の外に押し出されたら落ちる）
				dummy.Body.Step(0.0f, 0.0f, false, dt, params, stage);

				// プレイヤーに押される（真上から乗られたときは、プレイヤーのほうが押し出される）
				const XMFLOAT3 p = PlayerGetPosition();
				VEC3 playerFeet = { p.x, p.y, p.z };
				VEC3 dummyFeet = { dummy.Body.GetX(), dummy.Body.GetY(), dummy.Body.GetZ() };
				if (DummyCalcPushByPlayer(playerFeet, dummyFeet, BODY_RADIUS, BODY_HEIGHT)) {
					dummy.Body.SetPosition({ dummyFeet.X, dummy.Body.GetY(), dummy.Body.GetZ() });
					dummy.Body.SetPosition({ dummy.Body.GetX(), dummy.Body.GetY(), dummyFeet.Z });
					PlayerSetPosition({ playerFeet.X, playerFeet.Y, playerFeet.Z });
				}
			}
		}

		// 場外に落ちたら待って、最初の場所に戻る（ダメージ%も 0 に戻る）
		const DUMMY_EVENT event = dummy.Life.Update(StageCalcIsOut(stage, dummy.Body.GetY()), RESPAWN_SECONDS, dt);
		if (event == DUMMY_EVENT_RESPAWN) {
			PlaceAtSpawn(i);
		}
#ifdef _DEBUG
		if (event == DUMMY_EVENT_OUT) {
			hal::dout << "ダミー" << i << "：場外（合計 " << DummyGetTotalOutCount() << " 回）" << std::endl;
		}
		if (event == DUMMY_EVENT_RESPAWN) {
			hal::dout << "ダミー" << i << "：復活（0%）" << std::endl;
		}
#endif
	}
}

void DummyDraw(void)
{
	const int target = CameraGetTarget();
	for (int i = 0; i < DUMMY_COUNT; i++) {
		const DUMMY& dummy = g_Dummies[i];
		if (dummy.Life.GetState() != DUMMY_STATE_ACTIVE) {
			continue;	// 復活を待っている間は見えない
		}
		const XMFLOAT4& color = dummy.Launched ? LAUNCHED_COLOR
			: (i == target ? TARGET_COLOR : BODY_COLOR);
		ShapeDrawCapsule({ dummy.Body.GetX(), dummy.Body.GetY(), dummy.Body.GetZ() }, BODY_RADIUS, BODY_HEIGHT,
			color);
	}
}

void DummyHit(int index, const XMFLOAT3& direction, float damage, float power)
{
	if (index < 0 || index >= DUMMY_COUNT || g_Dummies[index].Life.GetState() != DUMMY_STATE_ACTIVE) {
		return;
	}
	DUMMY& dummy = g_Dummies[index];
	const KNOCKBACK_PARAMS params = KnockbackParams();

	// スマブラと同じく、ダメージを足してから、足したあとの % で吹っ飛ぶ速さを決める
	dummy.Percent = KnockbackCalcAddDamage(dummy.Percent, damage);
	const float speed = KnockbackCalcSpeed(dummy.Percent, power, params);
	const VEC3 velocity = KnockbackCalcVelocity({ direction.x, direction.y, direction.z }, speed, params);
	dummy.Body.SetVelocity({ velocity.X, velocity.Y, velocity.Z });
	dummy.Body.SetOnGround(false);
	dummy.Launched = true;

#ifdef _DEBUG
	hal::dout << "ダミー" << index << "：" << dummy.Percent << "%・速さ " << speed << std::endl;
#endif
}

void DummyAddDamage(int index, float damage)
{
	if (!DummyIsActive(index)) return;
	DUMMY& dummy = g_Dummies[index];
	dummy.Percent = KnockbackCalcAddDamage(dummy.Percent, damage);
}

void DummyLaunch(int index, const VEC3& direction, float damage, float power)
{
	if (!DummyIsActive(index)) return;
	DUMMY& dummy = g_Dummies[index];
	DummyAddDamage(index, damage);
	const float speed = KnockbackCalcSpeed(dummy.Percent, power, KnockbackParams());
	const VEC3 velocity = KnockbackCalcDirectedVelocity(direction, speed);
	dummy.Body.SetVelocity({ velocity.X, velocity.Y, velocity.Z });
	dummy.Body.SetOnGround(false);
	dummy.Held = false;
	dummy.Launched = true;
}

int DummyGetTotalOutCount(void)
{
	int total = 0;
	for (int i = 0; i < DUMMY_COUNT; i++) {
		total += g_Dummies[i].Life.GetOutCount();
	}
	return total;
}

float DummyGetPercent(int index)
{
	if (index < 0 || index >= DUMMY_COUNT) return 0.0f;
	return g_Dummies[index].Percent;
}

bool DummyIsActive(int index)
{
	return index >= 0 && index < DUMMY_COUNT && g_Dummies[index].Life.GetState() == DUMMY_STATE_ACTIVE;
}

TENTACLE_TARGET DummyGetTarget(int index)
{
	if (!DummyIsActive(index)) return {};
	const DUMMY& dummy = g_Dummies[index];
	const VEC3 feet{ dummy.Body.GetX(), dummy.Body.GetY(), dummy.Body.GetZ() };
	return { DummyCalcBodyCapsule(feet, BODY_RADIUS, BODY_HEIGHT), !dummy.Held };
}

bool DummyGrab(int index)
{
	if (!DummyIsActive(index) || g_Dummies[index].Held) return false;
	DUMMY& dummy = g_Dummies[index];
	dummy.Held = true;
	dummy.Launched = false;
	dummy.Body.SetVelocity({ 0.0f, 0.0f, 0.0f });
	dummy.Body.SetOnGround(false);
	return true;
}

void DummyMoveHeld(int index, const VEC3& center)
{
	if (!DummyIsActive(index) || !g_Dummies[index].Held) return;
	DUMMY& dummy = g_Dummies[index];
	dummy.Body.SetPosition({ center.X, center.Y - BODY_HEIGHT * 0.5f, center.Z });
}

void DummyRelease(int index, bool throwTarget, const VEC3& direction, float damage, float power)
{
	if (index < 0 || index >= DUMMY_COUNT) return;
	DUMMY& dummy = g_Dummies[index];
	dummy.Held = false;
	if (!DummyIsActive(index) || !throwTarget) return;
	// ダメージと速さは従来の攻撃と同じ経路、投げる方向だけ3Dにする。
	DummyHit(index, { direction.X, direction.Y, direction.Z }, damage, power);
	const float speed = KnockbackCalcSpeed(dummy.Percent, power, KnockbackParams());
	const VEC3 velocity = KnockbackCalcDirectedVelocity(direction, speed);
	dummy.Body.SetVelocity({ velocity.X, velocity.Y, velocity.Z });
}
