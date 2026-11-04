#include "game/arm_freeze.h"
#include "game/fightermanager.h"
#include "game/fighter.h"
#include "core/gametime.h"
#include "physics/areacalc.h"
#include "physics/armcalc.h"
#include "render/shape.h"

// 調整用の値
static constexpr float PHASE_SECONDS[] = { 0.2f, 0.8f }; // 冷気・休み
static constexpr float COLD_RADIUS = 5.0f;
static constexpr float COLD_HALF_ANGLE = XMConvertToRadians(20.0f);
static constexpr float COLD_HALF_HEIGHT = 1.0f;
static constexpr float COLD_DAMAGE = 3.0f;
static constexpr float SLOW_SECONDS = 2.5f;
static constexpr float SLOW_SCALE = 0.4f;
static constexpr float FROZEN_SECONDS = 1.0f; // もう遅い相手を凍らせる（しびれ）秒数。
static constexpr float STRAIGHTEN_SECONDS = 0.05f;
static constexpr float ARM_LENGTH = 0.5f;
static constexpr float NOZZLE_LENGTH = 0.2f;
static constexpr float NOZZLE_RADIUS = 0.07f;
static constexpr int COLD_BALLS = 5;
static constexpr float COLD_BALL_RADIUS = 0.22f;
static constexpr XMFLOAT4 COLD_COLOR{ 0.55f, 0.85f, 1.0f, 1.0f };
static constexpr float HIT_RUMBLE_SMALL = 0.4f;
static constexpr float HIT_RUMBLE_LARGE = 0.2f;
static constexpr float HIT_RUMBLE_SECONDS = 0.12f;

enum FREEZE_PHASE { FREEZE_BLAST, FREEZE_RECOVER, FREEZE_PHASE_COUNT };

FreezeArm::FreezeArm(int hand, const ARM_LINKS& links) : Arm(hand, links)
{
	Reset();
}

void FreezeArm::Reset()
{
	m_Phase.Reset();
	m_Straighten = 0.0f;
}

void FreezeArm::Update(bool trigger)
{
	const float dt = fmaxf(0.0f, GameTimeGetDeltaTime());
	if (GetPlayer().DidRespawn()) Reset();
	if (m_Phase.IsIdle() && trigger) {
		m_Direction = GetAim(false);
		m_Phase.Start(FREEZE_BLAST);
		m_Straighten = 1.0f;
		const VEC3 shoulder = GetShoulder();
		bool hit = false;
		for (int i = 0; i < FIGHTER_COUNT; ++i) {
			const TENTACLE_TARGET target = GetFighters().GetTarget(i, GetSlot());
			if (!target.Available) continue;
			if (!AreaCalcInSector(shoulder, m_Direction, 0.0f, COLD_RADIUS, COLD_HALF_ANGLE, COLD_HALF_HEIGHT, target.Body)) continue;
			// もう遅くなっている相手は凍る（しびれ）。
			const bool frozen = GetFighters().GetFighter(i).IsStatus(STATUS_SLOW);
			GetFighters().AddDamage(i, GetSlot(), COLD_DAMAGE);
			if (frozen) GetFighters().ApplyStatus(i, GetSlot(), STATUS_STUN, FROZEN_SECONDS, 0.0f);
			GetFighters().ApplyStatus(i, GetSlot(), STATUS_SLOW, SLOW_SECONDS, SLOW_SCALE);
			hit = true;
		}
		if (hit) Rumble(HIT_RUMBLE_SMALL, HIT_RUMBLE_LARGE, HIT_RUMBLE_SECONDS);
	}
	m_Phase.Step(dt, PHASE_SECONDS, FREEZE_PHASE_COUNT);
	const float speed = dt / STRAIGHTEN_SECONDS;
	m_Straighten = IsActive() ? fminf(1.0f, m_Straighten + speed) : fmaxf(0.0f, m_Straighten - speed);
}

void FreezeArm::Draw()
{
	const VEC3 direction = ArmCalcBlendDirection(GetRestDirection(), m_Direction, m_Straighten);
	const XMFLOAT4 color = GetColor(IsActive());
	const VEC3 hand = DrawStubArm(direction, ARM_LENGTH, color);
	const VEC3 nozzle = Vec3Add(hand, Vec3Scale(direction, NOZZLE_LENGTH));
	ShapeDrawCylinder({ hand.X, hand.Y, hand.Z }, { nozzle.X, nozzle.Y, nozzle.Z }, NOZZLE_RADIUS, color);
	if (!IsActive()) return;
	const VEC3 shoulder = GetShoulder();
	for (int i = 1; i <= COLD_BALLS; ++i) {
		const VEC3 p = Vec3Add(shoulder, Vec3Scale(m_Direction, COLD_RADIUS * i / COLD_BALLS));
		ShapeDrawSphere({ p.X, p.Y, p.Z }, COLD_BALL_RADIUS, COLD_COLOR);
	}
}

bool FreezeArm::IsActive() const
{
	return m_Phase.GetPhase() == FREEZE_BLAST;
}

bool FreezeArm::IsHolding() const
{
	return false;
}
