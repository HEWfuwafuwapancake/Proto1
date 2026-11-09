#include "game/arm_katana.h"
#include "game/fightermanager.h"
#include "game/fighter.h"
#include "core/gametime.h"
#include "core/mathutil.h"
#include "physics/areacalc.h"
#include "physics/armcalc.h"

// 調整用の値
static constexpr float SLASH_SECONDS[] = { 0.08f, 0.12f, 0.2f }; // 単押し：構え・斬り・休み
static constexpr float RUSH_SECONDS[] = { 0.0f, 1.5f, 0.2f }; // 長押し：（構えなし）・連続斬り・休み
static constexpr float SLASH_RADIUS = 2.5f;
static constexpr float SLASH_HALF_ANGLE = XMConvertToRadians(70.0f);
static constexpr float SLASH_HALF_HEIGHT = 1.5f; // 肩の高さから上下それぞれの当たる幅。
static constexpr float SLASH_DAMAGE = 9.0f;
static constexpr float SLASH_POWER = 2.0f;
static constexpr float SLASH_UP = 0.6f; // 水平の単位方向に足す上向きの量。
static constexpr float RUSH_SPEED = 12.0f;
static constexpr float RUSH_TICK_SECONDS = 0.15f; // 連続斬りの間隔。
static constexpr float RUSH_CUT_DAMAGE = 3.0f;
static constexpr float RUSH_LAST_POWER = 2.0f; // 最後の一振りの吹っ飛ばす強さ。
static constexpr float STRAIGHTEN_SECONDS = 0.08f;
static constexpr float CARRY_PITCH = XMConvertToRadians(-35.0f); // 待機のときの刀の上下の向き。
static constexpr float SWING_ANGLE = XMConvertToRadians(70.0f); // 描画の左右の振り幅。
static constexpr float MODEL_SCALE = 0.08f * Arm::MODEL_SIZE;
static constexpr float ARM_LENGTH = 19.291699f; // arm_hasami_base の長さ（cm）。
static constexpr float BLADE_LENGTH = 38.7f; // arm_katana_blade の長さ（cm）。
static constexpr float HIT_RUMBLE_SMALL = 0.5f;
static constexpr float HIT_RUMBLE_LARGE = 0.5f;
static constexpr float HIT_RUMBLE_SECONDS = 0.1f;

enum KATANA_PHASE { KATANA_WINDUP, KATANA_SLASH, KATANA_RECOVER, KATANA_PHASE_COUNT };

KatanaArm::KatanaArm(int hand, const ARM_LINKS& links) : Arm(hand, links)
{
	m_ArmModel = LoadModel("assets/models/arm_hasami_base.obj");
	m_BladeModel = LoadModel("assets/models/arm_katana_blade.obj");
	Reset();
}

void KatanaArm::Reset()
{
	m_Phase.Reset();
	m_Rush = false;
	m_Straighten = 0.0f;
}

bool KatanaArm::IsRushing() const
{
	return m_Rush && m_Phase.GetPhase() == KATANA_SLASH;
}

void KatanaArm::Slash(float damage, float power)
{
	bool hit = false;
	for (int i = 0; i < FIGHTER_COUNT; ++i) {
		const TENTACLE_TARGET target = GetFighters().GetTarget(i, GetSlot());
		if (!target.Available || !AreaCalcInSector(GetShoulder(), m_Direction, 0.0f, SLASH_RADIUS, SLASH_HALF_ANGLE, SLASH_HALF_HEIGHT, target.Body)) continue;
		GetFighters().Launch(i, GetSlot(), AreaCalcLaunchDirection(m_Direction, SLASH_UP), damage, power);
		hit = true;
	}
	if (hit) Rumble(HIT_RUMBLE_SMALL, HIT_RUMBLE_LARGE, HIT_RUMBLE_SECONDS);
}

