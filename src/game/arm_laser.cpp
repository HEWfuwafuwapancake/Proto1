#include "game/arm_laser.h"
#include "game/fightermanager.h"
#include "game/fighter.h"
#include "game/camera.h"
#include "core/gametime.h"
#include "core/mathutil.h"
#include "physics/areacalc.h"
#include "physics/armcalc.h"
#include "render/shape.h"

// 調整用の値
// 耐久の減る量は physics/armcostcalc の表のレーザーの行（弱：単発、強：溜めきった砲）。
static constexpr ARM_COST_ROW COST_ROW = ARM_COST_ROW_LASER;
// 溜め：押し始めから 3 秒で砲を撃てる（長押しの判定 0.25 秒は armpresscalc.cpp の HOLD_SECONDS と同じにする）。
static constexpr LASER_PARAMS PARAMS{ 3.0f, 0.25f, 0.4f, 0.6f };
static constexpr float AIM_RANGE = 60.0f; // フリーカメラの視線で相手を探す距離。
static constexpr float AIM_RADIUS = 0.3f; // フリーカメラの視線の太さ（レティクルに少しかかれば狙える）。
static constexpr float SHOT_RANGE = 60.0f; // 単発の届く距離。
static constexpr float SHOT_RADIUS = 0.3f; // フリーカメラの単発の太さ（ロックオン中は必ず当たる）。
static constexpr float SHOT_DAMAGE = 3.0f; // 単発は % だけ増やし、吹っ飛ばさない（連射されても歩ける）。
static constexpr float CANNON_RANGE = 60.0f; // 砲の届く距離。
static constexpr float CANNON_RADIUS = 0.6f; // 砲の太さ（当たり）。
static constexpr float CANNON_DAMAGE = 25.0f; // 3 秒溜める分、ハンマー（18%）より強い。
static constexpr float CANNON_POWER = 3.2f;
static constexpr float CANNON_UP = 0.3f; // 光線の向きに足す上向きの量。
static constexpr float CHARGE_MOVE_SCALE = 0.5f; // 溜めている間の歩く速さの倍率。
static constexpr float STRAIGHTEN_SECONDS = 0.1f;
static constexpr float CARRY_PITCH = XMConvertToRadians(15.0f); // 待機のときの砲身の上下の向き。
static constexpr float SHOT_BEAM_SECONDS = 0.1f; // 単発の光線を描く秒数。
static constexpr float CANNON_BEAM_SECONDS = 0.45f; // 砲の光線を描く秒数（細くなって消える）。
static constexpr float SHOT_BEAM_RADIUS = 0.035f;
static constexpr float CANNON_BEAM_RADIUS = 0.5f;
static constexpr XMFLOAT4 SHOT_BEAM_COLOR{ 1.0f, 0.25f, 0.35f, 1.0f };
static constexpr XMFLOAT4 CANNON_BEAM_COLOR{ 0.55f, 0.95f, 1.0f, 0.8f };
static constexpr XMFLOAT4 CANNON_CORE_COLOR{ 1.0f, 1.0f, 1.0f, 1.0f };
static constexpr float GLOW_MIN_RADIUS = 0.05f; // 溜め始めの砲口の光の半径。
static constexpr float GLOW_MAX_RADIUS = 0.22f; // 溜めきったときの砲口の光の半径。
static constexpr float GLOW_BLINK_SPEED = 18.0f; // 溜めきったあとの光の点滅の速さ。
static constexpr XMFLOAT4 GLOW_COLOR{ 0.4f, 0.9f, 1.0f, 0.9f };
static constexpr XMFLOAT4 GLOW_FULL_COLOR{ 1.0f, 1.0f, 1.0f, 1.0f };
static constexpr float SHOT_AIM_CLIP_SECONDS = 6.0f / 24.0f; // 射撃のクリップの、構え終わるまで（gunlance_shot を使い回す）。
static constexpr float SHOT_RECOIL_CLIP_SECONDS = 8.0f / 24.0f; // 射撃のクリップの、反動の長さ。
static constexpr float MODEL_SCALE = 0.08f * Arm::MODEL_SIZE;
static constexpr float ARM_LENGTH = 15.189075f; // arm_drill_base の長さ（cm）。
static constexpr float BARREL_LENGTH = 24.0f; // arm_gunlance_barrel の長さ（cm）。
static constexpr float BARREL_SIZE = 1.4f; // 砲身の大きさの倍率。
static constexpr float ARM_ROLL = XMConvertToRadians(180.0f); // 腕のモデルの手のひらを上へ向けるひねり。
static constexpr float SHOT_RUMBLE = 0.3f;
static constexpr float SHOT_RUMBLE_SECONDS = 0.08f;
static constexpr float CANNON_RUMBLE = 1.0f;
static constexpr float CANNON_RUMBLE_SECONDS = 0.4f;

LaserArm::LaserArm(int hand, const ARM_LINKS& links) : Arm(hand, links)
{
	m_ArmModel = LoadModel("assets/models/arm_drill_base.obj");
	m_BarrelModel = LoadModel("assets/models/arm_gunlance_barrel.obj");
	Reset();
}

