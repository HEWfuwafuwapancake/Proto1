#include "render/motioncalc.h"
#include "core/mathutil.h"
#include <algorithm>
#include <cmath>

static constexpr float MOTION_PI = 3.14159265f;

MOTION_HIT_REACTION MotionCalcHitReaction(bool held, bool stunned, bool launched, float launchElapsed, float flinchSeconds)
{
	if (held) return { true, SKINNED_CLIP::HitHeld, -1.0f };
	if (launchElapsed >= 0.0f && launchElapsed < flinchSeconds) return { true, SKINNED_CLIP::HitFlinch, launchElapsed };
	if (stunned) return { true, SKINNED_CLIP::HitStun, -1.0f };
	if (launched) return { true, SKINNED_CLIP::HitLaunched, -1.0f };
	return {};
}

MOTION_DASH_CLIP MotionCalcDash(float elapsed, bool air, float clipSeconds)
{
	if (elapsed < 0.0f || elapsed >= clipSeconds) return {};
	return { true, air ? SKINNED_CLIP::AirDash : SKINNED_CLIP::Dash, elapsed };
}

void MotionCalc::Reset(float yaw)
{
	m_LegYaw = WrapYaw(yaw);
	m_Twist = 0.0f;
	m_LeanPitch = 0.0f;
	m_LeanRoll = 0.0f;
	m_LandTimer = 0.0f;
	m_PlaySpeed = 1.0f;
	m_ClipTime = -1.0f;
	m_Backward = false;
	m_WasOnGround = true;
	m_Clip = SKINNED_CLIP::Idle;
}

void MotionCalc::Step(const MOTION_INPUT& input, const MOTION_PARAMS& params, float dt)
{
	if (dt <= 0.0f) return;
	const float aim = WrapYaw(input.AimYaw);
	const float speed = sqrtf(input.VelocityX * input.VelocityX + input.VelocityZ * input.VelocityZ);
	const bool moving = input.Locomotion && speed > params.RunThreshold;
	m_ClipTime = -1.0f;

	// 0. ダッシュ中は、脚（モデル全体）をダッシュの向きへすぐそろえ、ダッシュのクリップを見せる。
	//    上半身は狙いの方へ上限までねじる（脚は引っぱらない）。傾きはクリップに任せて戻す。
	const MOTION_DASH_CLIP dash = input.Locomotion ? MotionCalcDash(input.DashElapsed, input.DashAir, params.DashClipSeconds) : MOTION_DASH_CLIP{};
	if (dash.Active) {
		m_LegYaw = WrapYaw(input.DashYaw);
		m_Twist = std::max(-params.MaxTwist, std::min(params.MaxTwist, WrapYaw(aim - m_LegYaw)));
		m_Backward = false;
		m_LandTimer = 0.0f;
		m_WasOnGround = input.OnGround;
		m_Clip = dash.Clip;
		m_ClipTime = dash.ClipTime;
		m_PlaySpeed = 1.0f;
		const float rate = Saturate(params.LeanRate * dt);
		m_LeanPitch -= m_LeanPitch * rate;
		m_LeanRoll -= m_LeanRoll * rate;
		return;
	}

	// 1. 脚の向き：走っているときは進む向き（狙いと反対へ進むときは後ろ走りで狙いの側）、止まっているときは狙いの向き
	float legTarget = aim;
	float turnSpeed = params.IdleTurnSpeed;
	if (moving) {
		const float moveYaw = atan2f(input.VelocityX, input.VelocityZ);
		const float difference = fabsf(WrapYaw(moveYaw - aim));
		m_Backward = m_Backward ? difference > params.BackAngle - params.BackHysteresis : difference > params.BackAngle;
		legTarget = m_Backward ? WrapYaw(moveYaw + MOTION_PI) : moveYaw;
		turnSpeed = params.LegTurnSpeed;
	} else {
		m_Backward = false;
	}
	const float previousLegYaw = m_LegYaw;
	m_LegYaw = input.Locomotion ? TurnYaw(m_LegYaw, legTarget, turnSpeed, dt) : aim;

	// 2. 上半身のねじれは上限まで。超える分は脚を狙いの側へ引っぱる
	float twist = WrapYaw(aim - m_LegYaw);
	if (fabsf(twist) > params.MaxTwist) {
		twist = twist > 0.0f ? params.MaxTwist : -params.MaxTwist;
		m_LegYaw = WrapYaw(aim - twist);
	}
	m_Twist = twist;

	// 3. クリップ：空中は上昇／落下、着地の直後は着地、地面では走り／後ろ走り／待機
	m_LandTimer = std::max(0.0f, m_LandTimer - dt);
	if (input.OnGround && !m_WasOnGround) m_LandTimer = params.LandSeconds;
	m_WasOnGround = input.OnGround;
	if (!input.OnGround) m_Clip = input.Locomotion && input.VelocityY > 0.0f ? SKINNED_CLIP::Jump : SKINNED_CLIP::Fall;
	else if (moving) m_Clip = m_Backward ? SKINNED_CLIP::RunBack : SKINNED_CLIP::Run;
	else if (m_LandTimer > 0.0f) m_Clip = SKINNED_CLIP::Land;
	else m_Clip = SKINNED_CLIP::Idle;

	// 4. 走りの再生速度は横の速さに合わせる（足がすべって見えないように）
	const float speedRatio = params.FullSpeed > 0.0f ? speed / params.FullSpeed : 1.0f;
	m_PlaySpeed = moving ? std::max(params.MinPlaySpeed, speedRatio) : 1.0f;

	// 5. 傾き：走ると前へ（後ろ走りは少し後ろへ）、曲がると内側へ。なめらかに近づける
	float pitchTarget = 0.0f, rollTarget = 0.0f;
	if (moving && input.OnGround) {
		pitchTarget = params.LeanForward * Saturate(speedRatio) * (m_Backward ? -0.5f : 1.0f);
		const float turnRate = WrapYaw(m_LegYaw - previousLegYaw) / dt;
		rollTarget = std::max(-params.LeanSideMax, std::min(params.LeanSideMax, turnRate * params.LeanTurn));
	}
	const float rate = Saturate(params.LeanRate * dt);
	m_LeanPitch += (pitchTarget - m_LeanPitch) * rate;
	m_LeanRoll += (rollTarget - m_LeanRoll) * rate;
}