void KatanaArm::AddCutDamage()
{
	bool hit = false;
	for (int i = 0; i < FIGHTER_COUNT; ++i) {
		const TENTACLE_TARGET target = GetFighters().GetTarget(i, GetSlot());
		if (!target.Available || !AreaCalcInSector(GetShoulder(), m_Direction, 0.0f, SLASH_RADIUS, SLASH_HALF_ANGLE, SLASH_HALF_HEIGHT, target.Body)) continue;
		GetFighters().AddDamage(i, GetSlot(), RUSH_CUT_DAMAGE);
		hit = true;
	}
	if (hit) Rumble(HIT_RUMBLE_SMALL, HIT_RUMBLE_LARGE, HIT_RUMBLE_SECONDS);
}

void KatanaArm::FinishRush()
{
	Slash(RUSH_CUT_DAMAGE, RUSH_LAST_POWER);
	m_Phase.Start(KATANA_RECOVER);
}

void KatanaArm::Update(const ARM_INPUT& input)
{
	const float dt = fmaxf(0.0f, GameTimeGetDeltaTime());
	if (GetPlayer().DidRespawn()) Reset();
	if (m_Phase.IsIdle() && input.Tap) {
		m_Rush = false;
		m_Direction = GetAim(false);
		m_Phase.Start(KATANA_WINDUP);
	}
	else if (m_Phase.IsIdle() && input.HoldStarted) {
		m_Rush = true;
		m_Direction = GetAim(false);
		m_Phase.Start(KATANA_SLASH);
	}
	const bool wasRushing = IsRushing();
	const ARM_PHASE_STEP step = m_Phase.Step(dt, m_Rush ? RUSH_SECONDS : SLASH_SECONDS, KATANA_PHASE_COUNT, wasRushing ? RUSH_TICK_SECONDS : 0.0f);
	if (!m_Rush && step.Entered == KATANA_SLASH) Slash(SLASH_DAMAGE, SLASH_POWER);
	if (wasRushing) {
		if (step.Entered == KATANA_RECOVER) {
			// 時間切れ：段階は休みへ進んだので、最後の一振りだけ効かせる。
			Slash(RUSH_CUT_DAMAGE, RUSH_LAST_POWER);
		}
		else if (!input.Pressed) {
			FinishRush();
		}
		else {
			m_Direction = GetAim(false);
			for (int i = 0; i < step.Ticks; ++i) AddCutDamage();
		}
	}
	const float speed = dt / STRAIGHTEN_SECONDS;
	m_Straighten = m_Phase.IsIdle() ? fmaxf(0.0f, m_Straighten - speed) : fminf(1.0f, m_Straighten + speed);
}

void KatanaArm::Draw()
{
	// 斬りの間は、左右の振りを時間で動かす（連続斬りは刻みごとに振り直す）。
	float swing = 0.0f;
	if (m_Phase.GetPhase() == KATANA_SLASH) {
		const float seconds = m_Rush ? RUSH_TICK_SECONDS : SLASH_SECONDS[KATANA_SLASH];
		const float t = fmodf(m_Phase.GetTime(), seconds) / seconds;
		swing = Lerp(SWING_ANGLE, -SWING_ANGLE, t);
	}
	const float side = GetHand() == 0 ? -1.0f : 1.0f;
	const VEC3 aim = AreaCalcRotateYaw(m_Direction, side * swing);
	const VEC3 armDirection = ArmCalcBlendDirection(GetRestDirection(), aim, m_Straighten);
	const VEC3 bladeDirection = ArmCalcBlendDirection(ArmCalcPitchDirection(GetForward(), CARRY_PITCH), aim, m_Straighten);
	const XMFLOAT4 color = GetColor(!m_Phase.IsIdle());
	const float scale = MODEL_SCALE * GetDrawScale();
	const VEC3 hand = DrawAlong(m_ArmModel, GetDrawShoulder(), armDirection, ARM_LENGTH, scale, color);
	DrawAlong(m_BladeModel, hand, bladeDirection, BLADE_LENGTH, scale, color);
}

bool KatanaArm::IsActive() const
{
	return !m_Phase.IsIdle();
}

bool KatanaArm::IsHolding() const
{
	return false;
}

VEC3 KatanaArm::GetRushVelocity() const
{
	return IsRushing() ? Vec3Scale(m_Direction, RUSH_SPEED) : VEC3{};
}
