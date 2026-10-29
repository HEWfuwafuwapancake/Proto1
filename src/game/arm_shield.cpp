#include "game/arm_shield.h"
#include "game/fightermanager.h"
#include "game/fighter.h"
#include "core/gametime.h"
#include "input/pad.h"
#include "physics/armcalc.h"
#include "physics/grapplecalc.h"

// 調整用の値
static constexpr float GUARD_SECONDS = 2.0f;
static constexpr float GUARD_MOVE_SCALE = 0.4f;
static constexpr float GUARD_RAISE_SECONDS = 0.1f;
static constexpr float BASH_SECONDS = 0.15f;
static constexpr float BASH_REACH = 0.3f;
static constexpr float BASH_RANGE = 2.0f;
static constexpr float BASH_RADIUS = 0.9f;
static constexpr float BASH_SPEED = 12.0f;
static constexpr float BASH_UP = 0.15f;
static constexpr float COOLDOWN_SECONDS = 0.3f;
static constexpr SHIELD_PARAMS PARAMS{
	GUARD_SECONDS, BASH_SECONDS, COOLDOWN_SECONDS, BASH_RANGE, BASH_RADIUS
};
static constexpr float MODEL_SCALE = 0.08f;
static constexpr float BASE_LENGTH = 19.250364f; // OBJ の肩から -Y 側の端までの長さ（cm）。
static constexpr float PLATE_LENGTH = 2.901904f; // 板の取付原点から -Y 側の端までの長さ（cm）。
static constexpr VEC3 PLATE_OFFSET{ 0.229075f, -15.022320f, -0.102787f }; // Z 反転済みの取付位置（cm）。
static constexpr float MODEL_ROLL = 0.0f; // 上下の向きは段階3の画面確認で決める。
// 触腕の「投げる」と同じ強さ・秒数。
static constexpr float BASH_RUMBLE_SMALL = 0.6f;
static constexpr float BASH_RUMBLE_LARGE = 0.6f;
static constexpr float BASH_RUMBLE_SECONDS = 0.15f;

ShieldArm::ShieldArm(int hand, const ARM_LINKS& links) : Arm(hand, links)
{
	m_BaseModel = LoadModel("assets/models/arm_shield_base.obj");
	m_PlateModel = LoadModel("assets/models/arm_shield_plate.obj");
	Reset();
}

void ShieldArm::Reset()
{
	m_Arm.Reset();
	m_Straighten = 0.0f;
}

void ShieldArm::Update(bool trigger)
{
	const float dt = fmaxf(0.0f, GameTimeGetDeltaTime());
	const bool bash = m_Arm.Step(trigger, GetPlayer().DidRespawn(), dt, PARAMS);
	if (bash) {
		const float yaw = GetPlayer().GetYaw();
		const VEC3 forward{ sinf(yaw), 0.0f, cosf(yaw) };
		const CAPSULE capsule = ShieldCalcBashCapsule(GetShoulder(), forward, PARAMS);
		const VEC3 velocity = ShieldCalcPushVelocity(forward, BASH_SPEED, BASH_UP);
		bool hit = false;
		for (int i = 0; i < FIGHTER_COUNT; ++i) {
			const TENTACLE_TARGET target = GetFighters().GetTarget(i, GetSlot());
			if (!target.Available || !CollisionCapsuleCapsule(capsule, target.Body).Hit) continue;
			GetFighters().Push(i, GetSlot(), velocity);
			hit = true;
		}
		if (hit) Rumble(BASH_RUMBLE_SMALL, BASH_RUMBLE_LARGE, BASH_RUMBLE_SECONDS);
	}
	const bool raised = m_Arm.GetState() == SHIELD_GUARDING || m_Arm.GetState() == SHIELD_BASHING;
	const float step = dt / GUARD_RAISE_SECONDS;
	m_Straighten = raised
		? fminf(1.0f, m_Straighten + step)
		: fmaxf(0.0f, m_Straighten - step);
}

void ShieldArm::Draw()
{
	const float yaw = GetPlayer().GetYaw();
	const VEC3 forward{ sinf(yaw), 0.0f, cosf(yaw) };
	const VEC3 direction = ArmCalcBlendDirection(GetRestDirection(), forward, m_Straighten);
	VEC3 origin = GetShoulder();
	// 押し出し中は2部品を一緒に前へ出し、モデル自体は伸縮させない。
	if (m_Arm.GetState() == SHIELD_BASHING) origin = Vec3Add(origin, Vec3Scale(forward, BASH_REACH));
	const VEC3 end = Vec3Add(origin, Vec3Scale(direction, BASE_LENGTH * MODEL_SCALE));
	const VEC3 plateStart = GrappleCalcModelPoint(PLATE_OFFSET, origin, end, MODEL_SCALE, GetHand() == 1, MODEL_ROLL);
	const VEC3 plateEnd = Vec3Add(plateStart, Vec3Scale(direction, PLATE_LENGTH * MODEL_SCALE));
	const bool raised = m_Arm.GetState() == SHIELD_GUARDING || m_Arm.GetState() == SHIELD_BASHING;
	const XMFLOAT4 color = GetColor(raised);
	DrawPiece(m_BaseModel, origin, end, BASE_LENGTH, MODEL_SCALE, color, MODEL_ROLL);
	DrawPiece(m_PlateModel, plateStart, plateEnd, PLATE_LENGTH, MODEL_SCALE, color, MODEL_ROLL);
}

bool ShieldArm::IsActive() const
{
	return m_Arm.GetState() != SHIELD_IDLE;
}

bool ShieldArm::IsHolding() const
{
	return false;
}

float ShieldArm::GetMoveScale() const
{
	return m_Arm.GetState() == SHIELD_GUARDING || m_Arm.GetState() == SHIELD_BASHING ? GUARD_MOVE_SCALE : 1.0f;
}
