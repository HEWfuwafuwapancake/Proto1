#include "game/arm_gunlance.h"
#include "game/fightermanager.h"
#include "game/fighter.h"
#include "game/stage.h"
#include "core/gametime.h"
#include "physics/areacalc.h"
#include "physics/armcalc.h"
#include "render/shape.h"

// 調整用の値
static constexpr int SHOT_COST = 5;
static constexpr int IMPALE_COST = 50;
static constexpr int RUSH_MISS_COST = 25;
static constexpr int IMPALE_SHOTS = 3;
static constexpr PROJECTILE_PARAMS SHOT_PARAMS{ 0.2f, 0.0f, 0.8f, 0.0f, false };
static constexpr float SHOT_SPEED = 30.0f;
static constexpr float MUZZLE_FORWARD = 1.4f; // 肩から弾を出す所までの距離。
static constexpr float SHOT_DAMAGE = 3.0f;
static constexpr float RUSH_SPEED = 8.0f;
static constexpr float RUSH_REACH = 1.6f; // 突進の当たりの、肩から前への距離。
static constexpr float RUSH_RADIUS = 0.35f;
static constexpr float IMPALE_DAMAGE = 3.0f;
static constexpr float BURST_DAMAGE = 4.0f;
static constexpr float FINISH_DAMAGE = 6.0f;
static constexpr float FINISH_POWER = 1.5f;
static constexpr float FINISH_UP = 1.0f; // 最後の1発で飛ばす向きの、前 1 に足す上の量（1.0 で約45°。大きいほど真上に近い）。
static constexpr float LIFT_FORWARD = 1.2f; // 持ち上げ終わった相手の肩から前への距離。
static constexpr float LIFT_HEIGHT = 1.6f;
static constexpr float POSE_SECONDS = 0.2f; // 発射のあと構えを保つ秒数。
static constexpr float RECOIL_SECONDS = 0.15f; // 撃ち込みの反動を描く秒数。
static constexpr float SHOT_AIM_CLIP_SECONDS = 6.0f / 24.0f; // 射撃のクリップの、構え終わるまで（gunlance_shot）。
static constexpr float SHOT_RECOIL_CLIP_SECONDS = 8.0f / 24.0f; // 射撃のクリップの、反動の長さ。
static constexpr float RUSH_ENTER_CLIP_SECONDS = 5.0f / 24.0f; // 突進のクリップの、構え終わるまで（gunlance_rush）。
static constexpr float RUSH_LOOP_CLIP_SECONDS = 12.0f / 24.0f; // 突進のクリップの、くり返す長さ。
static constexpr float LIFT_CLIP_SECONDS = 8.0f / 24.0f; // 突き上げのクリップの、持ち上げ終わるまで（gunlance_impale）。
static constexpr float RECOIL_CLIP_SECONDS = 5.0f / 24.0f; // 突き上げのクリップの、1発分の反動の長さ。
static constexpr float STRAIGHTEN_SECONDS = 0.1f;
static constexpr float CARRY_PITCH = XMConvertToRadians(15.0f); // 待機のときの槍の上下の向き。
static constexpr float LIFT_ARM_PITCH = XMConvertToRadians(40.0f);
static constexpr float LIFT_LANCE_PITCH = XMConvertToRadians(60.0f);
static constexpr float RECOIL_PITCH = XMConvertToRadians(10.0f);
static constexpr float MODEL_SCALE = 0.08f * Arm::MODEL_SIZE;
static constexpr float ARM_LENGTH = 15.189075f; // arm_drill_base の長さ（cm）。
static constexpr float LANCE_LENGTH = 18.060484f; // arm_drill_bit の長さ（cm）。
static constexpr float BARREL_LENGTH = 24.0f; // arm_gunlance_barrel の長さ（cm）。
static constexpr float LANCE_SIZE = 2.6f; // 槍だけを大きく描く倍率（砲身より先へ出す）。
static constexpr float BARREL_SIZE = 1.1f; // 砲身の大きさの倍率。
static constexpr float ARM_ROLL = XMConvertToRadians(180.0f); // 腕のモデルの手のひらを上へ向けるひねり。
static constexpr float BARREL_UP = 0.14f; // 砲身を槍より上へずらす距離。
static constexpr float SHOT_RADIUS = 0.15f;
static constexpr XMFLOAT4 SHOT_COLOR{ 1.0f, 0.5f, 0.1f, 1.0f };
static constexpr float HIT_RUMBLE_SMALL = 0.6f;
static constexpr float HIT_RUMBLE_LARGE = 0.8f;
static constexpr float HIT_RUMBLE_SECONDS = 0.15f;

