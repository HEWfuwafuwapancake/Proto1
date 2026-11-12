#include <math.h>
#include "physics/knockbackcalc.h"


static constexpr float MIN_HORIZONTAL = 1.0e-4f;	// 攻撃の向きの横の長さがこれより短ければ「横に向いていない」とみなす


float KnockbackCalcAddDamage(float percent, float damage)
{
	return percent + (damage > 0.0f ? damage : 0.0f);
}

float KnockbackCalcHeal(float percent, float amount)
{
	const float healed = percent - (amount > 0.0f ? amount : 0.0f);
	return healed > 0.0f ? healed : 0.0f;
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

bool KnockbackCalcStep(PlayerBodyCalc& body, float dt, const KNOCKBACK_PARAMS& params, const STAGE& stage,
	float moveX, float moveZ)
{
	float X = body.GetX();
	float Y = body.GetY();
	float Z = body.GetZ();
	float VelocityX = body.GetVelocityX();
	float VelocityY = body.GetVelocityY();
	float VelocityZ = body.GetVelocityZ();
	bool OnGround = body.GetOnGround();
	// 足元の高さに足場がなければ落ち始める
	if (OnGround && !StageCalcHasSupport(stage, X, Y, Z)) {
		OnGround = false;
	}

	if (OnGround) {
		// 足場の上をすべる：横の速さを摩擦の分だけ減らす
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
		VelocityY = PlayerCalcFall(VelocityY, params.Gravity, params.MaxFallSpeed, dt);
		// 移動の入力：入力の向きの速さが上限に届くまで、少しずつ足す（吹っ飛びの速度は消さない）
		const float input = sqrtf(moveX * moveX + moveZ * moveZ);
		if (input > MIN_HORIZONTAL && params.AirControl > 0.0f) {
			const float scale = input > 1.0f ? 1.0f : input;
			const float directionX = moveX / input;
			const float directionZ = moveZ / input;
			const float along = VelocityX * directionX + VelocityZ * directionZ;
			const float room = params.ControlSpeed * scale - along;
			if (room > 0.0f) {
				const float gain = params.AirControl * scale * dt;
				const float add = gain < room ? gain : room;
				VelocityX += directionX * add;
				VelocityZ += directionZ * add;
			}
		}
	}

	const float previousY = Y;
	X += VelocityX * dt;
	Y += VelocityY * dt;
	Z += VelocityZ * dt;

	// 着地（上から落ちてきて、上の面を通り過ぎたときだけ。横の速さは残してすべらせる）
	float landingY;
	if (OnGround) {
		OnGround = StageCalcHasSupport(stage, X, Y, Z);
	} else if (VelocityY <= 0.0f && StageCalcFindLanding(stage, X, Z, previousY, Y, landingY)) {
		Y = landingY;
		VelocityY = 0.0f;
		OnGround = true;
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
