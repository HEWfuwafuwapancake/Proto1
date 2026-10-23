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

VEC3 CameraCalcSmooth(const VEC3& current, const VEC3& target, float speed, float dt)
{
	// 1フレームで、残りの距離のうち rate の割合だけ近づく（rate は 0〜1 未満なので行き過ぎない）
	const float rate = 1.0f - expf(-speed * dt);
	return {
		current.X + (target.X - current.X) * rate,
		current.Y + (target.Y - current.Y) * rate,
		current.Z + (target.Z - current.Z) * rate,
	};
}
