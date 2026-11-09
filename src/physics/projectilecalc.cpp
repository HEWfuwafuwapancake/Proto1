#include "physics/projectilecalc.h"
#include <math.h>

// 調整用の値
static constexpr float MIN_LENGTH = 0.0001f; // これより短い向きは「向きなし」とみなす。
static constexpr float MIN_FLIGHT_SECONDS = 0.2f; // 山なりの最短の飛ぶ時間（真下に近いときに速さが大きくなりすぎない）。

void ProjectileCalc::Reset()
{
	*this = {};
}

int ProjectileCalc::Fire(const VEC3& origin, const VEC3& velocity, int target, const PROJECTILE_PARAMS& params)
{
	for (int i = 0; i < PROJECTILE_MAX; ++i) {
		if (m_Shots[i].Active) continue;
		m_Shots[i] = {};
		m_Shots[i].Position = origin;
		m_Shots[i].Velocity = velocity;
		m_Shots[i].Life = params.LifeSeconds;
		m_Shots[i].Target = target;
		m_Shots[i].Active = true;
		return i;
	}
	return -1;
}

int ProjectileCalc::GetCount() const
{
	int count = 0;
	for (int i = 0; i < PROJECTILE_MAX; ++i) {
		if (m_Shots[i].Active) ++count;
	}
	return count;
}

// 速度の向きを target の方へ maxAngle まで回す（速さは変えない）。
static VEC3 TurnToward(const VEC3& velocity, const VEC3& toTarget, float maxAngle)
{
	const float speed = Vec3Length(velocity);
	const float distance = Vec3Length(toTarget);
	if (speed < MIN_LENGTH || distance < MIN_LENGTH) return velocity;
	const VEC3 current = Vec3Scale(velocity, 1.0f / speed);
	const VEC3 desired = Vec3Scale(toTarget, 1.0f / distance);
	const float cosine = Vec3Dot(current, desired);
	const float angle = acosf(cosine < -1.0f ? -1.0f : (cosine > 1.0f ? 1.0f : cosine));
	if (angle <= maxAngle) return Vec3Scale(desired, speed);
	const float t = maxAngle / angle;
	VEC3 mixed = Vec3Add(Vec3Scale(current, 1.0f - t), Vec3Scale(desired, t));
	const float length = Vec3Length(mixed);
	if (length < MIN_LENGTH) return velocity;
	return Vec3Scale(mixed, speed / length);
}

int ProjectileCalc::Step(float dt, const TENTACLE_TARGET* targets, int count, const STAGE& stage,
	const PROJECTILE_PARAMS& params, PROJECTILE_EVENT* events, int maxEvents)
{
	int written = 0;
	auto add = [&](PROJECTILE_EVENT_TYPE type, int index, int target, const VEC3& position) {
		if (written >= maxEvents) return;
		const VEC3 v = m_Shots[index].Velocity;
		const float speed = Vec3Length(v);
		events[written++] = { type, index, target, position, speed < MIN_LENGTH ? VEC3{ 0.0f, 0.0f, 1.0f } : Vec3Scale(v, 1.0f / speed) };
	};
	if (dt <= 0.0f) return 0;
	for (int i = 0; i < PROJECTILE_MAX; ++i) {
		PROJECTILE& shot = m_Shots[i];
		if (!shot.Active) continue;
		if (params.TurnRate > 0.0f && shot.Target >= 0 && shot.Target < count && targets[shot.Target].Available) {
			const CAPSULE& body = targets[shot.Target].Body;
			const VEC3 center = Vec3Scale(Vec3Add(body.Start, body.End), 0.5f);
			shot.Velocity = TurnToward(shot.Velocity, Vec3Sub(center, shot.Position), params.TurnRate * dt);
		}
		const VEC3 previous = shot.Position;
		shot.Velocity.Y -= params.Gravity * dt;
		shot.Position = Vec3Add(shot.Position, Vec3Scale(shot.Velocity, dt));
		shot.Life -= dt;
		const CAPSULE path{ previous, shot.Position, params.Radius };
		// 相手：貫通しない弾は前の位置にいちばん近い相手だけ。
		int nearest = -1;
		float nearestDistance = 0.0f;
		for (int t = 0; t < count; ++t) {
			if (!targets[t].Available || shot.Hits.Contains(t) || !CollisionCapsuleCapsule(path, targets[t].Body).Hit) continue;
			if (params.Pierce) {
				shot.Hits.TryAdd(t);
				add(PROJECTILE_HIT_FIGHTER, i, t, shot.Position);
				continue;
			}
			VEC3 onPath, onBody;
			CollisionClosestPointsOfSegments(previous, shot.Position, targets[t].Body.Start, targets[t].Body.End, onPath, onBody);
			const float d = Vec3Length(Vec3Sub(onPath, previous));
			if (nearest < 0 || d < nearestDistance) {
				nearest = t;
				nearestDistance = d;
			}
		}
		if (nearest >= 0) {
			add(PROJECTILE_HIT_FIGHTER, i, nearest, shot.Position);
			shot.Active = false;
			continue;
		}
		// 高台は上から当たる。浮島は今までどおり厚さの中に入っても当たる。
		float landingY = stage.TopY;
		if (StageCalcFindLanding(stage, shot.Position.X, shot.Position.Z, previous.Y, shot.Position.Y, landingY)
			|| (shot.Position.Y <= stage.TopY && shot.Position.Y >= stage.TopY - stage.Thickness - params.Radius
				&& StageCalcIsOnPlatform(stage, shot.Position.X, shot.Position.Z))) {
			add(PROJECTILE_HIT_STAGE, i, -1, { shot.Position.X, landingY, shot.Position.Z });
			shot.Active = false;
			continue;
		}
		if (shot.Life <= 0.0f || StageCalcIsOut(stage, shot.Position.Y)) {
			add(PROJECTILE_EXPIRED, i, -1, shot.Position);
			shot.Active = false;
		}
	}
	return written;
}

VEC3 ProjectileCalcLobVelocity(const VEC3& from, const VEC3& to, float horizontalSpeed, float gravity, float maxRange)
{
	VEC3 flat{ to.X - from.X, 0.0f, to.Z - from.Z };
	float distance = Vec3Length(flat);
	if (distance > maxRange) {
		flat = Vec3Scale(flat, maxRange / distance);
		distance = maxRange;
	}
	const float time = fmaxf(MIN_FLIGHT_SECONDS, distance / horizontalSpeed);
	const VEC3 horizontal = Vec3Scale(flat, 1.0f / time);
	const float vy = (to.Y - from.Y + 0.5f * gravity * time * time) / time;
	return { horizontal.X, vy, horizontal.Z };
}

VEC3 ProjectileCalcMoveToward(const VEC3& from, const VEC3& to, float speed, float dt)
{
	const VEC3 offset = Vec3Sub(to, from);
	const float distance = Vec3Length(offset);
	const float step = speed * dt;
	if (distance <= step || distance < MIN_LENGTH) return to;
	return Vec3Add(from, Vec3Scale(offset, step / distance));
}
