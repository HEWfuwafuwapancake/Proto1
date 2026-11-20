#include "game/arm_missile.h"
#include "game/fightermanager.h"
#include "game/fighter.h"
#include "game/stage.h"
#include "core/gametime.h"
#include "physics/areacalc.h"
#include "physics/armcalc.h"
#include "render/shape.h"

// 調整用の値
// 耐久：撃つたびに SHOT_COST を「腕の耐久の持ち」の倍率で割った量を減らし、腕を撃つ回は残りを全部使う（6発で使い切る）。
// 減る量の表（physics/armcostcalc）のミサイルの行は使わない（オプションでは変えられない）。
static constexpr int SHOT_COST = 16; // 指1本ぶんの減る量（等倍）
static constexpr int EMPTY_COST = ARM_COST_MAX * 8; // 腕を撃つ回に渡す量（持ちの倍率 ×0.5〜×2 で割っても残りを全部使う）
// 時間：撃つ（10フレーム）→ 次に撃てるまで（4フレーム）。長押しは構えて待ち、離した瞬間に撃つ。
enum MISSILE_PHASE { MISSILE_AIM, MISSILE_FIRE, MISSILE_COOL, MISSILE_PHASE_COUNT };
static constexpr float PHASE_SECONDS[MISSILE_PHASE_COUNT] = { 0.0f, 10.0f / 60.0f, 4.0f / 60.0f };
static constexpr float TAP_MOVE_SCALE = 0.8f; // 単押しで撃つ間の歩く速さの倍率
// 弾：1秒にキャラの横幅10体分（10m）、1秒に180°曲がる。敵か浮島・高台に当たるか、2秒で爆発する。
static constexpr PROJECTILE_PARAMS SHOT_PARAMS{ 0.3f, 0.0f, 2.0f, XM_PI, false };
static constexpr float SHOT_SPEED = 10.0f;
static constexpr float FIRE_UP_ANGLE = XMConvertToRadians(45.0f); // 撃つ向きの上向きの角度
static constexpr float FIRE_OUT_ANGLE = XMConvertToRadians(30.0f); // 撃つ向きを腕の側へ開く角度
static constexpr float FIRE_SPREAD_ANGLE = XMConvertToRadians(8.0f); // 何発かまとめて撃つときの、1発ごとの左右のずらし
// 爆発：指は小さく、腕は中くらい。吹っ飛ばす強さは、浮島の中央から指は150%・腕は100%で場外まで飛ぶ値。
static constexpr float FINGER_BLAST_RADIUS = 0.5f;
static constexpr float FINGER_DAMAGE = 6.0f;
static constexpr float FINGER_POWER = 1.6f;
static constexpr float ARM_BLAST_RADIUS = 1.5f;
static constexpr float ARM_DAMAGE = 10.0f;
static constexpr float ARM_POWER = 2.1f;
static constexpr float BLAST_UP = 0.5f; // 爆心から外への水平の向きに足す上向きの量
// 振動（撃った瞬間）
static constexpr float TAP_RUMBLE = 0.4f;
static constexpr float TAP_RUMBLE_SECONDS = 0.1f;
static constexpr float HOLD_RUMBLE = 0.7f;
static constexpr float HOLD_RUMBLE_SECONDS = 0.2f;
// 見た目（仮の形。長さはメカの大きさで、キャラの大きさの倍率を掛ける）
static constexpr float UPPER_LENGTH = 0.32f; // 肩からひじ
static constexpr float FORE_LENGTH = 0.3f; // ひじから手首
static constexpr float ARM_WIDTH = 0.11f;
static constexpr float JOINT_RADIUS = 0.075f; // 肩・ひじ・手首の球
static constexpr float FINGER_LENGTH = 0.14f;
static constexpr float FINGER_WIDTH = 0.035f;
static constexpr float FINGER_GAP = 0.04f; // 指の並びの間隔
static constexpr float AIM_PITCH = XMConvertToRadians(30.0f); // 構えて撃つ間の腕の上向きの角度
static constexpr float STRAIGHTEN_SECONDS = 0.1f;
static constexpr float FINGER_SHOT_LENGTH = 0.35f; // 飛んでいる指の弾の長さ
static constexpr float FINGER_SHOT_WIDTH = 0.1f;
static constexpr float ARM_SHOT_LENGTH = 0.6f; // 飛んでいる腕の弾の長さ
static constexpr float ARM_SHOT_WIDTH = 0.18f;
static constexpr float FLAME_RADIUS = 0.07f; // 弾の後ろの炎
static constexpr float BLAST_SECONDS = 0.25f; // 爆発を見せる秒数
static constexpr XMFLOAT4 FINGER_SHOT_COLOR{ 0.8f, 0.8f, 0.85f, 1.0f };
static constexpr XMFLOAT4 ARM_SHOT_COLOR{ 0.45f, 0.05f, 0.15f, 1.0f };
static constexpr XMFLOAT4 FLAME_COLOR{ 1.0f, 0.6f, 0.15f, 1.0f };
static constexpr XMFLOAT4 BLAST_COLOR{ 1.0f, 0.5f, 0.1f, 0.7f };
static constexpr float SHOT_AIM_CLIP_SECONDS = 6.0f / 24.0f; // 射撃のクリップの、構え終わるまで（gunlance_shot を使い回す）
static constexpr float SHOT_RECOIL_CLIP_SECONDS = 8.0f / 24.0f; // 射撃のクリップの、反動の長さ

