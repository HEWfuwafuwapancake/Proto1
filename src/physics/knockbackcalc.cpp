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

bool KnockbackCalcStep(PlayerBodyCalc& body, float dt, const KNOCKBACK_PARAMS& params, const STAGE& stage)
{
	float X = body.GetX();
	float Y = body.GetY();
	float Z = body.GetZ();
	float VelocityX = body.GetVelocityX();
	float VelocityY = body.GetVelocityY();
	float VelocityZ = body.GetVelocityZ();
	bool OnGround = body.GetOnGround();
	// すべっている途中で浮島の端から出たら、落ち始める
	if (OnGround && !StageCalcIsOnPlatform(stage, X, Z)) {
		OnGround = false;
	}

	if (OnGround) {
		// 浮島の上をすべる：横の速さを摩擦の分だけ減らす
		const float speed = sqrtf(VelocityX * VelocityX + VelocityZ * VelocityZ);
		const float slowed = speed - params.GroundFriction * dt;
		if (slowed <= params.StopSpeed) {
			VelocityX = 0.0f;
			VelocityZ = 0.0f;
			VelocityY = 0.0f;
			body.SetPosition({ X, Y, Z });
			body.SetVelocity({ VelocityX, VelocityY, VelocityZ });
			body.SetOnGround(OnGround);
			return false;	// 止まった
		}
		VelocityX *= slowed / speed;
		VelocityZ *= slowed / speed;
		VelocityY = 0.0f;
	}
	else {
		// 空中：重力と空気の抵抗
		const float drag = expf(-params.AirDrag * dt);
		VelocityX *= drag;
		VelocityZ *= drag;
		VelocityY -= params.Gravity * dt;
	}

	const float previousY = Y;
	X += VelocityX * dt;
	Y += VelocityY * dt;
	Z += VelocityZ * dt;

	// 着地（上から落ちてきて、上の面を通り過ぎたときだけ。横の速さは残してすべらせる）
	if (!OnGround && VelocityY <= 0.0f
		&& previousY >= stage.TopY && Y <= stage.TopY
		&& StageCalcIsOnPlatform(stage, X, Z)) {
		Y = stage.TopY;
		VelocityY = 0.0f;
		OnGround = true;
	}
	if (OnGround) {
		Y = stage.TopY;
	}
	body.SetPosition({ X, Y, Z });
	body.SetVelocity({ VelocityX, VelocityY, VelocityZ });
	body.SetOnGround(OnGround);
	return true;
}

VEC3 KnockbackCalcDirectedVelocity(const VEC3& direction, float speed)
{
	const float length = Vec3Length(direction);
	return length < MIN_HORIZONTAL ? VEC3{ 0.0f, speed, 0.0f } : Vec3Scale(direction, speed / length);
}