GunlanceArm::GunlanceArm(int hand, const ARM_LINKS& links) : Arm(hand, links)
{
	m_ArmModel = LoadModel("assets/models/arm_drill_base.obj");
	m_LanceModel = LoadModel("assets/models/arm_drill_bit.obj");
	m_BarrelModel = LoadModel("assets/models/arm_gunlance_barrel.obj");
	Reset();
}

void GunlanceArm::FinishRush()
{
	if (m_RushPending) ConsumeDurability(RUSH_MISS_COST);
	m_RushPending = false;
}

void GunlanceArm::Reset()
{
	FinishRush();
	ReleaseHeld();
	m_Calc.Reset();
	m_Shots.Reset();
	m_Straighten = 0.0f;
	m_PoseSeconds = 0.0f;
	m_RecoilSeconds = 0.0f;
}

void GunlanceArm::Interrupt()
{
	FinishRush();
	ReleaseHeld();
	m_Calc.StopAction();
	m_Straighten = 0.0f;
	m_PoseSeconds = 0.0f;
	m_RecoilSeconds = 0.0f;
	const float dt = fmaxf(0.0f, GameTimeGetDeltaTime());
	m_Calc.Step(dt, false);
	StepShots(dt);
}

void GunlanceArm::ReleaseHeld()
{
	if (m_HeldTarget < 0) return;
	GetFighters().Release(m_HeldTarget, GetSlot(), false, {}, 0.0f, 0.0f);
	m_HeldTarget = -1;
}

void GunlanceArm::Fire()
{
	if (!CanUse() || !m_Calc.CanFire()) return;
	const VEC3 shoulder = GetShoulder();
	const VEC3 aim = GetAim(true);
	const int index = m_Shots.Fire(Vec3Add(shoulder, Vec3Scale(aim, MUZZLE_FORWARD)), Vec3Scale(aim, SHOT_SPEED), -1, SHOT_PARAMS);
	if (index < 0) return;
	m_Direction = aim;
	m_PoseSeconds = POSE_SECONDS;
	m_RecoilSeconds = 0.0f;
	m_Calc.RecordShot();
	ConsumeDurability(SHOT_COST);
}

void GunlanceArm::StepRush()
{
	m_Direction = GetAim(false);
	const VEC3 shoulder = GetShoulder();
	const CAPSULE reach{ shoulder, Vec3Add(shoulder, Vec3Scale(m_Direction, RUSH_REACH)), RUSH_RADIUS };
	FighterManager& fighters = GetFighters();
	for (int i = 0; i < FIGHTER_COUNT; ++i) {
		const TENTACLE_TARGET target = fighters.GetTarget(i, GetSlot());
		if (!target.Available || !CollisionCapsuleCapsule(reach, target.Body).Hit) continue;
		if (!fighters.Grab(i, GetSlot())) {
			m_Calc.StopAction();
			return;
		}
		m_HeldTarget = i;
		fighters.AddDamage(i, GetSlot(), IMPALE_DAMAGE);
		ConsumeDurability(IMPALE_COST);
		m_RushPending = false;
		m_Calc.StartImpale(IMPALE_SHOTS);
		Rumble(HIT_RUMBLE_SMALL, HIT_RUMBLE_LARGE, HIT_RUMBLE_SECONDS);
		return;
	}
}

