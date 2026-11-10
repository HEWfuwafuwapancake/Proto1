#include "game/arm_gunlance.h"
#include "game/fightermanager.h"
#include "game/fighter.h"
#include "game/stage.h"
#include "core/gametime.h"
#include "physics/areacalc.h"
#include "physics/armcalc.h"
#include "render/shape.h"

// 調整用の値
static constexpr GUNLANCE_PARAMS GUN_PARAMS{ 1.0f, 0.5f };
static constexpr PROJECTILE_PARAMS SHOT_PARAMS{ 0.2f, 0.0f, 0.8f, 0.0f, false };
static constexpr float SHOT_SPEED = 30.0f;
static constexpr float MUZZLE_FORWARD = 1.4f; // 肩から弾を出す所までの距離。
static constexpr float LEVEL_DAMAGE = 6.0f; // 1段階あたりのダメージ %。
static constexpr float LEVEL_POWER = 1.0f; // 1段階あたりの吹っ飛ばす強さ。
static constexpr float BLAST_FORWARD = 1.5f; // 肩から爆心までの前への距離。
static constexpr float BLAST_RADIUS = 2.0f;
static constexpr float BLAST_UP = 0.6f;
static constexpr float SHOT_UP = 0.4f;
static constexpr float RUSH_SPEED = 8.0f;
static constexpr float RUSH_SECONDS = 2.0f; // 突進を続けられる秒数。
static constexpr float RUSH_REACH = 1.6f; // 突進の当たりの、肩から前への距離。
static constexpr float RUSH_RADIUS = 0.35f;
static constexpr float RUSH_DAMAGE = 6.0f;
static constexpr float POSE_SECONDS = 0.2f; // 発射のあと構えを保つ秒数。
static constexpr float SHOT_AIM_CLIP_SECONDS = 6.0f / 24.0f; // 射撃のクリップの、構え終わるまで（gunlance_shot）。
static constexpr float SHOT_RECOIL_CLIP_SECONDS = 8.0f / 24.0f; // 射撃のクリップの、反動の長さ。
static constexpr float RUSH_ENTER_CLIP_SECONDS = 5.0f / 24.0f; // 突進のクリップの、構え終わるまで（gunlance_rush）。
static constexpr float RUSH_LOOP_CLIP_SECONDS = 12.0f / 24.0f; // 突進のクリップの、くり返す長さ。
static constexpr float RANGE_SHOW_SECONDS = 0.2f; // 爆発の範囲を見せる秒数。
static constexpr float STRAIGHTEN_SECONDS = 0.1f;
static constexpr float CARRY_PITCH = XMConvertToRadians(15.0f); // 待機のときの槍の上下の向き。
static constexpr float MODEL_SCALE = 0.08f * Arm::MODEL_SIZE;
static constexpr float ARM_LENGTH = 15.189075f; // arm_drill_base の長さ（cm）。
static constexpr float LANCE_LENGTH = 18.060484f; // arm_drill_bit の長さ（cm）。
static constexpr float BARREL_LENGTH = 24.0f; // arm_gunlance_barrel の長さ（cm）。
static constexpr float LANCE_SIZE = 2.6f; // 槍だけを大きく描く倍率（砲身より先へ出す）。
static constexpr float BARREL_SIZE = 1.1f; // 砲身の大きさの倍率。
static constexpr float ARM_ROLL = XMConvertToRadians(180.0f); // 腕のモデルの手のひらを上へ向けるひねり。
static constexpr float BARREL_UP = 0.14f; // 砲身を槍より上へずらす距離。
static constexpr float SHOT_RADIUS = 0.15f;
static constexpr float LEVEL_LAMP_GAP = 0.12f; // ためた段階を見せる球の間隔。
static constexpr float LEVEL_LAMP_RADIUS = 0.05f;
static constexpr XMFLOAT4 LAMP_COLOR{ 1.0f, 0.5f, 0.1f, 1.0f };
static constexpr XMFLOAT4 RANGE_COLOR{ 1.0f, 0.6f, 0.2f, 1.0f };
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

