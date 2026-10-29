#include "game/fighter.h"
#include "game/fightermanager.h"
#include "game/stage.h"
#include "game/camera.h"
#include "game/arm.h"
#include "core/gametime.h"
#include "physics/cameracalc.h"
#include "render/shape.h"

// 調整用の値（従来のPlayer・DummyManagerと同じ）
static constexpr float MOVE_SPEED = 6.0f;
static constexpr float TURN_SPEED = XMConvertToRadians(720.0f);
static constexpr float JUMP_SPEED = 9.0f;
static constexpr float GRAVITY = 25.0f;
static constexpr float BODY_RADIUS = 0.5f;
static constexpr float BODY_HEIGHT = 1.8f;
static constexpr float RESPAWN_SECONDS = 2.0f;
static constexpr XMFLOAT4 PLAYER_COLORS[FIGHTER_COUNT] = {
	{ 0.95f, 0.2f, 0.2f, 1.0f }, { 0.2f, 0.45f, 1.0f, 1.0f },
	{ 0.2f, 0.85f, 0.3f, 1.0f }, { 1.0f, 0.85f, 0.15f, 1.0f }
};
static constexpr float TARGET_MARKER_RADIUS = 0.16f;
static constexpr float TARGET_MARKER_HEIGHT = 0.2f;
static constexpr XMFLOAT4 DUMMY_COLOR{ 0.35f, 0.55f, 0.95f, 1.0f };
static constexpr XMFLOAT4 TARGET_COLOR{ 1.0f, 0.55f, 0.15f, 1.0f };
static constexpr XMFLOAT4 LAUNCHED_COLOR{ 0.95f, 0.85f, 0.3f, 1.0f };
static constexpr float KNOCKBACK_BASE_SPEED = 4.0f;
static constexpr float KNOCKBACK_GROWTH = 0.12f;
static constexpr float KNOCKBACK_ANGLE = XMConvertToRadians(30.0f);
static constexpr float KNOCKBACK_AIR_DRAG = 0.5f;
static constexpr float KNOCKBACK_FRICTION = 20.0f;
static constexpr float KNOCKBACK_STOP_SPEED = 0.5f;

KNOCKBACK_PARAMS Fighter::KnockbackParams() const
{
	return { KNOCKBACK_BASE_SPEED, KNOCKBACK_GROWTH, KNOCKBACK_ANGLE,
		KNOCKBACK_AIR_DRAG, KNOCKBACK_FRICTION, KNOCKBACK_STOP_SPEED, GRAVITY };
}

XMFLOAT4 Fighter::GetPlayerColor(int slot)
{
	return slot >= 0 && slot < FIGHTER_COUNT ? PLAYER_COLORS[slot] : XMFLOAT4{ 1.0f, 1.0f, 1.0f, 1.0f };
}

void Fighter::Initialize(const Stage& stage, int slot, int device, const VEC3& spawn)
{
	m_Stage = &stage;
	m_Slot = slot;
	m_Device = device;
	m_Spawn = spawn;
	m_Input = {};
	m_PreviousInput = {};
	Reset();
}

void Fighter::PlaceAtSpawn()
{
	m_Body = {};
	m_Body.SetPosition(m_Spawn);
	m_Body.SetOnGround(true);
	m_Percent = 0.0f;
	m_Yaw = 0.0f;
	m_Launched = false;
	m_HeldBy = -1;
	m_Pulled = false;
}

void Fighter::Reset()
{
	m_Life.Reset();
	m_Respawned = false;
	PlaceAtSpawn();
}

ACTION_AXIS Fighter::GetMoveInput() const
{
	return ActionCalcPlayerMove(m_Input);
}

ACTION_AXIS Fighter::GetLookInput() const
{
	return ActionCalcPlayerLook(m_Input);
}

bool Fighter::IsTrigger(ACTION action) const
{
	return ActionCalcPlayerTrigger(m_Input, m_PreviousInput, action);
}

void Fighter::SetInput(const PLAYER_INPUT& current, const PLAYER_INPUT& previous)
{
	m_Input = IsHuman() ? current : PLAYER_INPUT{};
	m_PreviousInput = IsHuman() ? previous : PLAYER_INPUT{};
}

DUMMY_EVENT Fighter::Update(const Camera* camera, const ArmManager* arms)
{
	m_Respawned = false;
	const float dt = GameTimeGetDeltaTime();
	const STAGE& stage = m_Stage->GetStage();
	if (IsActive() && !IsHeld()) {
		if (m_Launched) {
			m_Launched = KnockbackCalcStep(m_Body, dt, KnockbackParams(), stage);
		}
		else if (!m_Pulled) {
			PLAYER_PARAMS params{};
			params.Gravity = GRAVITY;
			float moveX = 0.0f, moveZ = 0.0f;
			bool jump = false;
			if (IsHuman() && camera != nullptr && arms != nullptr) {
				const ACTION_AXIS move = GetMoveInput();
				const float yaw = camera->GetYaw();
				moveX = cosf(yaw) * move.X + sinf(yaw) * move.Y;
				moveZ = -sinf(yaw) * move.X + cosf(yaw) * move.Y;
				params.MoveSpeed = MOVE_SPEED * arms->GetMoveScale();
				params.JumpSpeed = JUMP_SPEED;
				jump = IsTrigger(ACTION_JUMP);
			}
			m_Body.Step(moveX, moveZ, jump, dt, params, stage);
		}
	}
	const DUMMY_EVENT event = m_Life.Update(StageCalcIsOut(stage, m_Body.GetY()), RESPAWN_SECONDS, dt);
	if (event == DUMMY_EVENT_OUT) {
		m_HeldBy = -1;
		m_Pulled = false;
	}
	else if (event == DUMMY_EVENT_RESPAWN) {
		PlaceAtSpawn();
		m_Respawned = true;
	}
	return event;
}

