#include "game/arm_c4.h"
#include "game/fightermanager.h"
#include "game/fighter.h"
#include "game/stage.h"
#include "core/gametime.h"
#include "physics/armcalc.h"
#include "render/shape.h"

// 調整用の値
// 耐久の減る量は physics/armcostcalc の表のC4の行（弱：通常の爆弾、強：大きい爆弾、そのほか：場外で消えた爆弾1つ）。
static constexpr ARM_COST_ROW COST_ROW = ARM_COST_ROW_C4;
static constexpr float C4_SECONDS[] = { 1.0f, 0.3f }; // 大型のため・投げたあとの構え（ハンマーの強攻撃より速く強くならないようにする）
static constexpr float THROW_CLIP_SECONDS[] = { 0.0f, 8.0f / 24.0f }; // 単押しの投げ
static constexpr float CHARGE_CLIP_SECONDS[] = { 24.0f / 24.0f, 8.0f / 24.0f }; // 長押しのため・投げ
static constexpr PROJECTILE_PARAMS BOMB_PARAMS{ 0.2f, 20.0f, 5.0f, 0.0f, false };
static constexpr float THROW_SPEED = 12.0f;
static constexpr float THROW_RANGE = 10.0f;
static constexpr float LARGE_THROW_SPEED = 8.0f;
static constexpr float LARGE_THROW_RANGE = 8.0f;
static constexpr float LARGE_SCALE = 1.8f; // 大型の見た目の倍率
static constexpr float CHARGE_MOVE_SCALE = 0.5f;
static constexpr float BLAST_RADIUS = 3.0f;
static constexpr float BLAST_DAMAGE = 15.0f;
static constexpr float BLAST_POWER = 2.4f;
static constexpr float BLAST_UP = 0.8f;
static constexpr float LARGE_BLAST_RADIUS = 5.0f;
static constexpr float LARGE_BLAST_DAMAGE = 18.0f;
static constexpr float LARGE_BLAST_POWER = 2.3f; // ハンマーの長押し（2.4）・単押し（2.8）より弱く
static constexpr float LARGE_BLAST_UP = 1.0f;
static constexpr float BLAST_HEIGHT = 0.3f;
static constexpr float RANGE_SHOW_SECONDS = 0.2f;
static constexpr float THROW_PITCH = XMConvertToRadians(35.0f);
static constexpr float STRAIGHTEN_SECONDS = 0.1f;
static constexpr float MODEL_SCALE = 0.08f * Arm::MODEL_SIZE;
static constexpr float ARM_LENGTH = 19.250364f;
static constexpr float BODY_LENGTH = 4.1f;
static constexpr float BOMB_SCALE = 0.05f;
static constexpr float LAMP_HEIGHT = 0.12f;
static constexpr float LAMP_RADIUS = 0.05f;
static constexpr float LAMP_BLINKS = 3.0f;
static constexpr XMFLOAT4 LAMP_COLOR{ 1.0f, 0.15f, 0.1f, 1.0f };
static constexpr XMFLOAT4 RANGE_COLOR{ 1.0f, 0.6f, 0.2f, 1.0f };
static constexpr float BLAST_RUMBLE_SMALL = 0.8f;
static constexpr float BLAST_RUMBLE_LARGE = 1.0f;
static constexpr float BLAST_RUMBLE_SECONDS = 0.25f;

enum C4_PHASE { C4_CHARGE, C4_THROW, C4_PHASE_COUNT };

C4Arm::C4Arm(int hand, const ARM_LINKS& links) : Arm(hand, links)
{
	m_ArmModel = LoadModel("assets/models/arm_shield_base.obj");
	m_BodyModel = LoadModel("assets/models/arm_c4_body.obj");
	Reset();
}

void C4Arm::Reset()
{
	m_Bombs.Reset();
	m_ChargedClip = false;
	m_Phase.Reset();
	m_Straighten = 0.0f;
	m_RangeSeconds = 0.0f;
	m_RangeCount = 0;
}

void C4Arm::Interrupt()
{
	m_Phase.Reset();
	m_Straighten = 0.0f;
	StepBombs(fmaxf(0.0f, GameTimeGetDeltaTime()));
}

void C4Arm::ThrowBomb(bool large)
{
	const VEC3 origin = GetShoulder();
	const XMFLOAT3 feet = GetPlayer().GetPosition();
	m_Direction = GetAim(false);
	const float range = large ? LARGE_THROW_RANGE : THROW_RANGE;
	const float speed = large ? LARGE_THROW_SPEED : THROW_SPEED;
	// 届く範囲の相手の足元を狙い、届かなければ同じ向きの最大距離へ投げる。
	VEC3 goal = Vec3Add({ feet.x, feet.y, feet.z }, Vec3Scale(m_Direction, range));
	const int target = GetAimTarget();
	if (target >= 0) {
		const CAPSULE body = GetFighters().GetTarget(target, GetSlot()).Body;
		goal = { body.Start.X, body.Start.Y - body.Radius, body.Start.Z };
	}
	m_Bombs.Throw(origin, ProjectileCalcLobVelocity(origin, goal, speed, BOMB_PARAMS.Gravity, range), BOMB_PARAMS, large);
	m_Phase.Start(C4_THROW);
	m_ChargedClip = large;
}

