#include "game/arm_shotgun.h"
#include "game/fightermanager.h"
#include "game/fighter.h"
#include "core/gametime.h"
#include "physics/areacalc.h"
#include "physics/armcalc.h"
#include "render/shape.h"

// 調整用の値
static constexpr float PHASE_SECONDS[] = { 0.08f, 0.62f }; // 弾の線を見せる・休み（合わせて 0.7 秒）
static constexpr int PELLETS = 7;
static constexpr float SPREAD_ANGLE = XMConvertToRadians(20.0f); // 端の弾の左右の開き。
static constexpr float PELLET_RANGE = 7.5f;
static constexpr float PELLET_RADIUS = 0.18f;
static constexpr float PELLET_DAMAGE = 2.5f; // 1本あたり
static constexpr float BASE_POWER = 0.5f;
static constexpr float PELLET_POWER = 0.25f; // 1本あたり足す強さ
static constexpr float LAUNCH_UP = 0.2f;
static constexpr float STRAIGHTEN_SECONDS = 0.05f;
static constexpr float ARM_LENGTH = 0.55f;
static constexpr float BARREL_LENGTH = 0.25f;
static constexpr float BARREL_RADIUS = 0.07f;
static constexpr float TRACE_RADIUS = 0.02f;
static constexpr float FIRE_RUMBLE_SMALL = 0.7f;
static constexpr float FIRE_RUMBLE_LARGE = 0.5f;
static constexpr float FIRE_RUMBLE_SECONDS = 0.12f;

enum SHOTGUN_PHASE { SHOTGUN_FLASH, SHOTGUN_RECOVER, SHOTGUN_PHASE_COUNT };

ShotgunArm::ShotgunArm(int hand, const ARM_LINKS& links) : Arm(hand, links)
{
	Reset();
}

void ShotgunArm::Reset()
{
	m_Phase.Reset();
	m_Straighten = 0.0f;
}

void ShotgunArm::Update(bool trigger)
{
	const float dt = fmaxf(0.0f, GameTimeGetDeltaTime());
	if (GetPlayer().DidRespawn()) Reset();
	if (m_Phase.IsIdle() && trigger) {
		m_Direction = GetAim(false);
		m_Phase.Start(SHOTGUN_FLASH);
		m_Straighten = 1.0f; // 撃った瞬間に構える
		const VEC3 shoulder = GetShoulder();
		for (int i = 0; i < FIGHTER_COUNT; ++i) {
			const TENTACLE_TARGET target = GetFighters().GetTarget(i, GetSlot());
			if (!target.Available) continue;
			const int hits = AreaCalcCountRays(shoulder, m_Direction, PELLET_RANGE, PELLET_RADIUS, PELLETS, SPREAD_ANGLE, target.Body);
			if (hits <= 0) continue;
			GetFighters().Launch(i, GetSlot(), AreaCalcLaunchDirection(m_Direction, LAUNCH_UP),
				PELLET_DAMAGE * hits, BASE_POWER + PELLET_POWER * hits);
		}
		Rumble(FIRE_RUMBLE_SMALL, FIRE_RUMBLE_LARGE, FIRE_RUMBLE_SECONDS);
	}
	m_Phase.Step(dt, PHASE_SECONDS, SHOTGUN_PHASE_COUNT);
	const float speed = dt / STRAIGHTEN_SECONDS;
	m_Straighten = m_Phase.IsIdle() ? fmaxf(0.0f, m_Straighten - speed) : fminf(1.0f, m_Straighten + speed);
}

void ShotgunArm::Draw()
{
	const VEC3 direction = ArmCalcBlendDirection(GetRestDirection(), m_Direction, m_Straighten);
	const XMFLOAT4 color = GetColor(!m_Phase.IsIdle());
	const VEC3 hand = DrawStubArm(direction, ARM_LENGTH, color);
	const VEC3 muzzle = Vec3Add(hand, Vec3Scale(direction, BARREL_LENGTH));
	ShapeDrawCylinder({ hand.X, hand.Y, hand.Z }, { muzzle.X, muzzle.Y, muzzle.Z }, BARREL_RADIUS, color);
	if (m_Phase.GetPhase() != SHOTGUN_FLASH) return;
	// 当たりと同じ扇の線を、撃った直後だけ見せる。
	const VEC3 shoulder = GetShoulder();
	for (int i = 0; i < PELLETS; ++i) {
		const float t = static_cast<float>(i) / (PELLETS - 1) * 2.0f - 1.0f;
		const VEC3 end = Vec3Add(shoulder, Vec3Scale(AreaCalcRotateYaw(m_Direction, SPREAD_ANGLE * t), PELLET_RANGE));
		ShapeDrawCylinder({ muzzle.X, muzzle.Y, muzzle.Z }, { end.X, end.Y, end.Z }, TRACE_RADIUS, color);
	}
}

bool ShotgunArm::IsActive() const
{
	return !m_Phase.IsIdle();
}

bool ShotgunArm::IsHolding() const
{
	return false;
}