void GunlanceArm::StepImpale(const GUNLANCE_STEP& step)
{
	const float lift = m_Calc.GetLiftProgress();
	const float forward = RUSH_REACH + (LIFT_FORWARD - RUSH_REACH) * lift;
	VEC3 center = Vec3Add(GetShoulder(), Vec3Scale(m_Direction, forward));
	center.Y += LIFT_HEIGHT * lift;
	FighterManager& fighters = GetFighters();
	fighters.MoveHeld(m_HeldTarget, GetSlot(), center);

	for (int i = 0; i < step.Shots; ++i) {
		m_RecoilSeconds = RECOIL_SECONDS;
		Rumble(HIT_RUMBLE_SMALL, HIT_RUMBLE_LARGE, HIT_RUMBLE_SECONDS);
		if (step.Finished && i + 1 == step.Shots) {
			fighters.Release(m_HeldTarget, GetSlot(), true, AreaCalcLaunchDirection(m_Direction, FINISH_UP), FINISH_DAMAGE, FINISH_POWER);
			m_HeldTarget = -1;
		}
		else fighters.AddDamage(m_HeldTarget, GetSlot(), BURST_DAMAGE);
	}
}

void GunlanceArm::StepShots(float dt)
{
	TENTACLE_TARGET targets[FIGHTER_COUNT];
	for (int i = 0; i < FIGHTER_COUNT; ++i) targets[i] = GetFighters().GetTarget(i, GetSlot());
	PROJECTILE_EVENT events[PROJECTILE_MAX];
	const int written = m_Shots.Step(dt, targets, FIGHTER_COUNT, GetStage().GetStage(), SHOT_PARAMS, events, PROJECTILE_MAX);
	for (int i = 0; i < written; ++i) {
		if (events[i].Type != PROJECTILE_HIT_FIGHTER) continue;
		if (GetFighters().AddDamage(events[i].Target, GetSlot(), SHOT_DAMAGE)) {
			Rumble(HIT_RUMBLE_SMALL, HIT_RUMBLE_LARGE, HIT_RUMBLE_SECONDS);
		}
	}
}

void GunlanceArm::Update(const ARM_INPUT& input)
{
	const float dt = fmaxf(0.0f, GameTimeGetDeltaTime());
	if (GetPlayer().DidRespawn()) Reset();
	m_PoseSeconds = fmaxf(0.0f, m_PoseSeconds - dt);
	m_RecoilSeconds = fmaxf(0.0f, m_RecoilSeconds - dt);
	if (m_HeldTarget >= 0 && !GetFighters().IsHeldBy(m_HeldTarget, GetSlot())) {
		m_HeldTarget = -1;
		m_Calc.StopAction();
		m_RecoilSeconds = 0.0f;
	}
	if (input.HoldStarted && CanTakeHold()) {
		m_Calc.StartRush();
		m_RushPending = true;
		m_PoseSeconds = 0.0f;
		m_RecoilSeconds = 0.0f;
	}
	const GUNLANCE_STEP step = m_Calc.Step(dt, input.Pressed);
	if (input.Tap) Fire();
	if (m_Calc.IsRushing()) StepRush();
	if (!m_Calc.IsRushing()) FinishRush();
	if (m_HeldTarget >= 0) StepImpale(step);
	StepShots(dt);
	const float speed = dt / STRAIGHTEN_SECONDS;
	m_Straighten = IsActive() ? fminf(1.0f, m_Straighten + speed) : fmaxf(0.0f, m_Straighten - speed);
}