void Fighter::UpdateFacing(const Camera& camera, const ArmManager& arms, const FighterManager& fighters, int slot)
{
	if (!CanUseArms()) return;
	if (m_Respawned) {
		m_Yaw = camera.GetYaw();
		return;
	}
	const bool armActive = arms.IsActive() || IsTrigger(ACTION_ARM_LEFT) || IsTrigger(ACTION_ARM_RIGHT);
	const int target = camera.GetTarget();
	float targetYaw = m_Yaw;
	if (target >= 0) {
		const CAPSULE body = fighters.GetTarget(target, slot).Body;
		const VEC3 center = Vec3Scale(Vec3Add(body.Start, body.End), 0.5f);
		targetYaw = CameraCalcYawTo({ m_Body.GetX(), m_Body.GetY(), m_Body.GetZ() }, center, m_Yaw);
	}
	m_Yaw = PlayerCalcFacing(m_Yaw, m_Body.GetVelocityX(), m_Body.GetVelocityZ(), camera.GetYaw(),
		armActive, TURN_SPEED, GameTimeGetDeltaTime(), target >= 0, targetYaw);
}

void Fighter::Draw(bool highlighted) const
{
	if (!IsActive()) return;
	// 人の体は吹っ飛び中・ターゲット中も本人の色を保つ。
	const XMFLOAT4 color = IsHuman() ? GetPlayerColor(m_Slot)
		: (m_Launched ? LAUNCHED_COLOR : (highlighted ? TARGET_COLOR : DUMMY_COLOR));
	ShapeDrawCapsule(GetPosition(), BODY_RADIUS, BODY_HEIGHT, color);
	if (IsHuman() && highlighted) {
		const XMFLOAT3 p = GetPosition();
		ShapeDrawSphere({ p.x, p.y + BODY_HEIGHT + TARGET_MARKER_HEIGHT, p.z }, TARGET_MARKER_RADIUS, TARGET_COLOR);
	}
}

XMFLOAT3 Fighter::GetPosition() const
{
	return { m_Body.GetX(), m_Body.GetY(), m_Body.GetZ() };
}

void Fighter::SetPosition(const XMFLOAT3& position)
{
	m_Body.SetPosition({ position.x, position.y, position.z });
}

void Fighter::SetPulled(bool pulled)
{
	m_Pulled = pulled;
	if (pulled) {
		m_Body.SetVelocity({});
		m_Body.SetOnGround(false);
	}
}

float Fighter::GetHeight() const
{
	return BODY_HEIGHT;
}

CAPSULE Fighter::GetBody() const
{
	return DummyCalcBodyCapsule({ m_Body.GetX(), m_Body.GetY(), m_Body.GetZ() }, BODY_RADIUS, BODY_HEIGHT);
}

bool Fighter::Grab(int holder)
{
	if (!IsActive() || IsHeld() || holder < 0 || holder >= FIGHTER_COUNT) return false;
	m_HeldBy = holder;
	m_Launched = false;
	m_Pulled = false;
	m_Body.SetVelocity({});
	m_Body.SetOnGround(false);
	return true;
}

void Fighter::MoveHeld(int holder, const VEC3& center)
{
	if (IsHeldBy(holder)) m_Body.SetPosition({ center.X, center.Y - BODY_HEIGHT * 0.5f, center.Z });
}

bool Fighter::Release(int holder)
{
	if (!IsHeldBy(holder)) return false;
	m_HeldBy = -1;
	return true;
}

void Fighter::AddDamage(float damage)
{
	if (IsActive()) m_Percent = KnockbackCalcAddDamage(m_Percent, damage);
}

void Fighter::SetLaunchVelocity(const VEC3& velocity)
{
	m_Body.SetVelocity(velocity);
	m_Body.SetOnGround(false);
	m_HeldBy = -1;
	m_Pulled = false;
	m_Launched = true;
}

void Fighter::Hit(const VEC3& direction, float damage, float power)
{
	if (!IsActive()) return;
	AddDamage(damage);
	const KNOCKBACK_PARAMS params = KnockbackParams();
	const float speed = KnockbackCalcSpeed(m_Percent, power, params);
	SetLaunchVelocity(KnockbackCalcVelocity(direction, speed, params));
}

void Fighter::Launch(const VEC3& direction, float damage, float power)
{
	if (!IsActive()) return;
	AddDamage(damage);
	const float speed = KnockbackCalcSpeed(m_Percent, power, KnockbackParams());
	SetLaunchVelocity(KnockbackCalcDirectedVelocity(direction, speed));
}

void Fighter::Push(const VEC3& velocity)
{
	if (IsActive() && !IsHeld()) SetLaunchVelocity(velocity);
}
