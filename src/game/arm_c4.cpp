#include "game/arm_c4.h"
#include "game/fightermanager.h"
#include "game/fighter.h"
#include "game/stage.h"
#include "core/gametime.h"
#include "physics/armcalc.h"
#include "render/shape.h"

// 調整用の値
static constexpr float THROW_SECONDS[] = { 0.3f }; // 投げたあとの構え。
static constexpr PROJECTILE_PARAMS BOMB_PARAMS{ 0.2f, 20.0f, 5.0f, 0.0f, false };
static constexpr float THROW_SPEED = 12.0f; // 投げる水平の速さ。
static constexpr float THROW_RANGE = 10.0f; // 投げて届く水平の距離。
static constexpr float BLAST_RADIUS = 3.0f;
static constexpr float BLAST_DAMAGE = 15.0f;
static constexpr float BLAST_POWER = 2.4f;
static constexpr float BLAST_UP = 0.8f;
static constexpr float BLAST_HEIGHT = 0.3f; // 爆心の、設置した足場からの高さ。
static constexpr float RANGE_SHOW_SECONDS = 0.2f; // 爆発の範囲を見せる秒数。
static constexpr float THROW_PITCH = XMConvertToRadians(35.0f); // 投げる腕の上下の角度。
static constexpr float STRAIGHTEN_SECONDS = 0.1f;
static constexpr float MODEL_SCALE = 0.08f * Arm::MODEL_SIZE;
static constexpr float ARM_LENGTH = 19.250364f; // arm_shield_base の長さ（cm）。
static constexpr float BODY_LENGTH = 4.1f; // arm_c4_body の厚さ（cm）。
static constexpr float BOMB_SCALE = 0.05f; // 置いた C4 の cm からゲーム単位への倍率。
static constexpr float LAMP_HEIGHT = 0.12f; // ランプの、C4 の下の面からの高さ。
static constexpr float LAMP_RADIUS = 0.05f;
static constexpr float LAMP_BLINKS = 3.0f; // ランプが1秒に点滅する回数。
static constexpr XMFLOAT4 LAMP_COLOR{ 1.0f, 0.15f, 0.1f, 1.0f };
static constexpr XMFLOAT4 RANGE_COLOR{ 1.0f, 0.6f, 0.2f, 1.0f };
static constexpr float BLAST_RUMBLE_SMALL = 0.8f;
static constexpr float BLAST_RUMBLE_LARGE = 1.0f;
static constexpr float BLAST_RUMBLE_SECONDS = 0.25f;

enum C4_PHASE { C4_THROW, C4_PHASE_COUNT };

C4Arm::C4Arm(int hand, const ARM_LINKS& links) : Arm(hand, links)
{
	m_ArmModel = LoadModel("assets/models/arm_shield_base.obj");
	m_BodyModel = LoadModel("assets/models/arm_c4_body.obj");
	Reset();
}

void C4Arm::Reset()
{
	m_Bombs.Reset();
	m_Phase.Reset();
	m_Straighten = 0.0f;
	m_RangeSeconds = 0.0f;
}

void C4Arm::Interrupt()
{
	m_Phase.Reset();
	m_Straighten = 0.0f;
	StepBombs(fmaxf(0.0f, GameTimeGetDeltaTime()));
}

void C4Arm::ThrowBomb()
{
	const VEC3 origin = GetShoulder();
	const XMFLOAT3 feet = GetPlayer().GetPosition();
	m_Direction = GetAim(false);
	// ロックオン中の相手が届く所にいれば足元へ、いなければ前へいちばん遠くまで投げる。
	VEC3 goal = Vec3Add({ feet.x, feet.y, feet.z }, Vec3Scale(m_Direction, THROW_RANGE));
	const int target = GetAimTarget();
	if (target >= 0) {
		const CAPSULE body = GetFighters().GetTarget(target, GetSlot()).Body;
		goal = { body.Start.X, body.Start.Y - body.Radius, body.Start.Z };
	}
	m_Bombs.Throw(origin, ProjectileCalcLobVelocity(origin, goal, THROW_SPEED, BOMB_PARAMS.Gravity, THROW_RANGE), BOMB_PARAMS);
	m_Phase.Start(C4_THROW);
}

