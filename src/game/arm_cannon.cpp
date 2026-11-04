#include "game/arm_cannon.h"
#include "game/fightermanager.h"
#include "game/fighter.h"
#include "game/stage.h"
#include "core/gametime.h"
#include "physics/armcalc.h"
#include "render/shape.h"

// 調整用の値
static constexpr float PHASE_SECONDS[] = { 0.3f, 1.0f }; // 構え・休み
static constexpr float MOVE_SCALE = 0.5f;
static constexpr PROJECTILE_PARAMS SHELL{ 0.25f, 12.0f, 5.0f, 0.0f, false }; // 半径・重力・寿命・追いかけない・貫通しない
static constexpr float SHELL_SPEED = 14.0f; // 水平の速さ
static constexpr float NO_TARGET_DISTANCE = 10.0f; // 相手がいないときに落とす前の距離。
static constexpr float MAX_RANGE = 16.0f;
static constexpr float BLAST_RADIUS = 2.2f;
static constexpr float BLAST_DAMAGE = 11.0f;
static constexpr float BLAST_POWER = 2.2f;
static constexpr float BLAST_UP = 0.4f;
static constexpr float BLAST_SHOW_SECONDS = 0.15f;
static constexpr float STRAIGHTEN_SECONDS = 0.1f;
static constexpr float ARM_LENGTH = 0.45f;
static constexpr float BARREL_LENGTH = 0.4f;
static constexpr float BARREL_RADIUS = 0.15f;
static constexpr float FIRE_PITCH = XMConvertToRadians(20.0f); // 構えの砲身の上向き。
static constexpr float FIRE_RUMBLE_SMALL = 0.5f;
static constexpr float FIRE_RUMBLE_LARGE = 0.8f;
static constexpr float FIRE_RUMBLE_SECONDS = 0.15f;
static constexpr float BLAST_RUMBLE_SMALL = 0.7f;
static constexpr float BLAST_RUMBLE_LARGE = 0.7f;
static constexpr float BLAST_RUMBLE_SECONDS = 0.15f;

enum CANNON_PHASE { CANNON_AIM, CANNON_RECOVER, CANNON_PHASE_COUNT };

CannonArm::CannonArm(int hand, const ARM_LINKS& links) : Arm(hand, links)
{
	Reset();
}

void CannonArm::Reset()
{
	m_Phase.Reset();
	m_Shells.Reset();
	m_BlastTime = 0.0f;
	m_Straighten = 0.0f;
}

void CannonArm::Update(bool trigger)
{
	const float dt = fmaxf(0.0f, GameTimeGetDeltaTime());
	if (GetPlayer().DidRespawn()) Reset();
	if (m_Phase.IsIdle() && trigger) m_Phase.Start(CANNON_AIM);
	const ARM_PHASE_STEP step = m_Phase.Step(dt, PHASE_SECONDS, CANNON_PHASE_COUNT);
	if (step.Entered == CANNON_RECOVER) {
		const VEC3 shoulder = GetShoulder();
		const XMFLOAT3 feet = GetPlayer().GetPosition();
		VEC3 to = Vec3Add({ feet.x, feet.y, feet.z }, Vec3Scale(GetAim(false), NO_TARGET_DISTANCE));
		const int target = GetAimTarget();
		if (target >= 0) {
			const CAPSULE body = GetFighters().GetTarget(target, GetSlot()).Body;
			to = { body.Start.X, body.Start.Y - body.Radius, body.Start.Z }; // 相手の足元
		}
		m_Shells.Fire(shoulder, ProjectileCalcLobVelocity(shoulder, to, SHELL_SPEED, SHELL.Gravity, MAX_RANGE), -1, SHELL);
		Rumble(FIRE_RUMBLE_SMALL, FIRE_RUMBLE_LARGE, FIRE_RUMBLE_SECONDS);
	}
	TENTACLE_TARGET targets[FIGHTER_COUNT];
	for (int i = 0; i < FIGHTER_COUNT; ++i) targets[i] = GetFighters().GetTarget(i, GetSlot());
	PROJECTILE_EVENT events[PROJECTILE_MAX];
	const int count = m_Shells.Step(dt, targets, FIGHTER_COUNT, GetStage().GetStage(), SHELL, events, PROJECTILE_MAX);
	m_BlastTime = fmaxf(0.0f, m_BlastTime - dt);
	for (int e = 0; e < count; ++e) {
		if (events[e].Type == PROJECTILE_EXPIRED && events[e].Position.Y < GetStage().GetStage().OutY) continue; // 場外の下では爆発しない
		if (Explode(events[e].Position, BLAST_RADIUS, BLAST_DAMAGE, BLAST_POWER, BLAST_UP) > 0) {
			Rumble(BLAST_RUMBLE_SMALL, BLAST_RUMBLE_LARGE, BLAST_RUMBLE_SECONDS);
		}
		m_Blast = events[e].Position;
		m_BlastTime = BLAST_SHOW_SECONDS;
	}
	const float speed = dt / STRAIGHTEN_SECONDS;
	m_Straighten = IsActive() ? fminf(1.0f, m_Straighten + speed) : fmaxf(0.0f, m_Straighten - speed);
}

void CannonArm::Draw()
{
	const VEC3 aim = ArmCalcPitchDirection(GetForward(), FIRE_PITCH);
	const VEC3 direction = ArmCalcBlendDirection(GetRestDirection(), aim, m_Straighten);
	const XMFLOAT4 color = GetColor(IsActive());
	const VEC3 hand = DrawStubArm(direction, ARM_LENGTH, color);
	const VEC3 muzzle = Vec3Add(hand, Vec3Scale(direction, BARREL_LENGTH));
	ShapeDrawCylinder({ hand.X, hand.Y, hand.Z }, { muzzle.X, muzzle.Y, muzzle.Z }, BARREL_RADIUS, color);
	for (int i = 0; i < PROJECTILE_MAX; ++i) {
		const PROJECTILE& shell = m_Shells.Get(i);
		if (shell.Active) ShapeDrawSphere({ shell.Position.X, shell.Position.Y, shell.Position.Z }, SHELL.Radius, color);
	}
	if (m_BlastTime > 0.0f) ShapeDrawSphere({ m_Blast.X, m_Blast.Y, m_Blast.Z }, BLAST_RADIUS, GetColor(true));
}

bool CannonArm::IsActive() const
{
	return m_Phase.GetPhase() == CANNON_AIM;
}

bool CannonArm::IsHolding() const
{
	return false;
}

float CannonArm::GetMoveScale() const
{
	return IsActive() ? MOVE_SCALE : 1.0f;
}
