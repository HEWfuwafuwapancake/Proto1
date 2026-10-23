#include <math.h>
#include "physics/knockbackcalc.h"


static constexpr float MIN_HORIZONTAL = 1.0e-4f;	// 攻撃の向きの横の長さがこれより短ければ「横に向いていない」とみなす


float KnockbackCalcAddDamage(float percent, float damage)
{
	return percent + (damage > 0.0f ? damage : 0.0f);
}

float KnockbackCalcSpeed(float percent, float power, const KNOCKBACK_PARAMS& params)
{
	return (params.BaseSpeed + percent * params.Growth) * power;
}

VEC3 KnockbackCalcVelocity(const VEC3& direction, float speed, const KNOCKBACK_PARAMS& params)
{
	const float horizontal = sqrtf(direction.X * direction.X + direction.Z * direction.Z);
	if (horizontal < MIN_HORIZONTAL) {
		return { 0.0f, speed, 0.0f };	// 横に向いていない：真上
	}
	const float side = cosf(params.LaunchAngle) * speed;	// 横の速さ
	const float up = sinf(params.LaunchAngle) * speed;		// 上の速さ
	return { direction.X / horizontal * side, up, direction.Z / horizontal * side };
}

bool KnockbackCalcStep(PLAYER_BODY& body, float dt, const KNOCKBACK_PARAMS& params, const STAGE& stage)
{
	// すべっている途中で浮島の端から出たら、落ち始める
	if (body.OnGround && !StageCalcIsOnPlatform(stage, body.X, body.Z)) {
		body.OnGround = false;
	}

	if (body.OnGround) {
		// 浮島の上をすべる：横の速さを摩擦の分だけ減らす
		const float speed = sqrtf(body.VelocityX * body.VelocityX + body.VelocityZ * body.VelocityZ);
		const float slowed = speed - params.GroundFriction * dt;
		if (slowed <= params.StopSpeed) {
			body.VelocityX = 0.0f;
			body.VelocityZ = 0.0f;
			body.VelocityY = 0.0f;
			return false;	// 止まった
		}
		body.VelocityX *= slowed / speed;
		body.VelocityZ *= slowed / speed;
		body.VelocityY = 0.0f;
	}
	else {
		// 空中：重力と空気の抵抗
		const float drag = expf(-params.AirDrag * dt);
		body.VelocityX *= drag;
		body.VelocityZ *= drag;
		body.VelocityY -= params.Gravity * dt;
	}

	const float previousY = body.Y;
	body.X += body.VelocityX * dt;
	body.Y += body.VelocityY * dt;
	body.Z += body.VelocityZ * dt;

	// 着地（上から落ちてきて、上の面を通り過ぎたときだけ。横の速さは残してすべらせる）
	if (!body.OnGround && body.VelocityY <= 0.0f
		&& previousY >= stage.TopY && body.Y <= stage.TopY
		&& StageCalcIsOnPlatform(stage, body.X, body.Z)) {
		body.Y = stage.TopY;
		body.VelocityY = 0.0f;
		body.OnGround = true;
	}
	if (body.OnGround) {
		body.Y = stage.TopY;
	}
	return true;
}

VEC3 KnockbackCalcDirectedVelocity(const VEC3& direction, float speed)
{
	const float length = Vec3Length(direction);
	return length < MIN_HORIZONTAL ? VEC3{ 0.0f, speed, 0.0f } : Vec3Scale(direction, speed / length);
}
