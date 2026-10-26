#include "physics/grapplecalc.h"
#include "physics/armcalc.h"

// 判定の刻み。半径の半分以下、かつ 0.1 以下ずつ進める。
static constexpr float MAX_HIT_STEP = 0.1f;
static constexpr float MIN_LENGTH = 0.00001f;

static VEC3 TargetCenter(const TENTACLE_TARGET& target)
{
	return Vec3Scale(Vec3Add(target.Body.Start, target.Body.End), 0.5f);
}

static void ReturnHook(GRAPPLE& grapple)
{
	grapple.State = GRAPPLE_RETURNING;
	grapple.Target = -1;
	grapple.OnStage = false;
}

void GrappleCalcReset(GRAPPLE& grapple)
{
	grapple = {};
	grapple.State = GRAPPLE_IDLE;
	grapple.Target = -1;
}

VEC3 GrappleCalcAim(const VEC3& origin, const VEC3& target, bool hasTarget, const VEC3& fallback)
{
	const VEC3 delta = Vec3Sub(target, origin);
	const float length = Vec3Length(delta);
	if (hasTarget && length > MIN_LENGTH) return Vec3Scale(delta, 1.0f / length);
	return TentacleCalcAim(origin, target, false, fallback);
}

bool GrappleCalcPull(VEC3& center, const VEC3& anchor, float speed, float distance, float dt)
{
	const VEC3 delta = Vec3Sub(anchor, center);
	const float length = Vec3Length(delta);
	distance = fmaxf(0.0f, distance);
	if (length <= distance || length < MIN_LENGTH) return true;
	const float remaining = length - distance;
	const float move = fminf(remaining, fmaxf(0.0f, speed) * fmaxf(0.0f, dt));
	center = Vec3Add(center, Vec3Scale(delta, move / length));
	return move >= remaining;
}

