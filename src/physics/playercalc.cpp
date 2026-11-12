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
	const PLAYER_PARAMS& params, const STAGE& stage, const VEC3& rushVelocity)
{
	// 1. 突進中は指定された横の速度を使う。通常の移動の向きは長さ 1 まで。
	if (rushVelocity.X != 0.0f || rushVelocity.Z != 0.0f) {
		m_VelocityX = rushVelocity.X;
		m_VelocityZ = rushVelocity.Z;
	}
	else {
		const float length = sqrtf(moveX * moveX + moveZ * moveZ);
		if (length > 1.0f) {
			moveX /= length;
			moveZ /= length;
		}
		m_VelocityX = moveX * params.MoveSpeed;
		m_VelocityZ = moveZ * params.MoveSpeed;
	}

	// 2. 足元の高さに足場があればふつうのジャンプ、空中では残りがあれば空中ジャンプ
	if (m_OnGround && !StageCalcHasSupport(stage, m_X, m_Y, m_Z)) {
		m_OnGround = false;
	}
	if (m_OnGround) m_AirJumpsUsed = 0;
	if (jump) Jump(params.JumpSpeed, params.AirJumps);

	// 3. 重力と移動
	const float previousY = m_Y;
	if (m_OnGround) {
		m_VelocityY = 0.0f;
	} else {
		m_VelocityY = PlayerCalcFall(m_VelocityY, params.Gravity, params.MaxFallSpeed, dt);
	}
	m_X += m_VelocityX * dt;
	m_Y += m_VelocityY * dt;
	m_Z += m_VelocityZ * dt;

	// 4. 着地（上から落ちてきて、上の面を通り過ぎたときだけ）
	float landingY;
	if (m_OnGround) {
		m_OnGround = StageCalcHasSupport(stage, m_X, m_Y, m_Z);
	} else if (m_VelocityY <= 0.0f && StageCalcFindLanding(stage, m_X, m_Z, previousY, m_Y, landingY)) {
		m_Y = landingY;
		m_VelocityY = 0.0f;
		m_OnGround = true;
		m_AirJumpsUsed = 0;
	}
}

float PlayerCalcFall(float velocityY, float gravity, float maxFallSpeed, float dt)
{
	const float next = velocityY - gravity * dt;
	if (maxFallSpeed <= 0.0f) return next;
	// もう上限より速く落ちているなら、そのままにする（重力では加速しない）。
	if (velocityY <= -maxFallSpeed) return velocityY;
	return fmaxf(next, -maxFallSpeed);
}

bool PlayerBodyCalc::Jump(float jumpSpeed, int airJumps)
{
	if (!m_OnGround) {
		if (m_AirJumpsUsed >= airJumps) return false;
		++m_AirJumpsUsed;
	}
	m_VelocityY = jumpSpeed;
	m_OnGround = false;
	return true;
}