void C4Arm::StepBombs(float dt)
{
	TENTACLE_TARGET targets[FIGHTER_COUNT];
	for (int i = 0; i < FIGHTER_COUNT; ++i) targets[i] = GetFighters().GetTarget(i, GetSlot());
	C4_IMPACT impacts[PROJECTILE_MAX];
	int expired = 0;
	const int count = m_Bombs.Step(dt, targets, FIGHTER_COUNT, GetStage().GetStage(), BOMB_PARAMS, impacts, PROJECTILE_MAX, &expired);
	for (int i = 0; i < expired; ++i) ConsumeDurability(GetCost(COST_ROW, ARM_COST_EXTRA));
	m_RangeSeconds = fmaxf(0.0f, m_RangeSeconds - dt);
	if (count == 0) return;
	m_RangeCount = count;
	for (int i = 0; i < count; ++i) {
		const bool large = impacts[i].Large;
		m_RangeCenters[i] = Vec3Add(impacts[i].Position, { 0.0f, BLAST_HEIGHT, 0.0f });
		m_RangeRadii[i] = large ? LARGE_BLAST_RADIUS : BLAST_RADIUS;
		int affected = 0;
		Explode(m_RangeCenters[i], m_RangeRadii[i], large ? LARGE_BLAST_DAMAGE : BLAST_DAMAGE,
			large ? LARGE_BLAST_POWER : BLAST_POWER, large ? LARGE_BLAST_UP : BLAST_UP, &affected);
		ConsumeDurability(GetCost(COST_ROW, large ? (affected > 0 ? ARM_COST_HOLD_HIT : ARM_COST_HOLD_MISS)
			: (affected > 0 ? ARM_COST_TAP_HIT : ARM_COST_TAP_MISS)));
	}
	m_RangeSeconds = RANGE_SHOW_SECONDS;
	Rumble(BLAST_RUMBLE_SMALL, BLAST_RUMBLE_LARGE, BLAST_RUMBLE_SECONDS);
}

void C4Arm::Update(const ARM_INPUT& input)
{
	const float dt = fmaxf(0.0f, GameTimeGetDeltaTime());
	if (GetPlayer().DidRespawn()) Reset();
	if (m_Phase.IsIdle()) {
		if (input.Tap) {
			ThrowBomb(false);
		}
		else if (input.HoldStarted) {
			m_Direction = GetAim(false);
			m_Phase.Start(C4_CHARGE);
			m_ChargedClip = true;
		}
	}
	const ARM_PHASE_STEP step = m_Phase.Step(dt, C4_SECONDS, C4_PHASE_COUNT);
	if (step.Entered == C4_THROW) ThrowBomb(true);
	StepBombs(dt);
	const float speed = dt / STRAIGHTEN_SECONDS;
	m_Straighten = m_Phase.IsIdle() ? fmaxf(0.0f, m_Straighten - speed) : fminf(1.0f, m_Straighten + speed);
}

void C4Arm::DrawBomb(const VEC3& feet, bool large, const XMFLOAT4& color) const
{
	const float scale = BOMB_SCALE * (large ? LARGE_SCALE : 1.0f);
	const VEC3 top = Vec3Add(feet, { 0.0f, BODY_LENGTH * scale, 0.0f });
	DrawAlong(m_BodyModel, top, { 0.0f, -1.0f, 0.0f }, BODY_LENGTH, scale, color);
	if (fmodf(GameTimeGetElapsed() * LAMP_BLINKS, 1.0f) < 0.5f) {
		ShapeDrawSphere({ feet.X, feet.Y + LAMP_HEIGHT * (large ? LARGE_SCALE : 1.0f), feet.Z },
			LAMP_RADIUS * (large ? LARGE_SCALE : 1.0f), LAMP_COLOR);
	}
}

void C4Arm::Draw()
{
	const VEC3 aim = ArmCalcPitchDirection(m_Direction, THROW_PITCH);
	VEC3 direction = ArmCalcBlendDirection(GetRestDirection(), aim, m_Straighten);
	VEC3 unusedWeapon = direction;
	BlendClipPose(direction, unusedWeapon);
	const XMFLOAT4 color = GetColor(!m_Phase.IsIdle());
	const float scale = MODEL_SCALE * GetDrawScale();
	const VEC3 hand = DrawAlong(m_ArmModel, GetDrawShoulder(), direction, ARM_LENGTH, scale, color);
	if (m_Phase.GetPhase() != C4_THROW) {
		const float size = m_Phase.GetPhase() == C4_CHARGE ? LARGE_SCALE : 1.0f;
		DrawAlong(m_BodyModel, hand, direction, BODY_LENGTH, scale * size, color);
	}
	const ProjectileCalc& shots = m_Bombs.GetThrown();
	for (int i = 0; i < PROJECTILE_MAX; ++i) {
		if (shots.Get(i).Active) DrawBomb(shots.Get(i).Position, m_Bombs.IsLarge(i), GetColor(false));
	}
	if (m_RangeSeconds > 0.0f) {
		for (int i = 0; i < m_RangeCount; ++i) DrawRangeRing(m_RangeCenters[i], m_RangeRadii[i], RANGE_COLOR);
	}
}

bool C4Arm::IsActive() const
{
	return !m_Phase.IsIdle();
}

ARM_BODY_CLIP C4Arm::GetBodyClip() const
{
	if (m_Phase.IsIdle()) return {};
	return { true, m_ChargedClip ? ATTACK_CLIP::C4Charge : ATTACK_CLIP::C4Throw,
		ArmCalcClipTime(m_Phase.GetPhase(), m_Phase.GetTime(), C4_SECONDS,
			m_ChargedClip ? CHARGE_CLIP_SECONDS : THROW_CLIP_SECONDS), false };
}

bool C4Arm::IsBusy() const
{
	return m_RangeSeconds > 0.0f || m_Bombs.GetThrown().GetCount() > 0;
}

bool C4Arm::IsHolding() const
{
	return false;
}

float C4Arm::GetMoveScale() const
{
	return m_Phase.GetPhase() == C4_CHARGE ? CHARGE_MOVE_SCALE : 1.0f;
}
