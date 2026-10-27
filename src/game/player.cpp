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


// 浮島の真ん中に立たせる
void Player::Respawn(void)
{
	const STAGE& stage = m_Stage->GetStage();
	m_Body = {};
	m_Body.SetPosition({ stage.CenterX, stage.TopY, stage.CenterZ });
	m_Body.SetOnGround(true);
	m_Pulled = false;
	m_Yaw = 0.0f;
}


void Player::Reset(void)
{
	m_Respawned = false;
	Respawn();
}

void Player::Finalize(void)
{

}

void Player::Update(void)
{
	m_Respawned = false;
	// 移動の向き（前 = カメラが向いている向き。上下には傾けない）を、ワールドの向きに直す
	const ACTION_AXIS move = ActionGetMove();
	const float yaw = m_Camera->GetYaw();
	const float forwardX = sinf(yaw), forwardZ = cosf(yaw);
	const float rightX = cosf(yaw), rightZ = -sinf(yaw);
	const float moveX = rightX * move.X + forwardX * move.Y;
	const float moveZ = rightZ * move.X + forwardZ * move.Y;

	PLAYER_PARAMS params{};
	params.MoveSpeed = MOVE_SPEED * m_Arms->GetMoveScale();
	params.JumpSpeed = JUMP_SPEED;
	params.Gravity = GRAVITY;
	if (!m_Pulled) {
		m_Body.Step(moveX, moveZ, ActionIsTrigger(ACTION_JUMP), GameTimeGetDeltaTime(), params, m_Stage->GetStage());
	}

	// 場外に落ちたら、浮島の真ん中に戻す（試合の流れは No.14 で作る）
	if (StageCalcIsOut(m_Stage->GetStage(), m_Body.GetY())) {
#ifdef _DEBUG
		hal::dout << "プレイヤー：場外 → 浮島の真ん中に戻した" << std::endl;
#endif
		Respawn();
		m_Respawned = true;
	}
}

void Player::UpdateFacing(void)
{
	// カメラ更新後の向きに合わせる。復活したフレームの腕は解除される。
	if (m_Respawned) {
		m_Yaw = m_Camera->GetYaw();
		return;
	}
	const bool armActive = m_Arms->IsActive()
		|| ActionIsTrigger(ACTION_ARM_LEFT) || ActionIsTrigger(ACTION_ARM_RIGHT);
	const int target = m_Camera->GetTarget();
	float targetYaw = m_Yaw;
	if (target >= 0) {
		const CAPSULE body = m_Dummies->GetTarget(target).Body;
		const VEC3 center = Vec3Scale(Vec3Add(body.Start, body.End), 0.5f);
		targetYaw = CameraCalcYawTo({ m_Body.GetX(), m_Body.GetY(), m_Body.GetZ() }, center, m_Yaw);
	}
	m_Yaw = PlayerCalcFacing(m_Yaw, m_Body.GetVelocityX(), m_Body.GetVelocityZ(), m_Camera->GetYaw(),
		armActive, TURN_SPEED, GameTimeGetDeltaTime(), target >= 0, targetYaw);
}

float Player::GetYaw(void) const
{
	return m_Yaw;
}

void Player::Draw(void) const
{
	ShapeDrawCapsule({ m_Body.GetX(), m_Body.GetY(), m_Body.GetZ() }, BODY_RADIUS, BODY_HEIGHT, BODY_COLOR);
}

XMFLOAT3 Player::GetPosition(void) const
{
	return { m_Body.GetX(), m_Body.GetY(), m_Body.GetZ() };
}

void Player::SetPosition(const XMFLOAT3& position)
{
	m_Body.SetPosition({ position.x, position.y, position.z });
}

void Player::SetPulled(bool pulled)
{
	m_Pulled = pulled;
	if (pulled) {
		m_Body.SetVelocity({ 0.0f, 0.0f, 0.0f });
		m_Body.SetOnGround(false);
	}
}

float Player::GetHeight(void) const
{
	return BODY_HEIGHT;
}

bool Player::DidRespawn(void) const
{
	return m_Respawned;
}

bool Player::IsOnGround(void) const
{
	return m_Body.GetOnGround();
}

void Player::Initialize(const Stage& stage, const Camera& camera, const DummyManager& dummies, const ArmManager& arms)
{
	m_Stage = &stage;
	m_Camera = &camera;
	m_Dummies = &dummies;
	m_Arms = &arms;
	Reset();
}
