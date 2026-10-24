#include <math.h>
#include "physics/cameracalc.h"


VEC3 CameraCalcOrbitPosition(const VEC3& target, float yaw, float pitch, float distance)
{
	const float dirX = cosf(pitch) * sinf(yaw);
	const float dirY = sinf(pitch);
	const float dirZ = cosf(pitch) * cosf(yaw);
	return { target.X - dirX * distance, target.Y - dirY * distance, target.Z - dirZ * distance };
}

VEC3 CameraCalcClipToStage(const VEC3& target, const VEC3& camera, const STAGE& stage, float margin)
{
	// 浮島の箱（margin だけ大きくする）
	const float boxMin[3] = { stage.CenterX - stage.HalfWidth - margin, stage.TopY - stage.Thickness - margin, stage.CenterZ - stage.HalfDepth - margin };
	const float boxMax[3] = { stage.CenterX + stage.HalfWidth + margin, stage.TopY + margin, stage.CenterZ + stage.HalfDepth + margin };
	const float start[3] = { target.X, target.Y, target.Z };
	const float delta[3] = { camera.X - target.X, camera.Y - target.Y, camera.Z - target.Z };

	// 見ている点が箱の中なら、近づけようがない
	bool inside = true;
	for (int i = 0; i < 3; i++) {
		if (start[i] < boxMin[i] || start[i] > boxMax[i]) {
			inside = false;
		}
	}
	if (inside) {
		return camera;
	}

	// 線（start + delta × t、t は 0〜1）が箱に入る t を、軸ごとの「入る・出る」の範囲を重ねて求める（スラブ法）
	float enter = 0.0f;
	float exit = 1.0f;
	for (int i = 0; i < 3; i++) {
		if (fabsf(delta[i]) < 1.0e-6f) {
			// この軸には動かない：箱の範囲の外なら、ぶつからない
			if (start[i] < boxMin[i] || start[i] > boxMax[i]) {
				return camera;
			}
			continue;
		}
		float t0 = (boxMin[i] - start[i]) / delta[i];
		float t1 = (boxMax[i] - start[i]) / delta[i];
		if (t0 > t1) {
			const float swap = t0;
			t0 = t1;
			t1 = swap;
		}
		if (t0 > enter) enter = t0;
		if (t1 < exit) exit = t1;
		if (enter > exit) {
			return camera;	// どの軸の範囲も重ならない：ぶつからない
		}
	}

	// 箱に入るところ（enter）まで近づける
	return { start[0] + delta[0] * enter, start[1] + delta[1] * enter, start[2] + delta[2] * enter };
}

float CameraCalcSmoothDamp(float current, float target, float& velocity, float smoothTime, float dt)
{
	if (dt <= 0.0f) return current;
	// 臨界減衰バネ。速度を持ち越し、目標が変わっても今の動きから滑らかにつなぐ。
	const float omega = 2.0f / fmaxf(smoothTime, 0.0001f);
	const float x = omega * dt;
	const float decay = 1.0f / (1.0f + x + 0.48f * x * x + 0.235f * x * x * x);
	const float change = current - target;
	const float step = (velocity + omega * change) * dt;
	velocity = (velocity - omega * step) * decay;
	const float result = target + (change + step) * decay;
	// 持ち越した速度で目標を越える場合は、目標で止める。
	if ((target > current && result > target) || (target < current && result < target)) {
		velocity = 0.0f;
		return target;
	}
	return result;
}

static float WrapYaw(float yaw)
{
	return atan2f(sinf(yaw), cosf(yaw));
}

float CameraCalcSmoothDampAngle(float current, float target, float& velocity, float smoothTime, float dt)
{
	if (dt <= 0.0f) return current;
	const float nearest = current + WrapYaw(target - current);
	return WrapYaw(CameraCalcSmoothDamp(current, nearest, velocity, smoothTime, dt));
}

float CameraCalcYawTo(const VEC3& from, const VEC3& to, float fallbackYaw)
{
	const VEC3 delta = Vec3Sub(to, from);
	if (delta.X * delta.X + delta.Z * delta.Z < 1.0e-8f) return fallbackYaw;
	return atan2f(delta.X, delta.Z);
}

float CameraCalcTurnYaw(float current, float target, float speed, float dt)
{
	if (dt <= 0.0f || speed <= 0.0f) return current;
	const float delta = WrapYaw(target - current);
	const float step = speed * dt;
	return WrapYaw(current + fmaxf(-step, fminf(step, delta)));
}

int CameraCalcPickFocus(float yaw, const VEC3& player, const CAMERA_FOCUS_TARGET* targets,
	int count, int current, float maxAngle, float maxDistance, float bonus)
{
	int best = -1;
	float bestScore = maxAngle + 1.0f;
	for (int i = 0; i < count; ++i) {
		const VEC3 delta = Vec3Sub(targets[i].Position, player);
		if (!targets[i].Available || Vec3LengthSq(delta) > maxDistance * maxDistance) continue;
		// 真上・真下の敵には水平の狙いが定まらない。
		if (delta.X * delta.X + delta.Z * delta.Z < 1.0e-8f) continue;
		const float angle = fabsf(WrapYaw(CameraCalcYawTo(player, targets[i].Position, yaw) - yaw));
		if (angle > maxAngle) continue;
		const float score = angle - (i == current ? bonus : 0.0f);
		if (score < bestScore) { bestScore = score; best = i; }
	}
	return best;
}

VEC3 CameraCalcFocusPoint(const VEC3& player, const VEC3& enemy, float yaw, float pitch,
	float distance, float fov, float aspect, float margin, float radius, float rate)
{
	const VEC3 delta = Vec3Sub(enemy, player);
	const VEC3 forward{ cosf(pitch) * sinf(yaw), sinf(pitch), cosf(pitch) * cosf(yaw) };
	const VEC3 right{ cosf(yaw), 0.0f, -sinf(yaw) };
	const VEC3 up{ -sinf(pitch) * sinf(yaw), cosf(pitch), -sinf(pitch) * cosf(yaw) };
	const float tanY = tanf(fov * 0.5f) * (1.0f - margin);
	const float limits[2]{ tanY * aspect, tanY };
	const float sides[2]{ fabsf(Vec3Dot(delta, right)), fabsf(Vec3Dot(delta, up)) };
	float shift = fmaxf(0.0f, fminf(1.0f, rate));
	for (int axis = 0; axis < 2; ++axis) {
		// 球全体が視錐台の内側に入る条件。敵側への奥行きのずれも考慮する。
		const float room = distance * limits[axis] - radius * sqrtf(1.0f + limits[axis] * limits[axis]);
		const float cost = sides[axis] + limits[axis] * Vec3Dot(delta, forward);
		if (room <= 0.0f) return player;
		if (cost > 0.0f) shift = fminf(shift, room / cost);
	}
	return Vec3Add(player, Vec3Scale(delta, shift));
}
