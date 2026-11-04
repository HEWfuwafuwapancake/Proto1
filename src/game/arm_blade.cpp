#include "game/arm_blade.h"
#include "game/fightermanager.h"
#include "game/fighter.h"
#include "core/gametime.h"
#include "physics/areacalc.h"
#include "physics/armcalc.h"
#include "render/shape.h"

// 調整用の値
static constexpr BLADE_PARAMS PARAMS{ 0.08f, 0.08f, 0.35f, 0.3f, 3 };
static constexpr float SLASH_RADIUS = 1.8f;
static constexpr float SLASH_HALF_ANGLE = XMConvertToRadians(60.0f);
static constexpr float SLASH_HALF_HEIGHT = 1.0f;
static constexpr float SLASH_DAMAGE = 4.0f; // 1・2振り目
static constexpr float SLASH_PUSH_SPEED = 1.5f; // 1・2振り目は % を変えずにこの速さで押す（3 だと3振り目が届かなかった）
static constexpr float SLASH_PUSH_UP = 0.2f;
static constexpr float FINISH_DAMAGE = 6.0f; // 3振り目
static constexpr float FINISH_POWER = 1.8f;
static constexpr float FINISH_UP = 0.3f;
static constexpr float STRAIGHTEN_SECONDS = 0.1f;
static constexpr float ARM_LENGTH = 0.5f;
static constexpr float BLADE_LENGTH = 0.9f;
static constexpr float BLADE_RADIUS = 0.03f;
static constexpr float SWING_DRAW_ANGLE = XMConvertToRadians(60.0f); // 振りの描画で回す片側の角度。
static constexpr float HIT_RUMBLE_SMALL = 0.4f;
static constexpr float HIT_RUMBLE_LARGE = 0.2f;
static constexpr float HIT_RUMBLE_SECONDS = 0.1f;
static constexpr float FINISH_RUMBLE_SMALL = 0.6f;
static constexpr float FINISH_RUMBLE_LARGE = 0.6f;
static constexpr float FINISH_RUMBLE_SECONDS = 0.15f;

BladeArm::BladeArm(int hand, const ARM_LINKS& links) : Arm(hand, links)
{
	Reset();
}

void BladeArm::Reset()
{
	m_Blade.Reset();
	m_Straighten = 0.0f;
}

void BladeArm::Update(bool trigger)
{
	const float dt = fmaxf(0.0f, GameTimeGetDeltaTime());
	if (m_Blade.GetState() == BLADE_IDLE && trigger) m_Direction = GetAim(false);
	const int slash = m_Blade.Step(trigger, GetPlayer().DidRespawn(), dt, PARAMS);
	if (slash > 0) {
		const VEC3 shoulder = GetShoulder();
		const bool finish = slash >= PARAMS.MaxSlash;
		bool hit = false;
		for (int i = 0; i < FIGHTER_COUNT; ++i) {
			const TENTACLE_TARGET target = GetFighters().GetTarget(i, GetSlot());
			if (!target.Available) continue;
			if (!AreaCalcInSector(shoulder, m_Direction, 0.0f, SLASH_RADIUS, SLASH_HALF_ANGLE, SLASH_HALF_HEIGHT, target.Body)) continue;
			if (finish) {
				GetFighters().Launch(i, GetSlot(), AreaCalcLaunchDirection(m_Direction, FINISH_UP), FINISH_DAMAGE, FINISH_POWER);
			}
			else {
				GetFighters().AddDamage(i, GetSlot(), SLASH_DAMAGE);
				GetFighters().Push(i, GetSlot(), Vec3Scale(AreaCalcLaunchDirection(m_Direction, SLASH_PUSH_UP), SLASH_PUSH_SPEED));
			}
			hit = true;
		}
		if (hit && finish) Rumble(FINISH_RUMBLE_SMALL, FINISH_RUMBLE_LARGE, FINISH_RUMBLE_SECONDS);
		else if (hit) Rumble(HIT_RUMBLE_SMALL, HIT_RUMBLE_LARGE, HIT_RUMBLE_SECONDS);
	}
	const float speed = dt / STRAIGHTEN_SECONDS;
	const bool idle = m_Blade.GetState() == BLADE_IDLE;
	m_Straighten = idle ? fmaxf(0.0f, m_Straighten - speed) : fminf(1.0f, m_Straighten + speed);
}

void BladeArm::Draw()
{
	// 1・3振り目は右から左、2振り目は左から右へ刃を回して見せる（左手は鏡写し）。
	const float side = (m_Blade.GetSlash() % 2 == 1 ? 1.0f : -1.0f) * (GetHand() == 0 ? -1.0f : 1.0f);
	float ratio = 0.0f;
	if (m_Blade.GetState() == BLADE_SWING) ratio = fminf(1.0f, m_Blade.GetTime() / PARAMS.SwingSeconds);
	else if (m_Blade.GetState() == BLADE_WINDOW || m_Blade.GetState() == BLADE_COOLDOWN) ratio = 1.0f;
	const VEC3 swing = AreaCalcRotateYaw(m_Direction, side * SWING_DRAW_ANGLE * (1.0f - 2.0f * ratio));
	const VEC3 direction = ArmCalcBlendDirection(GetRestDirection(), swing, m_Straighten);
	const XMFLOAT4 color = GetColor(m_Blade.GetState() != BLADE_IDLE);
	const VEC3 hand = DrawStubArm(direction, ARM_LENGTH, color);
	const VEC3 tip = Vec3Add(hand, Vec3Scale(direction, BLADE_LENGTH));
	ShapeDrawCylinder({ hand.X, hand.Y, hand.Z }, { tip.X, tip.Y, tip.Z }, BLADE_RADIUS, color);
}

bool BladeArm::IsActive() const
{
	return m_Blade.GetState() != BLADE_IDLE;
}

bool BladeArm::IsHolding() const
{
	return false;
}
