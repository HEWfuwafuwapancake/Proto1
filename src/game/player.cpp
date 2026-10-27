#include "core/gametime.h"
#include "game/camera.h"
#include "game/dummy.h"
#include "game/player.h"
#include "game/stage.h"
#include "game/arm.h"
#include "input/action.h"
#include "physics/playercalc.h"
#include "physics/cameracalc.h"
#include "render/shape.h"

#ifdef _DEBUG
#include <iostream>
#include "core/debug_ostream.h"
#endif


//------------------------------------------------------------------------------
//	調整用の値
//------------------------------------------------------------------------------
static constexpr float MOVE_SPEED = 6.0f;		// 歩く速さ（1秒に進む距離）
static constexpr float TURN_SPEED = XMConvertToRadians(720.0f);	// 歩く方向へ1秒に回る角度の上限
static constexpr float JUMP_SPEED = 9.0f;		// ジャンプした瞬間の上向きの速さ（高さは JUMP_SPEED² / (2 × GRAVITY) ＝ 約 1.6）
static constexpr float GRAVITY = 25.0f;			// 重力（1秒ごとに下向きの速さがこれだけ増える）
static constexpr float BODY_RADIUS = 0.5f;		// カプセルの半径
static constexpr float BODY_HEIGHT = 1.8f;		// カプセルの高さ
static constexpr XMFLOAT4 BODY_COLOR = { 0.95f, 0.45f, 0.35f, 1.0f };	// 色（仮。サーモンピンク）


static PlayerBodyCalc g_Body = {};	// 位置・速度・立っているか
static float g_Yaw = 0.0f;		// キャラの左右の向き（ラジアン）
static bool g_Respawned = false;
static bool g_Pulled = false;


// 浮島の真ん中に立たせる
static void Respawn(void)
{
	const STAGE& stage = StageGet();
	g_Body = {};
	g_Body.SetPosition({ stage.CenterX, stage.TopY, stage.CenterZ });
	g_Body.SetOnGround(true);
	g_Pulled = false;
	g_Yaw = 0.0f;
}


void PlayerInitialize(void)
{
	g_Respawned = false;
	Respawn();
}

void PlayerFinalize(void)
{

}

void PlayerUpdate(void)
{
	g_Respawned = false;
	// 移動の向き（前 = カメラが向いている向き。上下には傾けない）を、ワールドの向きに直す
	const ACTION_AXIS move = ActionGetMove();
	const float yaw = CameraGetYaw();
	const float forwardX = sinf(yaw), forwardZ = cosf(yaw);
	const float rightX = cosf(yaw), rightZ = -sinf(yaw);
	const float moveX = rightX * move.X + forwardX * move.Y;
	const float moveZ = rightZ * move.X + forwardZ * move.Y;

	PLAYER_PARAMS params{};
	params.MoveSpeed = MOVE_SPEED;
	params.JumpSpeed = JUMP_SPEED;
	params.Gravity = GRAVITY;
	if (!g_Pulled) {
		g_Body.Step(moveX, moveZ, ActionIsTrigger(ACTION_JUMP), GameTimeGetDeltaTime(), params, StageGet());
	}

	// 場外に落ちたら、浮島の真ん中に戻す（試合の流れは No.14 で作る）
	if (StageCalcIsOut(StageGet(), g_Body.GetY())) {
#ifdef _DEBUG
		hal::dout << "プレイヤー：場外 → 浮島の真ん中に戻した" << std::endl;
#endif
		Respawn();
		g_Respawned = true;
	}
}

void PlayerUpdateFacing(void)
{
	// カメラ更新後の向きに合わせる。復活したフレームの腕は解除される。
	if (g_Respawned) {
		g_Yaw = CameraGetYaw();
		return;
	}
	const bool armActive = ArmIsActive()
		|| ActionIsTrigger(ACTION_ARM_LEFT) || ActionIsTrigger(ACTION_ARM_RIGHT);
	const int target = CameraGetTarget();
	float targetYaw = g_Yaw;
	if (target >= 0) {
		const CAPSULE body = DummyGetTarget(target).Body;
		const VEC3 center = Vec3Scale(Vec3Add(body.Start, body.End), 0.5f);
		targetYaw = CameraCalcYawTo({ g_Body.GetX(), g_Body.GetY(), g_Body.GetZ() }, center, g_Yaw);
	}
	g_Yaw = PlayerCalcFacing(g_Yaw, g_Body.GetVelocityX(), g_Body.GetVelocityZ(), CameraGetYaw(),
		armActive, TURN_SPEED, GameTimeGetDeltaTime(), target >= 0, targetYaw);
}

float PlayerGetYaw(void)
{
	return g_Yaw;
}

void PlayerDraw(void)
{
	ShapeDrawCapsule({ g_Body.GetX(), g_Body.GetY(), g_Body.GetZ() }, BODY_RADIUS, BODY_HEIGHT, BODY_COLOR);
}

XMFLOAT3 PlayerGetPosition(void)
{
	return { g_Body.GetX(), g_Body.GetY(), g_Body.GetZ() };
}

void PlayerSetPosition(const XMFLOAT3& position)
{
	g_Body.SetPosition({ position.x, position.y, position.z });
}

void PlayerSetPulled(bool pulled)
{
	g_Pulled = pulled;
	if (pulled) {
		g_Body.SetVelocity({ 0.0f, 0.0f, 0.0f });
		g_Body.SetOnGround(false);
	}
}

float PlayerGetHeight(void)
{
	return BODY_HEIGHT;
}

bool PlayerDidRespawn(void)
{
	return g_Respawned;
}

bool PlayerIsOnGround(void)
{
	return g_Body.GetOnGround();
}
