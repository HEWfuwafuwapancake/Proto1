#include <math.h>
#include "physics/playercalc.h"
#include "core/mathutil.h"


float PlayerCalcFacing(float current, float moveX, float moveZ, float cameraYaw,
	bool armActive, float speed, float dt, bool hasTarget, float targetYaw)
{
	if (armActive) return WrapYaw(cameraYaw);
	if (hasTarget) return TurnYaw(current, targetYaw, speed, dt);
	if (moveX == 0.0f && moveZ == 0.0f) return current;
	return TurnYaw(current, atan2f(moveX, moveZ), speed, dt);
}


void PlayerBodyCalc::Step(float moveX, float moveZ, bool jump, float dt,
	const PLAYER_PARAMS& params, const STAGE& stage)
{
	// 1. 横の速度（斜めでも速くならないように、向きの長さは 1 まで）
	const float length = sqrtf(moveX * moveX + moveZ * moveZ);
	if (length > 1.0f) {
		moveX /= length;
		moveZ /= length;
	}
	m_VelocityX = moveX * params.MoveSpeed;
	m_VelocityZ = moveZ * params.MoveSpeed;

	// 2. ジャンプ・浮島の端から歩き出たとき
	if (m_OnGround && jump) {
		m_VelocityY = params.JumpSpeed;
		m_OnGround = false;
	}
	if (m_OnGround && !StageCalcIsOnPlatform(stage, m_X, m_Z)) {
		m_OnGround = false;
	}

	// 3. 重力と移動
	const float previousY = m_Y;
	if (!m_OnGround) {
		m_VelocityY -= params.Gravity * dt;
	}
	m_X += m_VelocityX * dt;
	m_Y += m_VelocityY * dt;
	m_Z += m_VelocityZ * dt;

	// 4. 着地（上から落ちてきて、上の面を通り過ぎたときだけ）
	if (!m_OnGround && m_VelocityY <= 0.0f
		&& previousY >= stage.TopY && m_Y <= stage.TopY
		&& StageCalcIsOnPlatform(stage, m_X, m_Z)) {
		m_Y = stage.TopY;
		m_VelocityY = 0.0f;
		m_OnGround = true;
	}

	// 立っている間は上の面の高さに止めておく
	if (m_OnGround) {
		m_Y = stage.TopY;
		m_VelocityY = 0.0f;
	}
}