static XMFLOAT3 ToFloat3(const VEC3& v)
{
	return { v.X, v.Y, v.Z };
}

MissileArm::MissileArm(int hand, const ARM_LINKS& links) : Arm(hand, links)
{
	Reset();
}

void MissileArm::Reset()
{
	StopAction();
	m_FingersLeft = MISSILE_FINGER_COUNT;
	m_ArmFired = false;
	m_Straighten = 0.0f;
	ClearLeftover();
}

// 撃つ動きをやめる。腕を撃ったあとなら、次に装備したときのために指と腕を戻す。
void MissileArm::StopAction()
{
	m_Phase.Reset();
	m_Hold = false;
	if (m_ArmFired) {
		m_ArmFired = false;
		m_FingersLeft = MISSILE_FINGER_COUNT;
	}
}

void MissileArm::Interrupt()
{
	StopAction();
	StepEffects(fmaxf(0.0f, GameTimeGetDeltaTime()));
}

void MissileArm::Unequip()
{
	StopAction();
	m_FingersLeft = MISSILE_FINGER_COUNT;
	m_Straighten = 0.0f;
}

void MissileArm::UpdateLeftover()
{
	StepEffects(fmaxf(0.0f, GameTimeGetDeltaTime()));
}

void MissileArm::ClearLeftover()
{
	m_Shots.Reset();
	for (bool& isArm : m_ShotIsArm) isArm = false;
	for (BLAST& blast : m_Blasts) blast.Seconds = 0.0f;
}

// ロックオン中はその相手。いなければ（フリーカメラなど）、撃った瞬間にいちばん近い相手。
int MissileArm::PickTarget() const
{
	const int locked = GetAimTarget();
	if (locked >= 0) return locked;
	const VEC3 shoulder = GetShoulder();
	int nearest = -1;
	float nearestDistance = 0.0f;
	for (int i = 0; i < FIGHTER_COUNT; ++i) {
		const TENTACLE_TARGET target = GetFighters().GetTarget(i, GetSlot());
		if (!target.Available) continue;
		const VEC3 center = Vec3Scale(Vec3Add(target.Body.Start, target.Body.End), 0.5f);
		const float distance = Vec3Length(Vec3Sub(center, shoulder));
		if (nearest < 0 || distance < nearestDistance) {
			nearest = i;
			nearestDistance = distance;
		}
	}
	return nearest;
}

// 狙いの水平の向きから、腕の側へ開いて斜め上へ（何発かまとめて撃つときは、ここから左右へずらす）。
VEC3 MissileArm::GetFireDirection() const
{
	const float side = GetHand() == 0 ? -1.0f : 1.0f;
	return AreaCalcRotateYaw(GetAim(false), side * FIRE_OUT_ANGLE);
}

void MissileArm::FireVolley(bool all)
{
	const int pressCost = TuningCalcDurabilityCost(SHOT_COST, GetTuning());
	const MISSILE_VOLLEY volley = MissileCalcVolley(m_FingersLeft, GetDurability(), pressCost, SHOT_COST, all);
	const int count = volley.Fingers + (volley.Arm ? 1 : 0);
	const int target = PickTarget();
	const VEC3 flat = GetFireDirection();
	for (int k = 0; k < count; ++k) {
		const float spread = (static_cast<float>(k) - (count - 1) * 0.5f) * FIRE_SPREAD_ANGLE;
		const VEC3 direction = ArmCalcPitchDirection(AreaCalcRotateYaw(flat, spread), FIRE_UP_ANGLE);
		const int index = m_Shots.Fire(GetShoulder(), Vec3Scale(direction, SHOT_SPEED), target, SHOT_PARAMS);
		if (index >= 0) m_ShotIsArm[index] = volley.Arm && k == count - 1;
	}
	m_FingersLeft -= volley.Fingers;
	if (volley.Arm) {
		m_ArmFired = true;
		ConsumeDurability(EMPTY_COST);
	}
	else ConsumeDurability(SHOT_COST);
	if (all) Rumble(HOLD_RUMBLE, HOLD_RUMBLE, HOLD_RUMBLE_SECONDS);
	else Rumble(TAP_RUMBLE, TAP_RUMBLE, TAP_RUMBLE_SECONDS);
}

