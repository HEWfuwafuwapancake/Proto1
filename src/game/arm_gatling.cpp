#include "game/arm_gatling.h"
#include "game/fightermanager.h"
#include "game/fighter.h"
#include "game/stage.h"
#include "core/gametime.h"
#include "physics/areacalc.h"
#include "physics/armcalc.h"
#include "render/shape.h"

// 調整用の値
static constexpr float PHASE_SECONDS[] = { 0.3f, 1.5f, 0.6f }; // 回す・撃つ・休み
static constexpr float SHOT_SECONDS = 0.08f;
static constexpr float MOVE_SCALE = 0.5f;
static constexpr PROJECTILE_PARAMS BULLET{ 0.12f, 0.0f, 0.5f, 0.0f, false }; // 半径・重力なし・寿命・追いかけない・貫通しない
static constexpr float BULLET_SPEED = 40.0f;
static constexpr float SPREAD[] = { XMConvertToRadians(-2.0f), XMConvertToRadians(2.0f), 0.0f }; // 撃った回数で決まるぶれ
static constexpr float HIT_DAMAGE = 1.0f;
static constexpr float HIT_PUSH_SPEED = 2.5f;
static constexpr float STRAIGHTEN_SECONDS = 0.1f;
static constexpr float ARM_LENGTH = 0.5f;
static constexpr float BARREL_LENGTH = 0.45f;
static constexpr float BARREL_RADIUS = 0.025f;
static constexpr float BARREL_OFFSET = 0.06f; // 腕の線から銃身までの距離。
static constexpr float BARREL_SPIN = 20.0f; // 撃つ間の銃身の回る速さ（ラジアン/秒。描画だけ）。
static constexpr float BULLET_DRAW_RADIUS = 0.06f;
static constexpr float SHOT_RUMBLE_SMALL = 0.3f;
static constexpr float SHOT_RUMBLE_LARGE = 0.1f;
static constexpr float SHOT_RUMBLE_SECONDS = 0.08f;

enum GATLING_PHASE { GATLING_SPIN, GATLING_FIRE, GATLING_RECOVER, GATLING_PHASE_COUNT };

GatlingArm::GatlingArm(int hand, const ARM_LINKS& links) : Arm(hand, links)
{
	Reset();
}

void GatlingArm::Reset()
{
	m_Phase.Reset();
	m_Bullets.Reset();
	m_Shots = 0;
	m_Straighten = 0.0f;
}

void GatlingArm::Update(bool trigger)
{
	const float dt = fmaxf(0.0f, GameTimeGetDeltaTime());
	if (GetPlayer().DidRespawn()) Reset();
	if (m_Phase.IsIdle() && trigger) {
		m_Shots = 0;
		m_Direction = GetAim(true);
		m_Phase.Start(GATLING_SPIN);
	}
	else if (m_Phase.GetPhase() == GATLING_FIRE && trigger) {
		m_Phase.Start(GATLING_RECOVER);
	}
	const ARM_PHASE_STEP step = m_Phase.Step(dt, PHASE_SECONDS, GATLING_PHASE_COUNT, SHOT_SECONDS);
	// 撃つ間の刻み（撃つ段階を終えたフレームの刻みも数える）。
	const int ticks = (m_Phase.GetPhase() == GATLING_FIRE && step.Entered != GATLING_FIRE) || step.Entered == GATLING_RECOVER ? step.Ticks : 0;
	for (int t = 0; t < ticks; ++t) {
		m_Direction = AreaCalcRotateYaw(GetAim(true), SPREAD[m_Shots % 3]);
		m_Bullets.Fire(GetShoulder(), Vec3Scale(m_Direction, BULLET_SPEED), -1, BULLET);
		++m_Shots;
	}
	if (ticks > 0) Rumble(SHOT_RUMBLE_SMALL, SHOT_RUMBLE_LARGE, SHOT_RUMBLE_SECONDS);
	TENTACLE_TARGET targets[FIGHTER_COUNT];
	for (int i = 0; i < FIGHTER_COUNT; ++i) targets[i] = GetFighters().GetTarget(i, GetSlot());
	PROJECTILE_EVENT events[PROJECTILE_MAX];
	const int count = m_Bullets.Step(dt, targets, FIGHTER_COUNT, GetStage().GetStage(), BULLET, events, PROJECTILE_MAX);
	for (int e = 0; e < count; ++e) {
		if (events[e].Type != PROJECTILE_HIT_FIGHTER) continue;
		GetFighters().AddDamage(events[e].Target, GetSlot(), HIT_DAMAGE);
		GetFighters().Push(events[e].Target, GetSlot(), Vec3Scale(AreaCalcLaunchDirection(events[e].Direction, 0.0f), HIT_PUSH_SPEED));
	}
	const float speed = dt / STRAIGHTEN_SECONDS;
	m_Straighten = IsActive() ? fminf(1.0f, m_Straighten + speed) : fmaxf(0.0f, m_Straighten - speed);
}

void GatlingArm::Draw()
{
	const VEC3 direction = ArmCalcBlendDirection(GetRestDirection(), m_Direction, m_Straighten);
	const XMFLOAT4 color = GetColor(IsActive());
	const VEC3 hand = DrawStubArm(direction, ARM_LENGTH, color);
	// 腕の線のまわりに3本の銃身を置く（撃つ間だけ回す）。
	VEC3 side = AreaCalcRotateYaw({ direction.X, 0.0f, direction.Z }, XM_PIDIV2);
	const float sideLength = Vec3Length(side);
	side = sideLength > 0.0001f ? Vec3Scale(side, 1.0f / sideLength) : VEC3{ 1.0f, 0.0f, 0.0f };
	const float spin = m_Phase.GetPhase() == GATLING_FIRE ? GameTimeGetElapsed() * BARREL_SPIN : 0.0f;
	for (int k = 0; k < 3; ++k) {
		const float a = spin + XM_2PI * k / 3.0f;
		const VEC3 offset = Vec3Add(Vec3Scale(side, cosf(a) * BARREL_OFFSET), { 0.0f, sinf(a) * BARREL_OFFSET, 0.0f });
		const VEC3 start = Vec3Add(hand, offset);
		const VEC3 end = Vec3Add(start, Vec3Scale(direction, BARREL_LENGTH));
		ShapeDrawCylinder({ start.X, start.Y, start.Z }, { end.X, end.Y, end.Z }, BARREL_RADIUS, color);
	}
	for (int i = 0; i < PROJECTILE_MAX; ++i) {
		const PROJECTILE& bullet = m_Bullets.Get(i);
		if (bullet.Active) ShapeDrawSphere({ bullet.Position.X, bullet.Position.Y, bullet.Position.Z }, BULLET_DRAW_RADIUS, color);
	}
}

bool GatlingArm::IsActive() const
{
	const int phase = m_Phase.GetPhase();
	return phase == GATLING_SPIN || phase == GATLING_FIRE;
}

bool GatlingArm::IsHolding() const
{
	return false;
}

float GatlingArm::GetMoveScale() const
{
	return IsActive() ? MOVE_SCALE : 1.0f;
}