void C4Arm::Detonate()
{
	VEC3 positions[C4_MAX];
	m_RangeCount = m_Bombs.Detonate(positions);
	if (m_RangeCount == 0) return;
	int hits = 0;
	for (int i = 0; i < m_RangeCount; ++i) {
		m_RangeCenters[i] = Vec3Add(positions[i], { 0.0f, BLAST_HEIGHT, 0.0f });
		hits += Explode(m_RangeCenters[i], BLAST_RADIUS, BLAST_DAMAGE, BLAST_POWER, BLAST_UP);
	}
	m_RangeSeconds = RANGE_SHOW_SECONDS;
	if (hits > 0) Rumble(BLAST_RUMBLE_SMALL, BLAST_RUMBLE_LARGE, BLAST_RUMBLE_SECONDS);
}

void C4Arm::StepBombs(float dt)
{
	TENTACLE_TARGET targets[FIGHTER_COUNT];
	for (int i = 0; i < FIGHTER_COUNT; ++i) targets[i] = GetFighters().GetTarget(i, GetSlot());
	m_Bombs.Step(dt, targets, FIGHTER_COUNT, GetStage().GetStage(), BOMB_PARAMS);
	m_RangeSeconds = fmaxf(0.0f, m_RangeSeconds - dt);
}

void C4Arm::Update(const ARM_INPUT& input)
{
	const float dt = fmaxf(0.0f, GameTimeGetDeltaTime());
	if (GetPlayer().DidRespawn()) m_Phase.Reset();
	if (input.Tap) Detonate();
	if (input.HoldStarted && m_Phase.IsIdle()) ThrowBomb();
	m_Phase.Step(dt, THROW_SECONDS, C4_PHASE_COUNT);
	StepBombs(dt);
	const float speed = dt / STRAIGHTEN_SECONDS;
	m_Straighten = m_Phase.IsIdle() ? fmaxf(0.0f, m_Straighten - speed) : fminf(1.0f, m_Straighten + speed);
}

void C4Arm::DrawBomb(const VEC3& feet, const XMFLOAT4& color) const
{
	const VEC3 top = Vec3Add(feet, { 0.0f, BODY_LENGTH * BOMB_SCALE, 0.0f });
	DrawAlong(m_BodyModel, top, { 0.0f, -1.0f, 0.0f }, BODY_LENGTH, BOMB_SCALE, color);
	if (fmodf(GameTimeGetElapsed() * LAMP_BLINKS, 1.0f) < 0.5f) {
		ShapeDrawSphere({ feet.X, feet.Y + LAMP_HEIGHT, feet.Z }, LAMP_RADIUS, LAMP_COLOR);
	}
}

void C4Arm::Draw()
{
	const VEC3 aim = ArmCalcPitchDirection(m_Direction, THROW_PITCH);
	const VEC3 direction = ArmCalcBlendDirection(GetRestDirection(), aim, m_Straighten);
	const XMFLOAT4 color = GetColor(!m_Phase.IsIdle());
	const float scale = MODEL_SCALE * GetDrawScale();
	const VEC3 hand = DrawAlong(m_ArmModel, GetDrawShoulder(), direction, ARM_LENGTH, scale, color);
	if (m_Phase.IsIdle()) DrawAlong(m_BodyModel, hand, direction, BODY_LENGTH, scale, color);
	const XMFLOAT4 bombColor = GetColor(false);
	for (const ProjectileCalc* shots : { &m_Bombs.GetThrown(), &m_Bombs.GetFalling() }) {
		for (int i = 0; i < PROJECTILE_MAX; ++i) {
			if (shots->Get(i).Active) DrawBomb(shots->Get(i).Position, bombColor);
		}
	}
	for (int i = 0; i < m_Bombs.GetPlacedCount(); ++i) DrawBomb(m_Bombs.GetPlaced(i), bombColor);
	if (m_RangeSeconds > 0.0f) {
		for (int i = 0; i < m_RangeCount; ++i) DrawRangeRing(m_RangeCenters[i], BLAST_RADIUS, RANGE_COLOR);
	}
}

bool C4Arm::IsActive() const
{
	return !m_Phase.IsIdle();
}

bool C4Arm::IsHolding() const
{
	return false;
}