void LaserArm::Reset()
{
	m_Calc.Reset();
	m_ChargeSe.Stop();
	m_ChargeSePlaying = false;
	m_BeamSeconds = 0.0f;
	m_Straighten = 0.0f;
	m_GlowTime = 0.0f;
}

// ロックオン（ハード・ソフト）中は狙っている相手の体の中心。フリーカメラ（と狙う相手がいないとき）は、
// 画面の真ん中のレティクル（カメラの視線）の先で最初に当たる相手、いなければ届く距離の点。
VEC3 LaserArm::GetAimPoint(int* target) const
{
	*target = GetAimTarget();
	if (*target >= 0) {
		const CAPSULE body = GetFighters().GetTarget(*target, GetSlot()).Body;
		return Vec3Scale(Vec3Add(body.Start, body.End), 0.5f);
	}
	TENTACLE_TARGET targets[FIGHTER_COUNT];
	for (int i = 0; i < FIGHTER_COUNT; ++i) targets[i] = GetFighters().GetTarget(i, GetSlot());
	const XMFLOAT3 eye = GetCamera().GetPosition();
	const XMFLOAT3 look = GetCamera().GetDirection();
	return LaserCalcAimPoint({ eye.x, eye.y, eye.z }, { look.x, look.y, look.z }, AIM_RANGE, AIM_RADIUS, targets, FIGHTER_COUNT);
}

void LaserArm::Fire(LASER_FIRE fire)
{
	int target = -1;
	const VEC3 shoulder = GetShoulder();
	const VEC3 aim = GetAimPoint(&target);
	const VEC3 toAim = Vec3Sub(aim, shoulder);
	if (Vec3Length(toAim) > 0.001f) m_Direction = Vec3Scale(toAim, 1.0f / Vec3Length(toAim));
	TENTACLE_TARGET targets[FIGHTER_COUNT];
	for (int i = 0; i < FIGHTER_COUNT; ++i) targets[i] = GetFighters().GetTarget(i, GetSlot());
	bool hit = false;
	if (fire == LASER_FIRE_SHOT) {
		// ロックオン中は必ず当たる。フリーカメラは光線の上で最初の相手に当たる。
		float distance = SHOT_RANGE;
		const int victim = target >= 0 ? target
			: AreaCalcFirstOnRay(shoulder, m_Direction, SHOT_RANGE, SHOT_RADIUS, targets, FIGHTER_COUNT, &distance);
		if (target >= 0) distance = Vec3Length(toAim);
		if (victim >= 0) hit = GetFighters().AddDamage(victim, GetSlot(), SHOT_DAMAGE);
		m_BeamEnd = Vec3Add(shoulder, Vec3Scale(m_Direction, distance));
		StrikePillarsOnLine({ shoulder, m_BeamEnd, SHOT_RADIUS }, SHOT_DAMAGE, m_Direction);
		m_BeamSeconds = SHOT_BEAM_SECONDS;
		Rumble(SHOT_RUMBLE, 0.0f, SHOT_RUMBLE_SECONDS);
	}
	else {
		// 光線の上の相手を全部貫通する（ロックオン中は相手の方へまっすぐ撃つので、狙った相手には必ず当たる）。
		bool hits[FIGHTER_COUNT];
		LaserCalcPierce(shoulder, m_Direction, CANNON_RANGE, CANNON_RADIUS, targets, FIGHTER_COUNT, hits);
		if (target >= 0) hits[target] = true;
		for (int i = 0; i < FIGHTER_COUNT; ++i) {
			if (hits[i] && GetFighters().Launch(i, GetSlot(), AreaCalcLaunchDirection(m_Direction, CANNON_UP), CANNON_DAMAGE, CANNON_POWER)) hit = true;
		}
		m_BeamEnd = Vec3Add(shoulder, Vec3Scale(m_Direction, CANNON_RANGE));
		StrikePillarsOnLine({ shoulder, m_BeamEnd, CANNON_RADIUS }, CANNON_DAMAGE, m_Direction);
		m_BeamSeconds = CANNON_BEAM_SECONDS;
		Rumble(CANNON_RUMBLE, CANNON_RUMBLE, CANNON_RUMBLE_SECONDS);
	}
	m_BeamCannon = fire == LASER_FIRE_CANNON;
	const bool cannon = fire == LASER_FIRE_CANNON;
	PlaySe(cannon ? SE_LASER_CANNON : SE_LASER_SHOT); // 発射音は撃った腕で鳴らす（相手の側へは移さない）
	ConsumeDurability(GetCost(COST_ROW, cannon ? (hit ? ARM_COST_HOLD_HIT : ARM_COST_HOLD_MISS) : (hit ? ARM_COST_TAP_HIT : ARM_COST_TAP_MISS)));
}