GRAPPLE_EVENT GrappleCalcStep(GRAPPLE& grapple, const VEC3& origin, const VEC3& aim, VEC3& center,
	bool trigger, bool cancel, float dt, const GRAPPLE_PARAMS& params,
	const TENTACLE_TARGET* targets, int count, const STAGE& stage)
{
	GRAPPLE_EVENT event{ false, false, false, -1, false, {} };
	if (cancel) {
		event.Released = grapple.State != GRAPPLE_IDLE;
		event.Target = grapple.Target;
		event.OnStage = grapple.OnStage;
		GrappleCalcReset(grapple);
		return event;
	}
	if (dt <= 0.0f) return event;
	if (trigger && grapple.State != GRAPPLE_IDLE) {
		event.Released = true;
		event.Target = grapple.Target;
		event.OnStage = grapple.OnStage;
		ReturnHook(grapple);
		return event;
	}
	if (trigger && grapple.State == GRAPPLE_IDLE) {
		GrappleCalcReset(grapple);
		grapple.State = GRAPPLE_FLYING;
		grapple.Hook = origin;
		const float length = Vec3Length(aim);
		grapple.Direction = length > MIN_LENGTH ? Vec3Scale(aim, 1.0f / length) : VEC3{ 0, 0, 1 };
	}
	if (grapple.State == GRAPPLE_FLYING) {
		const VEC3 previous = grapple.Hook;
		const float move = fminf(fmaxf(0.0f, params.MaxDistance - grapple.Distance), params.FlySpeed * dt);
		grapple.Distance += move;
		grapple.Hook = Vec3Add(previous, Vec3Scale(grapple.Direction, move));
		const float step = fmaxf(MIN_LENGTH, fminf(MAX_HIT_STEP, params.Radius * 0.5f));
		const int steps = (int)ceilf(move / step) + 1;
		// 区間全体について敵を優先。同じ区間の敵は手前の刻み、同じ刻みなら番号順。
		for (int sample = 0; sample <= steps; ++sample) {
			const VEC3 point = Vec3Add(previous, Vec3Scale(Vec3Sub(grapple.Hook, previous), (float)sample / steps));
			for (int i = 0; targets != nullptr && i < count; ++i) {
				if (!targets[i].Available || !CollisionSphereCapsule({ point, params.Radius }, targets[i].Body).Hit) continue;
				grapple.Target = i;
				grapple.Anchor = TargetCenter(targets[i]);
				grapple.Hook = grapple.Anchor;
				grapple.State = GRAPPLE_PULLING;
				event.Attached = true;
				event.Target = i;
				return event;
			}
		}
		const BOX box{ { stage.CenterX - stage.HalfWidth, stage.TopY - stage.Thickness, stage.CenterZ - stage.HalfDepth },
			{ stage.CenterX + stage.HalfWidth, stage.TopY, stage.CenterZ + stage.HalfDepth } };
		for (int sample = 0; sample <= steps; ++sample) {
			const VEC3 point = Vec3Add(previous, Vec3Scale(Vec3Sub(grapple.Hook, previous), (float)sample / steps));
			if (!CollisionSphereBox({ point, params.Radius }, box).Hit) continue;
			grapple.Anchor = grapple.Hook = point;
			grapple.OnStage = true;
			grapple.State = GRAPPLE_PULLING;
			event.Attached = event.OnStage = true;
			return event;
		}
		if (grapple.Distance >= params.MaxDistance) ReturnHook(grapple);
	}
	else if (grapple.State == GRAPPLE_PULLING) {
		event.Target = grapple.Target;
		event.OnStage = grapple.OnStage;
		if (grapple.Target >= 0) {
			if (targets == nullptr || grapple.Target >= count) {
				event.Released = true;
				ReturnHook(grapple);
				return event;
			}
			grapple.Anchor = grapple.Hook = TargetCenter(targets[grapple.Target]);
		}
		const VEC3 delta = Vec3Sub(grapple.Anchor, center);
		const float length = Vec3Length(delta);
		event.Direction = length > MIN_LENGTH ? Vec3Scale(delta, 1.0f / length) : grapple.Direction;
		// 残っている時間までだけ進め、長いフレームでも制限時間を越えて引っぱらない。
		const float pullDt = fminf(dt, fmaxf(0.0f, params.MaxPullSeconds - grapple.PullTime));
		grapple.PullTime += pullDt;
		event.Arrived = GrappleCalcPull(center, grapple.Anchor, params.PullSpeed,
			grapple.OnStage ? params.StageDistance : params.EnemyDistance, pullDt);
		if (event.Arrived || grapple.PullTime >= params.MaxPullSeconds) {
			event.Released = true;
			ReturnHook(grapple);
		}
	}
	else if (grapple.State == GRAPPLE_RETURNING) {
		if (GrappleCalcPull(grapple.Hook, origin, params.FlySpeed, 0.0f, dt)) GrappleCalcReset(grapple);
	}
	return event;
}

VEC3 GrappleCalcLanding(const VEC3& anchor, const STAGE& stage, float inset)
{
	const float width = fmaxf(0.0f, stage.HalfWidth - fmaxf(0.0f, inset));
	const float depth = fmaxf(0.0f, stage.HalfDepth - fmaxf(0.0f, inset));
	return { fmaxf(stage.CenterX - width, fminf(stage.CenterX + width, anchor.X)), stage.TopY,
		fmaxf(stage.CenterZ - depth, fminf(stage.CenterZ + depth, anchor.Z)) };
}

VEC3 GrappleCalcModelPoint(const VEC3& point, const VEC3& start, const VEC3& end,
	float scale, bool mirrorX, float roll)
{
	const TENTACLE_PIECE_AXES axes = ArmCalcRollAxes(TentacleCalcPieceAxes(start, end, { 0, -1, 0 }), roll);
	const VEC3 x = Vec3Scale(axes.X, (mirrorX ? -point.X : point.X) * scale);
	const VEC3 y = Vec3Scale(axes.Y, point.Y * scale);
	const VEC3 z = Vec3Scale(axes.Z, point.Z * scale);
	return Vec3Add(start, Vec3Add(x, Vec3Add(y, z)));
}