void GunlanceArm::Draw()
{
	VEC3 aimArm = m_Direction;
	VEC3 aimLance = m_Direction;
	if (IsHolding() || m_RecoilSeconds > 0.0f) {
		const float lift = m_Calc.GetLiftProgress();
		const float recoil = RECOIL_PITCH * sinf(XM_PI * (1.0f - m_RecoilSeconds / RECOIL_SECONDS));
		aimArm = ArmCalcPitchDirection(m_Direction, LIFT_ARM_PITCH * lift + recoil);
		aimLance = ArmCalcPitchDirection(m_Direction, LIFT_LANCE_PITCH * lift + recoil);
	}
	VEC3 armDirection = ArmCalcBlendDirection(GetRestDirection(), aimArm, m_Straighten);
	VEC3 lanceDirection = ArmCalcBlendDirection(ArmCalcPitchDirection(GetForward(), CARRY_PITCH), aimLance, m_Straighten);
	BlendClipPose(armDirection, lanceDirection);
	const XMFLOAT4 color = GetColor(IsActive());
	const float scale = MODEL_SCALE * GetDrawScale();
	const VEC3 hand = DrawAlong(m_ArmModel, GetDrawShoulder(), armDirection, ARM_LENGTH, scale, color, ARM_ROLL);
	DrawAlong(m_LanceModel, hand, lanceDirection, LANCE_LENGTH, scale * LANCE_SIZE, color);
	const VEC3 barrel = Vec3Add(hand, { 0.0f, BARREL_UP, 0.0f });
	DrawAlong(m_BarrelModel, barrel, lanceDirection, BARREL_LENGTH, scale * BARREL_SIZE, color);
	for (int i = 0; i < PROJECTILE_MAX; ++i) {
		const PROJECTILE& shot = m_Shots.Get(i);
		if (shot.Active) ShapeDrawSphere({ shot.Position.X, shot.Position.Y, shot.Position.Z }, SHOT_RADIUS, SHOT_COLOR);
	}
}

bool GunlanceArm::IsActive() const
{
	return !m_Calc.IsIdle() || m_PoseSeconds > 0.0f || m_RecoilSeconds > 0.0f;
}

bool GunlanceArm::IsBusy() const
{
	return m_Shots.GetCount() > 0;
}

ARM_BODY_CLIP GunlanceArm::GetBodyClip() const
{
	ARM_BODY_CLIP clip;
	if (!IsActive()) return clip;
	clip.Active = true;
	if (m_Calc.IsRushing()) {
		const float rushTime = m_Calc.GetRushTime();
		clip.Kind = ATTACK_CLIP::GunlanceRush;
		clip.Time = rushTime < RUSH_ENTER_CLIP_SECONDS ? rushTime
			: RUSH_ENTER_CLIP_SECONDS + fmodf(rushTime - RUSH_ENTER_CLIP_SECONDS, RUSH_LOOP_CLIP_SECONDS);
		return clip;
	}
	if (IsHolding() || m_RecoilSeconds > 0.0f) {
		clip.Kind = ATTACK_CLIP::GunlanceImpale;
		clip.Time = m_RecoilSeconds > 0.0f ? LIFT_CLIP_SECONDS + RECOIL_CLIP_SECONDS * (1.0f - m_RecoilSeconds / RECOIL_SECONDS)
			: LIFT_CLIP_SECONDS * m_Calc.GetLiftProgress();
		return clip;
	}
	clip.Kind = ATTACK_CLIP::GunlanceShot;
	clip.Time = SHOT_AIM_CLIP_SECONDS + SHOT_RECOIL_CLIP_SECONDS * (1.0f - m_PoseSeconds / POSE_SECONDS);
	return clip;
}

bool GunlanceArm::CanTakeTap() const
{
	return CanUse() && m_Calc.CanFire(GameTimeGetDeltaTime());
}

bool GunlanceArm::CanTakeHold() const
{
	return CanUse() && m_Calc.IsIdle();
}

bool GunlanceArm::IsHolding() const
{
	return m_Calc.IsHolding();
}

bool GunlanceArm::LocksOtherHand() const
{
	return m_Calc.IsRushing() || m_Calc.IsHolding();
}

float GunlanceArm::GetMoveScale() const
{
	return IsHolding() ? 0.0f : 1.0f;
}

VEC3 GunlanceArm::GetRushVelocity() const
{
	return m_Calc.IsRushing() ? Vec3Scale(m_Direction, RUSH_SPEED * TuningCalcValue(GetTuning(), TUNING_RUSH_DISTANCE)) : VEC3{};
}