void MissileArm::AddBlast(const VEC3& position, float radius)
{
	// 空きがなければ、いちばん古い（残りの短い）爆発に上書きする。
	int slot = 0;
	for (int i = 1; i < BLAST_MAX; ++i) {
		if (m_Blasts[i].Seconds < m_Blasts[slot].Seconds) slot = i;
	}
	m_Blasts[slot] = { position, radius, BLAST_SECONDS };
}

// 弾を進め、敵・浮島・高台に当たった弾と寿命が来た弾をその場で爆発させる。
void MissileArm::StepEffects(float dt)
{
	for (BLAST& blast : m_Blasts) blast.Seconds -= dt;
	if (dt <= 0.0f || m_Shots.GetCount() == 0) return;
	TENTACLE_TARGET targets[FIGHTER_COUNT];
	for (int i = 0; i < FIGHTER_COUNT; ++i) targets[i] = GetFighters().GetTarget(i, GetSlot());
	PROJECTILE_EVENT events[PROJECTILE_MAX];
	const int count = m_Shots.Step(dt, targets, FIGHTER_COUNT, GetStage().GetStage(), SHOT_PARAMS, events, PROJECTILE_MAX);
	for (int e = 0; e < count; ++e) {
		const bool isArm = m_ShotIsArm[events[e].Index];
		const float radius = isArm ? ARM_BLAST_RADIUS : FINGER_BLAST_RADIUS;
		Explode(events[e].Position, radius, isArm ? ARM_DAMAGE : FINGER_DAMAGE, isArm ? ARM_POWER : FINGER_POWER, BLAST_UP);
		AddBlast(events[e].Position, radius);
		m_ShotIsArm[events[e].Index] = false;
	}
}

void MissileArm::Update(const ARM_INPUT& input)
{
	const float dt = fmaxf(0.0f, GameTimeGetDeltaTime());
	if (GetPlayer().DidRespawn()) Reset();
	// 単押しはすぐ撃つ。長押しは押している間構えて待ち、離した瞬間に残りを全部撃つ。
	if (m_Phase.IsIdle() && (input.Tap || input.HoldStarted || input.HoldReleased)) {
		m_Hold = !input.Tap;
		if (m_Hold) {
			m_Phase.Start(MISSILE_AIM);
			m_Phase.SetHeld(true);
			if (input.HoldReleased) m_Phase.Release();
		}
		else {
			m_Phase.Start(MISSILE_FIRE);
			FireVolley(false);
		}
	}
	else if (input.HoldReleased && m_Hold) m_Phase.Release();
	// 構えている間にボタンを受け付けなくなった（ガードなど）ときは、撃たずにやめる。
	else if (!input.Pressed && m_Phase.IsHeld()) StopAction();
	const ARM_PHASE_STEP step = m_Phase.Step(dt, PHASE_SECONDS, MISSILE_PHASE_COUNT);
	if (step.Entered == MISSILE_FIRE && m_Hold) FireVolley(true);
	if (step.Finished) StopAction();
	StepEffects(dt);
	const float speed = dt / STRAIGHTEN_SECONDS;
	m_Straighten = m_Phase.IsIdle() ? fmaxf(0.0f, m_Straighten - speed) : fminf(1.0f, m_Straighten + speed);
}