void LaserArm::Update(const ARM_INPUT& input)
{
	const float dt = fmaxf(0.0f, GameTimeGetDeltaTime());
	if (GetPlayer().DidRespawn()) Reset();
	const LASER_FIRE fire = m_Calc.Step(input, dt, PARAMS);
	if (fire != LASER_FIRE_NONE) Fire(fire);
	else if (m_Calc.IsCharging()) {
		// 溜めている間も、腕と砲身を狙う点へ向ける（描画だけ）。
		int target = -1;
		const VEC3 toAim = Vec3Sub(GetAimPoint(&target), GetShoulder());
		if (Vec3Length(toAim) > 0.001f) m_Direction = Vec3Scale(toAim, 1.0f / Vec3Length(toAim));
	}
	m_BeamSeconds = fmaxf(0.0f, m_BeamSeconds - dt);
	m_GlowTime = m_Calc.IsCharging() ? m_GlowTime + dt : 0.0f;
	// 溜めの音は溜め始めに1回鳴らし、撃つ・やめたら止める。
	if (m_Calc.IsCharging() && !m_ChargeSePlaying) m_ChargeSe.PlayAt(SE_LASER_CHARGE, GetShoulder(), GetSlot());
	else if (m_Calc.IsCharging()) m_ChargeSe.MoveTo(GetShoulder()); // 動いても、溜めの音は腕についてくる
	else if (m_ChargeSePlaying) m_ChargeSe.Stop();
	m_ChargeSePlaying = m_Calc.IsCharging();
	const float speed = dt / STRAIGHTEN_SECONDS;
	m_Straighten = IsActive() ? fminf(1.0f, m_Straighten + speed) : fmaxf(0.0f, m_Straighten - speed);
}

void LaserArm::Draw()
{
	VEC3 armDirection = ArmCalcBlendDirection(GetRestDirection(), m_Direction, m_Straighten);
	VEC3 barrelDirection = ArmCalcBlendDirection(ArmCalcPitchDirection(GetForward(), CARRY_PITCH), m_Direction, m_Straighten);
	BlendClipPose(armDirection, barrelDirection);
	const XMFLOAT4 color = GetColor(IsActive());
	const float scale = MODEL_SCALE * GetDrawScale();
	const VEC3 hand = DrawAlong(m_ArmModel, GetDrawShoulder(), armDirection, ARM_LENGTH, scale, color, ARM_ROLL);
	const VEC3 muzzle = DrawAlong(m_BarrelModel, hand, barrelDirection, BARREL_LENGTH, scale * BARREL_SIZE, color);
	if (m_Calc.IsCharging()) {
		// 溜めるほど砲口の光が大きくなり、溜めきったら白く点滅する。
		const float charge = m_Calc.GetCharge(PARAMS);
		const bool full = charge >= 1.0f;
		const float blink = full ? 0.85f + 0.15f * sinf(m_GlowTime * GLOW_BLINK_SPEED) : 1.0f;
		ShapeDrawSphere({ muzzle.X, muzzle.Y, muzzle.Z }, Lerp(GLOW_MIN_RADIUS, GLOW_MAX_RADIUS, charge) * blink, full ? GLOW_FULL_COLOR : GLOW_COLOR);
	}
	if (m_BeamSeconds > 0.0f) {
		const XMFLOAT3 start{ muzzle.X, muzzle.Y, muzzle.Z };
		const XMFLOAT3 end{ m_BeamEnd.X, m_BeamEnd.Y, m_BeamEnd.Z };
		if (m_BeamCannon) {
			const float rate = m_BeamSeconds / CANNON_BEAM_SECONDS;
			ShapeDrawCylinder(start, end, CANNON_BEAM_RADIUS * rate, CANNON_BEAM_COLOR);
			ShapeDrawCylinder(start, end, CANNON_BEAM_RADIUS * 0.4f * rate, CANNON_CORE_COLOR);
		}
		else ShapeDrawCylinder(start, end, SHOT_BEAM_RADIUS, SHOT_BEAM_COLOR);
	}
}

bool LaserArm::IsActive() const
{
	return m_Calc.IsCharging() || m_Calc.IsRecovering();
}

bool LaserArm::IsHolding() const
{
	return false;
}

bool LaserArm::CanTakeTap() const
{
	return !IsActive();
}

bool LaserArm::CanTakeHold() const
{
	return !IsActive();
}

float LaserArm::GetMoveScale() const
{
	return m_Calc.IsCharging() ? CHARGE_MOVE_SCALE : 1.0f;
}

ARM_BODY_CLIP LaserArm::GetBodyClip() const
{
	ARM_BODY_CLIP clip;
	if (!IsActive()) return clip;
	clip.Active = true;
	clip.Kind = ATTACK_CLIP::GunlanceShot;
	if (m_Calc.IsCharging()) {
		clip.Time = SHOT_AIM_CLIP_SECONDS;
		return clip;
	}
	const float recover = m_Calc.GetLastFire() == LASER_FIRE_CANNON ? PARAMS.CannonRecoverSeconds : PARAMS.ShotRecoverSeconds;
	clip.Time = SHOT_AIM_CLIP_SECONDS + SHOT_RECOIL_CLIP_SECONDS * (1.0f - m_Calc.GetRecover() / recover);
	return clip;
}

bool LaserArm::IsAiming() const
{
	return IsActive();
}

float LaserArm::GetAimCharge() const
{
	return m_Calc.IsCharging() ? m_Calc.GetCharge(PARAMS) : -1.0f;
}
