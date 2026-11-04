#include "game/arm_smoke.h"
#include "game/fightermanager.h"
#include "game/fighter.h"
#include "core/gametime.h"
#include "render/shape.h"

// 調整用の値
static constexpr float PHASE_SECONDS[] = { 5.0f, 1.0f }; // 煙・休み
static constexpr float CLOUD_RADIUS = 3.0f;
static constexpr float CLOUD_HEIGHT = 0.9f; // 足元から煙の中心までの高さ。
static constexpr float HIDE_SECONDS = 0.2f; // 毎フレームかけ直す「隠れる」の秒数（煙を出たらすぐ切れる）。
static constexpr XMFLOAT4 CLOUD_COLOR{ 0.55f, 0.55f, 0.58f, 1.0f };
static constexpr float ARM_LENGTH = 0.45f;
static constexpr float CAN_LENGTH = 0.2f;
static constexpr float CAN_RADIUS = 0.08f;
static constexpr float CLOUD_RUMBLE_SMALL = 0.3f;
static constexpr float CLOUD_RUMBLE_LARGE = 0.3f;
static constexpr float CLOUD_RUMBLE_SECONDS = 0.2f;

enum SMOKE_PHASE { SMOKE_CLOUD, SMOKE_COOLDOWN, SMOKE_PHASE_COUNT };

SmokeArm::SmokeArm(int hand, const ARM_LINKS& links) : Arm(hand, links)
{
	Reset();
}

void SmokeArm::Reset()
{
	m_Phase.Reset();
}

void SmokeArm::Update(bool trigger)
{
	const float dt = fmaxf(0.0f, GameTimeGetDeltaTime());
	if (GetPlayer().DidRespawn()) Reset();
	if (m_Phase.IsIdle() && trigger) {
		const XMFLOAT3 feet = GetPlayer().GetPosition();
		m_Center = { feet.x, feet.y + CLOUD_HEIGHT, feet.z };
		m_Phase.Start(SMOKE_CLOUD);
		Rumble(CLOUD_RUMBLE_SMALL, CLOUD_RUMBLE_LARGE, CLOUD_RUMBLE_SECONDS);
	}
	m_Phase.Step(dt, PHASE_SECONDS, SMOKE_PHASE_COUNT);
	if (m_Phase.GetPhase() != SMOKE_CLOUD) return;
	// 自分も相手も、体の中心が煙の中なら隠れる。
	for (int i = 0; i < FIGHTER_COUNT; ++i) {
		if (!GetFighters().IsActive(i)) continue;
		const CAPSULE body = GetFighters().GetFighter(i).GetBody();
		const VEC3 offset = Vec3Sub(Vec3Scale(Vec3Add(body.Start, body.End), 0.5f), m_Center);
		if (Vec3Length(offset) <= CLOUD_RADIUS) GetFighters().ApplyStatus(i, GetSlot(), STATUS_HIDDEN, HIDE_SECONDS, 0.0f);
	}
}

void SmokeArm::Draw()
{
	const bool cloud = m_Phase.GetPhase() == SMOKE_CLOUD;
	const XMFLOAT4 color = GetColor(cloud);
	const VEC3 hand = DrawStubArm(GetRestDirection(), ARM_LENGTH, color);
	const VEC3 can = Vec3Add(hand, Vec3Scale(GetRestDirection(), CAN_LENGTH));
	ShapeDrawCylinder({ hand.X, hand.Y, hand.Z }, { can.X, can.Y, can.Z }, CAN_RADIUS, color);
	// 外からは中が見えない。中にいるときは球の裏側なので見えない（外が見える）。
	if (cloud) ShapeDrawSphere({ m_Center.X, m_Center.Y, m_Center.Z }, CLOUD_RADIUS, CLOUD_COLOR);
}

bool SmokeArm::IsActive() const
{
	return false;
}

bool SmokeArm::IsHolding() const
{
	return false;
}