void GunlanceArm::Reset()
{
	m_Calc.Reset();
	m_Shots.Reset();
	m_Rushing = false;
	m_Straighten = 0.0f;
	m_PoseSeconds = 0.0f;
	m_RangeSeconds = 0.0f;
}

void GunlanceArm::Interrupt()
{
	m_Calc.Reset();
	m_Rushing = false;
	m_Straighten = 0.0f;
	m_PoseSeconds = 0.0f;
	StepShots(fmaxf(0.0f, GameTimeGetDeltaTime()));
}

void GunlanceArm::Fire(int level)
{
	const VEC3 shoulder = GetShoulder();
	m_Direction = GetAim(false);
	m_PoseSeconds = POSE_SECONDS;
	m_RangeCenter = Vec3Add(shoulder, Vec3Scale(m_Direction, BLAST_FORWARD));
	m_RangeSeconds = RANGE_SHOW_SECONDS;
	const float damage = level * LEVEL_DAMAGE;
	const float power = level * LEVEL_POWER;
	const int hits = Explode(m_RangeCenter, BLAST_RADIUS, damage, power, BLAST_UP);
	const VEC3 aim = GetAim(true);
	const int index = m_Shots.Fire(Vec3Add(shoulder, Vec3Scale(aim, MUZZLE_FORWARD)), Vec3Scale(aim, SHOT_SPEED), -1, SHOT_PARAMS);
	if (index >= 0) m_ShotLevels[index] = level;
	if (hits > 0) Rumble(HIT_RUMBLE_SMALL, HIT_RUMBLE_LARGE, HIT_RUMBLE_SECONDS);
}

void GunlanceArm::StepRush(const ARM_INPUT& input, float dt)
{
	if (input.HoldStarted && !m_Rushing && m_Calc.GetLevel() == 0) {
		m_Rushing = true;
		m_RushTime = 0.0f;
	}
	if (!m_Rushing) return;
	m_RushTime += dt;
	if (!input.Pressed || m_RushTime >= RUSH_SECONDS) {
		m_Rushing = false;
		return;
	}
	m_Direction = GetAim(false);
	const VEC3 shoulder = GetShoulder();
	const CAPSULE reach{ shoulder, Vec3Add(shoulder, Vec3Scale(m_Direction, RUSH_REACH)), RUSH_RADIUS };
	for (int i = 0; i < FIGHTER_COUNT; ++i) {
		const TENTACLE_TARGET target = GetFighters().GetTarget(i, GetSlot());
		if (!target.Available || !CollisionCapsuleCapsule(reach, target.Body).Hit || !m_Calc.TryHit(i, GUN_PARAMS)) continue;
		GetFighters().AddDamage(i, GetSlot(), RUSH_DAMAGE);
		Rumble(HIT_RUMBLE_SMALL, HIT_RUMBLE_LARGE, HIT_RUMBLE_SECONDS);
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
		const int level = m_ShotLevels[events[i].Index];
		GetFighters().Launch(events[i].Target, GetSlot(), AreaCalcLaunchDirection(events[i].Direction, SHOT_UP), level * LEVEL_DAMAGE, level * LEVEL_POWER);
		Rumble(HIT_RUMBLE_SMALL, HIT_RUMBLE_LARGE, HIT_RUMBLE_SECONDS);
	}
	m_RangeSeconds = fmaxf(0.0f, m_RangeSeconds - dt);
}

void GunlanceArm::Update(const ARM_INPUT& input)
{
	const float dt = fmaxf(0.0f, GameTimeGetDeltaTime());
	if (GetPlayer().DidRespawn()) Reset();
	const int fired = m_Calc.Step(input.Tap && !m_Rushing, dt, GUN_PARAMS);
	if (fired > 0) Fire(fired);
	if (m_Calc.GetLevel() > 0 && !m_Rushing) m_Direction = GetAim(false);
	StepRush(input, dt);
	StepShots(dt);
	m_PoseSeconds = fmaxf(0.0f, m_PoseSeconds - dt);
	const float speed = dt / STRAIGHTEN_SECONDS;
	m_Straighten = IsActive() ? fminf(1.0f, m_Straighten + speed) : fmaxf(0.0f, m_Straighten - speed);
}

