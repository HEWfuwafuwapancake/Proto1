#include "game/arm_rocket.h"
#include "game/fightermanager.h"
#include "game/fighter.h"
#include "game/stage.h"
#include "core/gametime.h"
#include "physics/areacalc.h"
#include "physics/armcalc.h"
#include "render/shape.h"

// 調整用の値
static constexpr ROCKET_PARAMS PARAMS{ 22.0f, 10.0f, 0.35f, 26.0f }; // 速さ・距離・半径・戻る速さ
static constexpr float HIT_DAMAGE = 9.0f;
static constexpr float HIT_POWER = 2.0f;
static constexpr float HIT_UP = 0.2f;
static constexpr float STRAIGHTEN_SECONDS = 0.1f;
static constexpr float ARM_LENGTH = 0.5f;
static constexpr float WRIST_LENGTH = 0.35f; // 拳が飛んでいる間の腕の長さ。
static constexpr float FIST_SIZE = 0.24f;
static constexpr float FIRE_RUMBLE_SMALL = 0.3f;
static constexpr float FIRE_RUMBLE_LARGE = 0.2f;
static constexpr float FIRE_RUMBLE_SECONDS = 0.1f;
static constexpr float HIT_RUMBLE_SMALL = 0.6f;
static constexpr float HIT_RUMBLE_LARGE = 0.6f;
static constexpr float HIT_RUMBLE_SECONDS = 0.15f;

RocketArm::RocketArm(int hand, const ARM_LINKS& links) : Arm(hand, links)
{
	Reset();
}

void RocketArm::Reset()
{
	m_Fist.Reset();
	m_Straighten = 0.0f;
}

void RocketArm::Update(bool trigger)
{
	const float dt = fmaxf(0.0f, GameTimeGetDeltaTime());
	if (GetPlayer().DidRespawn()) Reset();
	const VEC3 shoulder = GetShoulder();
	bool recall = false;
	if (trigger && m_Fist.GetState() == ROCKET_IDLE) {
		if (m_Fist.Launch(shoulder, GetAim(true), PARAMS)) Rumble(FIRE_RUMBLE_SMALL, FIRE_RUMBLE_LARGE, FIRE_RUMBLE_SECONDS);
	}
	else if (trigger && m_Fist.GetState() == ROCKET_FLYING) {
		recall = true;
	}
	TENTACLE_TARGET targets[FIGHTER_COUNT];
	for (int i = 0; i < FIGHTER_COUNT; ++i) targets[i] = GetFighters().GetTarget(i, GetSlot());
	const int hit = m_Fist.Step(recall, dt, shoulder, targets, FIGHTER_COUNT, GetStage().GetStage(), PARAMS);
	if (hit >= 0) {
		GetFighters().Launch(hit, GetSlot(), AreaCalcLaunchDirection(m_Fist.GetDirection(), HIT_UP), HIT_DAMAGE, HIT_POWER);
		Rumble(HIT_RUMBLE_SMALL, HIT_RUMBLE_LARGE, HIT_RUMBLE_SECONDS);
	}
	const float speed = dt / STRAIGHTEN_SECONDS;
	m_Straighten = IsActive() ? fminf(1.0f, m_Straighten + speed) : fmaxf(0.0f, m_Straighten - speed);
}

void RocketArm::Draw()
{
	const VEC3 direction = ArmCalcBlendDirection(GetRestDirection(), m_Fist.GetDirection(), m_Straighten);
	const XMFLOAT4 color = GetColor(IsActive());
	const VEC3 hand = DrawStubArm(direction, IsActive() ? WRIST_LENGTH : ARM_LENGTH, color);
	const VEC3 fist = IsActive() ? m_Fist.GetFist() : hand;
	ShapeDrawBox({ fist.X, fist.Y, fist.Z }, { FIST_SIZE, FIST_SIZE, FIST_SIZE }, color);
}

bool RocketArm::IsActive() const
{
	return m_Fist.GetState() != ROCKET_IDLE;
}

bool RocketArm::IsHolding() const
{
	return false;
}
