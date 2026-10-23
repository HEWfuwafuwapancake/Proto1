#include <math.h>
#include "physics/playercalc.h"


void PlayerCalcStep(PLAYER_BODY& body, float moveX, float moveZ, bool jump, float dt,
	const PLAYER_PARAMS& params, const STAGE& stage)
{
	// 1. 横の速度（斜めでも速くならないように、向きの長さは 1 まで）
	const float length = sqrtf(moveX * moveX + moveZ * moveZ);
	if (length > 1.0f) {
		moveX /= length;
		moveZ /= length;
	}
	body.VelocityX = moveX * params.MoveSpeed;
	body.VelocityZ = moveZ * params.MoveSpeed;

	// 2. ジャンプ・浮島の端から歩き出たとき
	if (body.OnGround && jump) {
		body.VelocityY = params.JumpSpeed;
		body.OnGround = false;
	}
	if (body.OnGround && !StageCalcIsOnPlatform(stage, body.X, body.Z)) {
		body.OnGround = false;
	}

	// 3. 重力と移動
	const float previousY = body.Y;
	if (!body.OnGround) {
		body.VelocityY -= params.Gravity * dt;
	}
	body.X += body.VelocityX * dt;
	body.Y += body.VelocityY * dt;
	body.Z += body.VelocityZ * dt;

	// 4. 着地（上から落ちてきて、上の面を通り過ぎたときだけ）
	if (!body.OnGround && body.VelocityY <= 0.0f
		&& previousY >= stage.TopY && body.Y <= stage.TopY
		&& StageCalcIsOnPlatform(stage, body.X, body.Z)) {
		body.Y = stage.TopY;
		body.VelocityY = 0.0f;
		body.OnGround = true;
	}

	// 立っている間は上の面の高さに止めておく
	if (body.OnGround) {
		body.Y = stage.TopY;
		body.VelocityY = 0.0f;
	}
}