void GunlanceArm::Draw()
{
	VEC3 armDirection = ArmCalcBlendDirection(GetRestDirection(), m_Direction, m_Straighten);
	VEC3 lanceDirection = ArmCalcBlendDirection(ArmCalcPitchDirection(GetForward(), CARRY_PITCH), m_Direction, m_Straighten);
	BlendClipPose(armDirection, lanceDirection);
	const XMFLOAT4 color = GetColor(IsActive());
	const float scale = MODEL_SCALE * GetDrawScale();
	const VEC3 hand = DrawAlong(m_ArmModel, GetDrawShoulder(), armDirection, ARM_LENGTH, scale, color, ARM_ROLL);
	DrawAlong(m_LanceModel, hand, lanceDirection, LANCE_LENGTH, scale * LANCE_SIZE, color);
	const VEC3 barrel = Vec3Add(hand, { 0.0f, BARREL_UP, 0.0f });
	const VEC3 muzzle = DrawAlong(m_BarrelModel, barrel, lanceDirection, BARREL_LENGTH, scale * BARREL_SIZE, color);
	// ためた段階の数だけ、砲身の先の上に球を並べる。
	for (int i = 0; i < m_Calc.GetLevel(); ++i) {
		ShapeDrawSphere({ muzzle.X, muzzle.Y + LEVEL_LAMP_GAP * (i + 1), muzzle.Z }, LEVEL_LAMP_RADIUS, LAMP_COLOR);
	}
	for (int i = 0; i < PROJECTILE_MAX; ++i) {
		const PROJECTILE& shot = m_Shots.Get(i);
		if (shot.Active) ShapeDrawSphere({ shot.Position.X, shot.Position.Y, shot.Position.Z }, SHOT_RADIUS, LAMP_COLOR);
	}
	if (m_RangeSeconds > 0.0f) DrawRangeRing(m_RangeCenter, BLAST_RADIUS, RANGE_COLOR);
}

bool GunlanceArm::IsActive() const
{
	return m_Rushing || m_Calc.GetLevel() > 0 || m_PoseSeconds > 0.0f;
}

ARM_BODY_CLIP GunlanceArm::GetBodyClip() const
{
	ARM_BODY_CLIP clip;
	if (!IsActive()) return clip;
	clip.Active = true;
	if (m_Rushing) {
		clip.Kind = ATTACK_CLIP::GunlanceRush;
		clip.Time = m_RushTime < RUSH_ENTER_CLIP_SECONDS ? m_RushTime
			: RUSH_ENTER_CLIP_SECONDS + fmodf(m_RushTime - RUSH_ENTER_CLIP_SECONDS, RUSH_LOOP_CLIP_SECONDS);
		return clip;
	}
	clip.Kind = ATTACK_CLIP::GunlanceShot;
	clip.Time = m_PoseSeconds > 0.0f ? SHOT_AIM_CLIP_SECONDS + SHOT_RECOIL_CLIP_SECONDS * (1.0f - m_PoseSeconds / POSE_SECONDS)
		: SHOT_AIM_CLIP_SECONDS * m_Straighten; // ためている間は構え終わりで止める。
	return clip;
}

bool GunlanceArm::CanTakeTap() const
{
	return !m_Rushing;
}

bool GunlanceArm::CanTakeHold() const
{
	return !m_Rushing && m_Calc.GetLevel() == 0;
}

bool GunlanceArm::IsHolding() const
{
	return false;
}

VEC3 GunlanceArm::GetRushVelocity() const
{
	return m_Rushing ? Vec3Scale(m_Direction, RUSH_SPEED) : VEC3{};
}