void MissileArm::Draw()
{
	const float scale = GetDrawScale();
	const VEC3 shoulder = GetDrawShoulder();
	const XMFLOAT4 color = GetColor(IsActive());
	ShapeDrawSphere(ToFloat3(shoulder), JOINT_RADIUS * scale, color);
	if (m_ArmFired) return; // 腕を撃ったあとは肩だけ残る
	VEC3 armDirection = ArmCalcBlendDirection(GetRestDirection(), ArmCalcPitchDirection(GetFireDirection(), AIM_PITCH), m_Straighten);
	VEC3 unusedWeapon = armDirection;
	BlendClipPose(armDirection, unusedWeapon);
	const VEC3 elbow = Vec3Add(shoulder, Vec3Scale(armDirection, UPPER_LENGTH * scale));
	const VEC3 wrist = Vec3Add(elbow, Vec3Scale(armDirection, FORE_LENGTH * scale));
	ShapeDrawBoxAlong(ToFloat3(shoulder), ToFloat3(elbow), ARM_WIDTH * scale, ARM_WIDTH * scale, color);
	ShapeDrawBoxAlong(ToFloat3(elbow), ToFloat3(wrist), ARM_WIDTH * scale, ARM_WIDTH * scale, color);
	ShapeDrawSphere(ToFloat3(elbow), JOINT_RADIUS * scale, color);
	ShapeDrawSphere(ToFloat3(wrist), JOINT_RADIUS * scale, color);
	// 指は手首の先に横に並べる（撃った指は描かない。小指から撃つ）。
	VEC3 side{ armDirection.Z, 0.0f, -armDirection.X };
	const float sideLength = Vec3Length(side);
	side = sideLength > 0.001f ? Vec3Scale(side, 1.0f / sideLength) : GetForward();
	const float handSide = GetHand() == 0 ? 1.0f : -1.0f; // 小指を体の外側に置く
	for (int k = MISSILE_FINGER_COUNT - m_FingersLeft; k < MISSILE_FINGER_COUNT; ++k) {
		const VEC3 base = Vec3Add(wrist, Vec3Scale(side, (k - 2) * FINGER_GAP * handSide * scale));
		const VEC3 tip = Vec3Add(base, Vec3Scale(armDirection, FINGER_LENGTH * scale));
		ShapeDrawBoxAlong(ToFloat3(base), ToFloat3(tip), FINGER_WIDTH * scale, FINGER_WIDTH * scale, color);
	}
}

void MissileArm::DrawLeftover()
{
	for (int i = 0; i < PROJECTILE_MAX; ++i) {
		const PROJECTILE& shot = m_Shots.Get(i);
		if (!shot.Active) continue;
		const float speed = Vec3Length(shot.Velocity);
		const VEC3 direction = speed > 0.001f ? Vec3Scale(shot.Velocity, 1.0f / speed) : VEC3{ 0.0f, 1.0f, 0.0f };
		const bool isArm = m_ShotIsArm[i];
		const float half = (isArm ? ARM_SHOT_LENGTH : FINGER_SHOT_LENGTH) * 0.5f;
		const float width = isArm ? ARM_SHOT_WIDTH : FINGER_SHOT_WIDTH;
		const VEC3 tail = Vec3Sub(shot.Position, Vec3Scale(direction, half));
		const VEC3 head = Vec3Add(shot.Position, Vec3Scale(direction, half));
		ShapeDrawBoxAlong(ToFloat3(tail), ToFloat3(head), width, width, isArm ? ARM_SHOT_COLOR : FINGER_SHOT_COLOR);
		ShapeDrawSphere(ToFloat3(tail), FLAME_RADIUS * (isArm ? 1.6f : 1.0f), FLAME_COLOR);
	}
	for (const BLAST& blast : m_Blasts) {
		if (blast.Seconds <= 0.0f) continue;
		// 小さく出て広がりながら消える。
		const float rate = 1.0f - blast.Seconds / BLAST_SECONDS;
		XMFLOAT4 color = BLAST_COLOR;
		color.w *= 1.0f - rate * 0.6f;
		ShapeDrawSphere(ToFloat3(blast.Position), blast.Radius * (0.4f + 0.6f * rate), color);
	}
}

bool MissileArm::IsActive() const
{
	return !m_Phase.IsIdle();
}

bool MissileArm::IsHolding() const
{
	return false;
}

float MissileArm::GetMoveScale() const
{
	if (m_Phase.IsIdle()) return 1.0f;
	if (m_Hold) return 0.0f;
	return m_Phase.GetPhase() == MISSILE_FIRE ? TAP_MOVE_SCALE : 1.0f;
}

bool MissileArm::BlocksJump() const
{
	return m_Hold && !m_Phase.IsIdle();
}

bool MissileArm::BlocksDash() const
{
	return m_Hold && !m_Phase.IsIdle();
}

ARM_BODY_CLIP MissileArm::GetBodyClip() const
{
	ARM_BODY_CLIP clip;
	if (m_Phase.IsIdle()) return clip;
	clip.Active = true;
	clip.Kind = ATTACK_CLIP::GunlanceShot;
	if (m_Phase.GetPhase() == MISSILE_AIM) {
		clip.Time = SHOT_AIM_CLIP_SECONDS;
		return clip;
	}
	// 撃ってから次に撃てるまでに、反動を1回見せる。
	const float total = PHASE_SECONDS[MISSILE_FIRE] + PHASE_SECONDS[MISSILE_COOL];
	const float elapsed = m_Phase.GetTime() + (m_Phase.GetPhase() == MISSILE_COOL ? PHASE_SECONDS[MISSILE_FIRE] : 0.0f);
	clip.Time = SHOT_AIM_CLIP_SECONDS + SHOT_RECOIL_CLIP_SECONDS * fminf(1.0f, elapsed / total);
	return clip;
}
