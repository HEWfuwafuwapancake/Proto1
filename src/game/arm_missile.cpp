#include "game/arm_missile.h"
#include "game/fightermanager.h"
#include "game/fighter.h"
#include "game/stage.h"
#include "core/gametime.h"
#include "physics/areacalc.h"
#include "physics/armcalc.h"
#include "render/shape.h"

// 調整用の値
static constexpr float PHASE_SECONDS[] = { 0.25f, 2.0f }; // 撃つ・休み
static constexpr float SHOT_SECONDS = 0.1f; // 2発目・3発目までの間
static constexpr int MISSILE_COUNT = 3;
static constexpr PROJECTILE_PARAMS MISSILE{ 0.25f, 0.0f, 3.0f, 3.5f, false }; // 半径・重力なし・寿命・1秒に曲がる角度・貫通しない
static constexpr float MISSILE_SPEED = 11.0f;
static constexpr float SPREAD[MISSILE_COUNT] = { XMConvertToRadians(-30.0f), 0.0f, XMConvertToRadians(30.0f) };
static constexpr float LOCK_UP = 2.0f; // 相手がいるときの上向き（前 1 に対して）。斜め上へ出て追いかける。
static constexpr float FREE_UP = 0.3f; // 相手がいないときの上向き。
static constexpr float BLAST_RADIUS = 1.0f;
static constexpr float BLAST_DAMAGE = 5.0f;
static constexpr float BLAST_POWER = 1.2f;
static constexpr float BLAST_UP = 0.3f;
static constexpr float BLAST_SHOW_SECONDS = 0.15f;
static constexpr float STRAIGHTEN_SECONDS = 0.1f;
static constexpr float ARM_LENGTH = 0.45f;
static constexpr float TUBE_LENGTH = 0.3f;
static constexpr float TUBE_RADIUS = 0.1f;
static constexpr float RAISE_PITCH = XMConvertToRadians(45.0f);
static constexpr float FIRE_RUMBLE_SMALL = 0.3f;
static constexpr float FIRE_RUMBLE_LARGE = 0.2f;
static constexpr float FIRE_RUMBLE_SECONDS = 0.08f;
static constexpr float BLAST_RUMBLE_SMALL = 0.5f;
static constexpr float BLAST_RUMBLE_LARGE = 0.4f;
static constexpr float BLAST_RUMBLE_SECONDS = 0.12f;

enum MISSILE_PHASE { MISSILE_FIRE, MISSILE_RECOVER, MISSILE_PHASE_COUNT };

MissileArm::MissileArm(int hand, const ARM_LINKS& links) : Arm(hand, links)
{
	Reset();
}

void MissileArm::Reset()
{
	m_Phase.Reset();
	m_Missiles.Reset();
	m_Fired = 0;
	m_BlastTime = 0.0f;
	m_Straighten = 0.0f;
}

void MissileArm::FireOne()
{
	if (m_Fired >= MISSILE_COUNT) return;
	const VEC3 side = AreaCalcRotateYaw(GetForward(), SPREAD[m_Fired]);
	const VEC3 direction = AreaCalcLaunchDirection(side, m_Target >= 0 ? LOCK_UP : FREE_UP);
	m_Missiles.Fire(GetShoulder(), Vec3Scale(direction, MISSILE_SPEED), m_Target, MISSILE);
	++m_Fired;
	Rumble(FIRE_RUMBLE_SMALL, FIRE_RUMBLE_LARGE, FIRE_RUMBLE_SECONDS);
}

void MissileArm::Update(bool trigger)
{
	const float dt = fmaxf(0.0f, GameTimeGetDeltaTime());
	if (GetPlayer().DidRespawn()) Reset();
	if (m_Phase.IsIdle() && trigger) {
		m_Target = GetAimTarget();
		m_Fired = 0;
		m_Phase.Start(MISSILE_FIRE);
		FireOne();
	}
	const ARM_PHASE_STEP step = m_Phase.Step(dt, PHASE_SECONDS, MISSILE_PHASE_COUNT, SHOT_SECONDS);
	const int ticks = m_Phase.GetPhase() == MISSILE_FIRE || step.Entered == MISSILE_RECOVER ? step.Ticks : 0;
	for (int t = 0; t < ticks; ++t) FireOne();
	TENTACLE_TARGET targets[FIGHTER_COUNT];
	for (int i = 0; i < FIGHTER_COUNT; ++i) targets[i] = GetFighters().GetTarget(i, GetSlot());
	PROJECTILE_EVENT events[PROJECTILE_MAX];
	const int count = m_Missiles.Step(dt, targets, FIGHTER_COUNT, GetStage().GetStage(), MISSILE, events, PROJECTILE_MAX);
	m_BlastTime = fmaxf(0.0f, m_BlastTime - dt);
	for (int e = 0; e < count; ++e) {
		if (events[e].Type == PROJECTILE_EXPIRED) continue;
		if (Explode(events[e].Position, BLAST_RADIUS, BLAST_DAMAGE, BLAST_POWER, BLAST_UP) > 0) {
			Rumble(BLAST_RUMBLE_SMALL, BLAST_RUMBLE_LARGE, BLAST_RUMBLE_SECONDS);
		}
		m_Blast = events[e].Position;
		m_BlastTime = BLAST_SHOW_SECONDS;
	}
	const float speed = dt / STRAIGHTEN_SECONDS;
	m_Straighten = IsActive() ? fminf(1.0f, m_Straighten + speed) : fmaxf(0.0f, m_Straighten - speed);
}

void MissileArm::Draw()
{
	const VEC3 raised = ArmCalcPitchDirection(GetForward(), RAISE_PITCH);
	const VEC3 direction = ArmCalcBlendDirection(GetRestDirection(), raised, m_Straighten);
	const XMFLOAT4 color = GetColor(IsActive());
	const VEC3 hand = DrawStubArm(direction, ARM_LENGTH, color);
	const VEC3 tube = Vec3Add(hand, Vec3Scale(direction, TUBE_LENGTH));
	ShapeDrawCylinder({ hand.X, hand.Y, hand.Z }, { tube.X, tube.Y, tube.Z }, TUBE_RADIUS, color);
	for (int i = 0; i < PROJECTILE_MAX; ++i) {
		const PROJECTILE& missile = m_Missiles.Get(i);
		if (missile.Active) ShapeDrawSphere({ missile.Position.X, missile.Position.Y, missile.Position.Z }, MISSILE.Radius, color);
	}
	if (m_BlastTime > 0.0f) ShapeDrawSphere({ m_Blast.X, m_Blast.Y, m_Blast.Z }, BLAST_RADIUS, GetColor(true));
}

bool MissileArm::IsActive() const
{
	return m_Phase.GetPhase() == MISSILE_FIRE;
}

bool MissileArm::IsHolding() const
{
	return false;
}
