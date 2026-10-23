#include "core/gametime.h"
#include "game/camera.h"
#include "game/player.h"
#include "game/stage.h"
#include "input/action.h"
#include "physics/playercalc.h"
#include "render/shape.h"

#ifdef _DEBUG
#include <iostream>
#include "core/debug_ostream.h"
#endif


//------------------------------------------------------------------------------
//	調整用の値
//------------------------------------------------------------------------------
static constexpr float MOVE_SPEED = 6.0f;		// 歩く速さ（1秒に進む距離）
static constexpr float JUMP_SPEED = 9.0f;		// ジャンプした瞬間の上向きの速さ（高さは JUMP_SPEED² / (2 × GRAVITY) ＝ 約 1.6）
static constexpr float GRAVITY = 25.0f;			// 重力（1秒ごとに下向きの速さがこれだけ増える）
static constexpr float BODY_RADIUS = 0.5f;		// カプセルの半径
static constexpr float BODY_HEIGHT = 1.8f;		// カプセルの高さ
static constexpr XMFLOAT4 BODY_COLOR = { 0.95f, 0.45f, 0.35f, 1.0f };	// 色（仮。サーモンピンク）


static PLAYER_BODY g_Body = {};	// 位置・速度・立っているか


// 浮島の真ん中に立たせる
static void Respawn(void)
{
	const STAGE& stage = StageGet();
	g_Body = {};
	g_Body.X = stage.CenterX;
	g_Body.Y = stage.TopY;
	g_Body.Z = stage.CenterZ;
	g_Body.OnGround = true;
}


void PlayerInitialize(void)
{
	Respawn();
}

void PlayerFinalize(void)
{

}

void PlayerUpdate(void)
{
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
	PlayerCalcStep(g_Body, moveX, moveZ, ActionIsTrigger(ACTION_JUMP), GameTimeGetDeltaTime(), params, StageGet());

	// 場外に落ちたら、浮島の真ん中に戻す（試合の流れは No.14 で作る）
	if (StageCalcIsOut(StageGet(), g_Body.Y)) {
#ifdef _DEBUG
		hal::dout << "プレイヤー：場外 → 浮島の真ん中に戻した" << std::endl;
#endif
		Respawn();
	}
}

void PlayerDraw(void)
{
	ShapeDrawCapsule({ g_Body.X, g_Body.Y, g_Body.Z }, BODY_RADIUS, BODY_HEIGHT, BODY_COLOR);
}

XMFLOAT3 PlayerGetPosition(void)
{
	return { g_Body.X, g_Body.Y, g_Body.Z };
}

void PlayerSetPosition(const XMFLOAT3& position)
{
	g_Body.X = position.x;
	g_Body.Y = position.y;
	g_Body.Z = position.z;
}

float PlayerGetHeight(void)
{
	return BODY_HEIGHT;
}
