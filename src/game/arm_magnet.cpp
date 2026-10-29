#include "game/arm_magnet.h"
#include "game/camera.h"
#include "game/fightermanager.h"
#include "game/fighter.h"
#include "core/gametime.h"
#include "input/pad.h"
#include "physics/armcalc.h"

// 調整用の値
static constexpr float RANGE = 10.0f;
static constexpr float BEAM_RADIUS = 0.8f;
static constexpr float PULL_SPEED = 15.0f;
static constexpr float ARRIVAL_DISTANCE = 0.1f;
static constexpr float HOLD_FORWARD = 2.0f;
static constexpr float HOLD_SECONDS = 1.5f;
static constexpr float COOLDOWN_SECONDS = 0.3f;
static constexpr MAGNET_PARAMS PARAMS{
	RANGE, BEAM_RADIUS, PULL_SPEED, ARRIVAL_DISTANCE, HOLD_SECONDS, COOLDOWN_SECONDS
};
static constexpr float REPEL_DAMAGE = 8.0f;
static constexpr float REPEL_POWER = 2.5f;
static constexpr float MAGNET_AIM_SECONDS = 0.1f;
static constexpr float MODEL_SCALE = 0.08f * Arm::MODEL_SIZE;
static constexpr VEC3 MOUNT_OFFSET{ 0.0f, -0.05f, 0.0f }; // 取付位置のずらし（外・上・前 m）。少し下げる
static constexpr float MODEL_LENGTH = 17.424655f; // OBJ の肩から -Y 側の端までの長さ（cm）。
// OBJ は読み込みで Z が反転するので、半回転して磁石の先を腕の線より上に出す（ドリルの付け根と同じ）。
static constexpr float MODEL_ROLL = XM_PI;
// 触腕の「つかむ」「投げる」と同じ強さ・秒数。
static constexpr float GRAB_RUMBLE_SMALL = 0.4f;
static constexpr float GRAB_RUMBLE_LARGE = 0.0f;
static constexpr float GRAB_RUMBLE_SECONDS = 0.1f;
static constexpr float REPEL_RUMBLE_SMALL = 0.6f;
static constexpr float REPEL_RUMBLE_LARGE = 0.6f;
static constexpr float REPEL_RUMBLE_SECONDS = 0.15f;

MagnetArm::MagnetArm(int hand, const ARM_LINKS& links) : Arm(hand, links)
{
	m_Model = LoadModel("assets/models/arm_magnet.obj");
	Reset();
}

MagnetArm::~MagnetArm()
{
	Reset();
}

void MagnetArm::Reset()
{
	GetFighters().Release(m_Arm.GetTarget(), GetSlot(), false, {}, 0.0f, 0.0f);
	m_Arm.Reset();
	m_Straighten = 0.0f;
}

void MagnetArm::Update(bool trigger)
{
	const VEC3 origin = GetShoulder();
	const float cameraYaw = GetCamera().GetYaw();
	const VEC3 cameraForward{ sinf(cameraYaw), 0.0f, cosf(cameraYaw) };
	const float playerYaw = GetPlayer().GetYaw();
	const VEC3 hold = Vec3Add(origin, { sinf(playerYaw) * HOLD_FORWARD, 0.0f, cosf(playerYaw) * HOLD_FORWARD });
	// 各腕の更新直前に候補を読み、もう一方が保持している敵を選ばない。
	TENTACLE_TARGET targets[FIGHTER_COUNT];
	for (int i = 0; i < FIGHTER_COUNT; ++i) targets[i] = GetFighters().GetTarget(i, GetSlot());
	const int target = GetCamera().GetTarget();
	const bool hasTarget = target >= 0 && target < FIGHTER_COUNT;
	const VEC3 center = hasTarget
		? Vec3Scale(Vec3Add(targets[target].Body.Start, targets[target].Body.End), 0.5f) : VEC3{};
	const VEC3 aim = TentacleCalcAim(origin, center, hasTarget, cameraForward);
	// 自分が保持している敵も Available は false。保持の所有者が自分かで解除を調べる。
	const bool cancel = GetPlayer().DidRespawn()
		|| (m_Arm.GetTarget() >= 0 && !GetFighters().IsHeldBy(m_Arm.GetTarget(), GetSlot()));
	const float dt = fmaxf(0.0f, GameTimeGetDeltaTime());
	const MAGNET_EVENT event = m_Arm.Step(origin, aim, hold, trigger, cancel, dt, PARAMS, targets, FIGHTER_COUNT);
	if (event.Grabbed >= 0) {
		if (GetFighters().Grab(event.Grabbed, GetSlot())) Rumble(GRAB_RUMBLE_SMALL, GRAB_RUMBLE_LARGE, GRAB_RUMBLE_SECONDS);
		else m_Arm.Reset();
	}
	if (event.Released >= 0) {
		// 時間切れのフレームも、更新した保持位置を反映してから離す。
		GetFighters().MoveHeld(event.Released, GetSlot(), m_Arm.GetCenter());
		GetFighters().Release(event.Released, GetSlot(), event.Repel, cameraForward, REPEL_DAMAGE, REPEL_POWER);
		if (event.Repel) Rumble(REPEL_RUMBLE_SMALL, REPEL_RUMBLE_LARGE, REPEL_RUMBLE_SECONDS);
	}
	if (m_Arm.GetTarget() >= 0) GetFighters().MoveHeld(m_Arm.GetTarget(), GetSlot(), m_Arm.GetCenter());
	const float step = dt / MAGNET_AIM_SECONDS;
	m_Straighten = IsHolding()
		? fminf(1.0f, m_Straighten + step)
		: fmaxf(0.0f, m_Straighten - step);
}

void MagnetArm::Draw()
{
	const VEC3 origin = GetShoulder();
	const float yaw = GetCamera().GetYaw();
	const VEC3 aim = TentacleCalcAim(origin, m_Arm.GetCenter(), true, { sinf(yaw), 0.0f, cosf(yaw) });
	const VEC3 direction = ArmCalcBlendDirection(GetRestDirection(), aim, m_Straighten);
	const VEC3 end = Vec3Add(origin, Vec3Scale(direction, MODEL_LENGTH * MODEL_SCALE));
	DrawPiece(m_Model, origin, end, MODEL_LENGTH, MODEL_SCALE, GetColor(IsHolding()), MODEL_ROLL);
}

bool MagnetArm::IsActive() const
{
	return m_Arm.GetState() != MAGNET_IDLE;
}

VEC3 MagnetArm::GetMountOffset() const
{
	return MOUNT_OFFSET;
}

bool MagnetArm::IsHolding() const
{
	return m_Arm.GetState() == MAGNET_PULLING || m_Arm.GetState() == MAGNET_HOLDING;
}
