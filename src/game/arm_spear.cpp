#include "game/arm_spear.h"
#include "audio/se.h"
#include "game/fightermanager.h"
#include "game/fighter.h"
#include "game/stage.h"
#include "core/gametime.h"
#include "core/mathutil.h"
#include "physics/areacalc.h"
#include "physics/armcalc.h"
#include "physics/grapplecalc.h"
#include "physics/lasercalc.h"
#include "physics/statuscalc.h"
#include "render/shape.h"

// 調整用の値
// 耐久の減る量は physics/armcostcalc の表の槍の行（弱：突き、強：投げた槍が刺さった・刺さらなかった）。
static constexpr ARM_COST_ROW COST_ROW = ARM_COST_ROW_SPEAR;
// 単押し：中距離の直線上を突く（吹っ飛びは刀の斬撃 1.6 以上・ナックルの殴り 2.0 以下）
static constexpr float THRUST_SECONDS[] = { 0.12f, 0.1f, 0.2f }; // 構え・突き・休み
static constexpr float THRUST_REACH = 3.2f; // 肩から突きの届く距離（ナックルの殴りは 1.6）。
static constexpr float THRUST_RADIUS = 0.4f;
// 穂先（体の手前の面が肩から 2.2m より先）で当てると強く、手元だと弱い（No.178）。
static constexpr SPEAR_THRUST_PARAMS THRUST{ 2.2f, 12.0f, 2.4f, 7.0f, 1.4f }; // 穂先の始まる距離・穂先のダメージと強さ・手元のダメージと強さ
static constexpr float LUNGE_SPEED = 15.0f; // 突く前の構えの間、前へ踏み込む速さ（構え 0.12 秒で約 1.8m）。
static constexpr float THRUST_UP = 0.6f; // 水平の単位方向に足す上向きの量。
// 長押し：構えて、離すと槍を直線上に投げる（吹っ飛びは刀の斬撃と同じ）
static constexpr float THROW_SECONDS[] = { 0.25f, 0.1f, 0.2f }; // 構え（押している間は待つ）・投げ・休み
static constexpr SPEAR_THROW_PARAMS THROW{ 45.0f, 25.0f, 0.4f }; // 速さ・届く距離・太さ
static constexpr float THROW_DAMAGE = 8.0f;
static constexpr float THROW_POWER = 1.6f; // 刀の斬撃（arm_katana.cpp の TAP_POWER）と同じ。
static constexpr float THROW_UP = 0.6f;
static constexpr float THROW_MOVE_SCALE = 0.6f; // 投げる構えの間の歩く速さの倍率。
// 刺さった相手へ突っ込む
static constexpr float STUN_TICK_SECONDS = 0.1f; // 吹っ飛んでいる間、毎フレームかけ直すしびれの秒数（着地するとこの秒数で解ける）。
static constexpr float RUSH_SPEED = 40.0f; // 突っ込む速さ。
static constexpr float RUSH_ARRIVE_DISTANCE = 1.2f; // 相手の足元にこれより近づいたら着いたことにする。
static constexpr float RUSH_MAX_SECONDS = 1.2f; // 刺さってからこの秒数で、着かなくても突っ込むのをやめる。
// 描画（腕は arm_shield_base、槍は形の部品）
static constexpr float STRAIGHTEN_SECONDS = 0.1f;
static constexpr float EXTEND_SECONDS = 0.05f;
static constexpr float RAISE_PITCH = XMConvertToRadians(-25.0f); // 投げる構えの間の腕の上下の角度（上へ）。
static constexpr float CARRY_PITCH = XMConvertToRadians(-10.0f); // 待機のときの槍の上下の向き（穂先を少し上へ）。
static constexpr float MODEL_SCALE = 0.08f * Arm::MODEL_SIZE;
static constexpr float ARM_LENGTH = 19.250364f; // arm_shield_base の長さ（cm）。
static constexpr float SPEAR_FRONT = 1.3f; // 手から穂先までの長さ。
static constexpr float SPEAR_BACK = 0.5f; // 手から石突きまでの長さ。
static constexpr float SHAFT_RADIUS = 0.03f;
static constexpr float HEAD_LENGTH = 0.3f; // 穂先の長さ。
static constexpr float HEAD_RADIUS = 0.06f;
static constexpr float STUCK_DEPTH = 0.2f; // 刺さった相手の体の中心から穂先までの奥への深さ。
static constexpr XMFLOAT4 SHAFT_COLOR{ 0.55f, 0.4f, 0.25f, 1.0f };
static constexpr XMFLOAT4 HEAD_COLOR{ 0.85f, 0.88f, 0.95f, 1.0f };
static constexpr float PUNCH_CLIP_SECONDS[] = { 4.0f / 24.0f, 3.0f / 24.0f, 5.0f / 24.0f }; // 突き・投げのクリップの区切り（knuckle_punch を使い回す）
static constexpr float RUSH_CLIP_TIME = 14.0f / 24.0f; // 突っ込む間に止めて見せる、knuckle_rush の突進の形の時刻。
static constexpr float HIT_RUMBLE = 1.0f;
static constexpr float HIT_RUMBLE_SECONDS = 0.25f;
static constexpr float THROW_RUMBLE = 0.4f;
static constexpr float THROW_RUMBLE_SECONDS = 0.1f;
static constexpr float THRUST_SE_VOLUME = 0.6f; // 突きで鳴らす「槍投げ」の音の大きさ（投げは 1.0）
static constexpr SE_ID STAB_SE = SE_KATANA_HIT2; // 刺さった音（No.195 で「刀単押しヒット2」に替えて比べる。元は SE_SPEAR_STAB）

enum SPEAR_PHASE { SPEAR_RAISE, SPEAR_STRIKE, SPEAR_RECOVER, SPEAR_PHASE_COUNT };

SpearArm::SpearArm(int hand, const ARM_LINKS& links) : Arm(hand, links)
{
	m_ArmModel = LoadModel("assets/models/arm_shield_base.obj");
	Reset();
}

void SpearArm::Reset()
{
	// 動作の途中でやめたら外れとして1回だけ減らす。
	if ((!m_Phase.IsIdle() || m_Spear.IsFlying()) && !m_UseConsumed) Consume(m_Throw ? ARM_COST_HOLD_MISS : ARM_COST_TAP_MISS);
	m_UseConsumed = true;
	EndRush();
	m_Phase.Reset();
	m_Throw = false;
	m_Lunging = false;
	m_Straighten = 0.0f;
	m_Extend = 0.0f;
}

// 1動作で1回だけ耐久を減らす。
void SpearArm::Consume(ARM_COST_ITEM item)
{
	if (m_UseConsumed) return;
	ConsumeDurability(GetCost(COST_ROW, item));
	m_UseConsumed = true;
}

// 肩から狙いの向きへ、届く距離の直線上にいる相手を全部突く。
void SpearArm::Thrust()
{
	TENTACLE_TARGET targets[FIGHTER_COUNT];
	for (int i = 0; i < FIGHTER_COUNT; ++i) targets[i] = GetFighters().GetMeleeTarget(i, GetSlot());
	bool hits[FIGHTER_COUNT];
	const VEC3 shoulder = GetShoulder();
	const VEC3 direction = GetStrikeDirection(m_Direction);
	StrikePillarsOnLine({ shoulder, Vec3Add(shoulder, Vec3Scale(direction, THRUST_REACH)), THRUST_RADIUS }, THRUST.TipDamage, m_Direction);
	LaserCalcPierce(shoulder, direction, THRUST_REACH, THRUST_RADIUS, targets, FIGHTER_COUNT, hits);
	bool hit = false;
	int hitTarget = -1; // 音を鳴らす相手（何人に当たっても音は1回）
	for (int i = 0; i < FIGHTER_COUNT; ++i) {
		if (!hits[i]) continue;
		const SPEAR_THRUST_HIT strike = SpearCalcThrustHit(shoulder, direction, targets[i].Body, THRUST);
		if (GetFighters().Launch(i, GetSlot(), AreaCalcLaunchDirection(m_Direction, THRUST_UP), strike.Damage, strike.Power)) {
			hit = true;
			hitTarget = i;
		}
	}
	if (hit) {
		Consume(ARM_COST_TAP_HIT);
		Rumble(HIT_RUMBLE, HIT_RUMBLE, HIT_RUMBLE_SECONDS);
		PlayHitSe(STAB_SE, hitTarget);
	}
}

// 突く前の構えの間：狙いの方へ踏み込み、前の相手が穂先の距離まで近づいたら止まる（No.178）。
void SpearArm::StepLunge()
{
	if (!m_Lunging) return;
	if (m_Throw || m_Phase.GetPhase() != SPEAR_RAISE) { m_Lunging = false; return; }
	// ロックオン中は動いた相手の方へ向き直しながら踏み込む。
	m_Direction = GetAim(false);
	TENTACLE_TARGET targets[FIGHTER_COUNT];
	for (int i = 0; i < FIGHTER_COUNT; ++i) targets[i] = GetFighters().GetMeleeTarget(i, GetSlot());
	if (SpearCalcShouldStopLunge(GetShoulder(), GetStrikeDirection(m_Direction), THRUST, THRUST_RADIUS, targets, FIGHTER_COUNT)) m_Lunging = false;
}

// 刺さった瞬間：相手を吹っ飛ばしてしびれさせ、相手が浮島の上にいれば突っ込み始める。
void SpearArm::OnStuck()
{
	const int target = m_Spear.GetStuckTarget();
	if (!GetFighters().Launch(target, GetSlot(), AreaCalcLaunchDirection(m_Spear.GetDirection(), THROW_UP), THROW_DAMAGE, THROW_POWER)) {
		// ガードではじかれたら外れとして槍を手に戻す。
		Consume(ARM_COST_HOLD_MISS);
		m_Spear.Reset();
		return;
	}
	Consume(ARM_COST_HOLD_HIT);
	GetFighters().ApplyStatus(target, GetSlot(), STATUS_STUN, STUN_TICK_SECONDS, 0.0f);
	GetFighters().SpawnEffect(EFFECT_DUST_KICK, GetFighters().GetBodyPoint(target, 0.5f), Vec3Scale(m_Spear.GetDirection(), -1.0f));
	Rumble(HIT_RUMBLE, HIT_RUMBLE, HIT_RUMBLE_SECONDS);
	PlayHitSe(STAB_SE, target);
	m_StuckSeconds = 0.0f;
	// もう一方の手や触手が体を動かしている間は突っ込まない（吹っ飛ばしとしびれは入る）。
	if (GetPlayer().IsPulled()) return;
	m_Rushing = true;
	m_OwnPull = true;
	GetPlayer().SetPulled(true);
	PlaySe(SE_SPEAR_PULL);
}

// 刺さっている間：吹っ飛んでいる相手をしびれさせ続け、自分は相手の足元へ突っ込む。
void SpearArm::StepRush(float dt)
{
	const int target = m_Spear.GetStuckTarget();
	m_StuckSeconds += dt;
	if (!GetFighters().IsActive(target)) { EndRush(); return; }
	const Fighter& victim = GetFighters().GetFighter(target);
	const XMFLOAT3 p = victim.GetPosition();
	const VEC3 targetFeet{ p.x, p.y, p.z };
	// 相手が浮島から落ちたら突っ込まない（突っ込んでいる途中でもやめる）。
	if (SpearCalcHasFallen(GetStage().GetStage(), targetFeet) || m_StuckSeconds >= RUSH_MAX_SECONDS) { EndRush(); return; }
	if (victim.IsLaunched()) GetFighters().ApplyStatus(target, GetSlot(), STATUS_STUN, STUN_TICK_SECONDS, 0.0f);
	if (!m_Rushing) {
		// 突っ込めないときは、相手が着地したら槍を戻す。
		if (!victim.IsLaunched()) EndRush();
		return;
	}
	GetPlayer().RequestGhosts(); // 刺さった所へ飛ぶ間は体の後ろに残像（見た目だけ）。
	const XMFLOAT3 own = GetPlayer().GetPosition();
	VEC3 feet{ own.x, own.y, own.z };
	const bool arrived = GrappleCalcPull(feet, targetFeet, RUSH_SPEED, RUSH_ARRIVE_DISTANCE, dt);
	GetPlayer().SetPosition({ feet.X, feet.Y, feet.Z });
	if (arrived) EndRush();
}

// 突っ込むのをやめ、止めていた体の計算を戻して、槍を手に戻す。
void SpearArm::EndRush()
{
	if (m_OwnPull) GetPlayer().SetPulled(false);
	m_OwnPull = false;
	m_Rushing = false;
	m_Spear.Reset();
}

void SpearArm::StepSpear(float dt)
{
	if (m_Spear.IsFlying()) {
		TENTACLE_TARGET targets[FIGHTER_COUNT];
		for (int i = 0; i < FIGHTER_COUNT; ++i) targets[i] = GetFighters().GetTarget(i, GetSlot());
		const VEC3 from = m_Spear.GetTip();
		// 飛んでいる槍の後ろに細い軌跡（見た目だけ）。
		GetFighters().SpawnEffect(EFFECT_TRAIL, from, { 0.0f, 1.0f, 0.0f });
		const SPEAR_THROW_EVENT event = m_Spear.Step(dt, THROW, targets, FIGHTER_COUNT);
		StrikePillarsOnLine({ from, m_Spear.GetTip(), THROW.Radius }, THROW_DAMAGE, m_Spear.GetDirection());
		if (event == SPEAR_THROW_STUCK) OnStuck();
		else if (event == SPEAR_THROW_MISSED) Consume(ARM_COST_HOLD_MISS);
	}
	else if (m_Spear.IsStuck()) StepRush(dt);
}

void SpearArm::Update(const ARM_INPUT& input)
{
	const float dt = fmaxf(0.0f, GameTimeGetDeltaTime());
	if (GetPlayer().DidRespawn()) Reset();
	// 長押しは押している間構えて待ち、離した瞬間に投げる。
	if (!IsActive() && (input.Tap || input.HoldStarted || input.HoldReleased)) {
		m_Throw = !input.Tap;
		m_UseConsumed = false;
		m_Direction = GetAim(false);
		m_Phase.Start(SPEAR_RAISE);
		m_Phase.SetHeld(m_Throw);
		m_Lunging = !m_Throw;
		if (input.HoldReleased) m_Phase.Release();
		else if (m_Throw) PlaySe(SE_ARM_HOLD);
	}
	else if (input.HoldReleased && m_Throw) m_Phase.Release();
	// 構えている間にボタンを受け付けなくなった（ガードなど）ときは、投げずにやめる。
	else if (!input.Pressed && m_Phase.IsHeld()) {
		m_Phase.Reset();
		m_UseConsumed = true;
	}
	const ARM_PHASE_STEP step = m_Phase.Step(dt, m_Throw ? THROW_SECONDS : THRUST_SECONDS, SPEAR_PHASE_COUNT);
	StepLunge();
	if (step.Entered == SPEAR_STRIKE) {
		m_Direction = GetAim(false);
		PlaySe(SE_SPEAR_THROW, m_Throw ? 1.0f : THRUST_SE_VOLUME); // 突きも投げと同じ風を切る音（小さめ）
		if (!m_Throw) Thrust();
		else {
			m_Spear.Throw(GetShoulder(), GetStrikeDirection(m_Direction));
			Rumble(THROW_RUMBLE, 0.0f, THROW_RUMBLE_SECONDS);
		}
	}
	StepSpear(dt);
	if (step.Finished && !m_Throw) Consume(ARM_COST_TAP_MISS);
	const float speed = dt / STRAIGHTEN_SECONDS;
	m_Straighten = m_Phase.IsIdle() ? fmaxf(0.0f, m_Straighten - speed) : fminf(1.0f, m_Straighten + speed);
	const float extendSpeed = dt / EXTEND_SECONDS;
	m_Extend = (!m_Throw && m_Phase.GetPhase() == SPEAR_STRIKE) ? fminf(1.0f, m_Extend + extendSpeed) : fmaxf(0.0f, m_Extend - extendSpeed);
}

// 刺さっている穂先の位置（相手の体の中心の少し奥）。
VEC3 SpearArm::GetStuckTip() const
{
	const CAPSULE body = GetFighters().GetFighter(m_Spear.GetStuckTarget()).GetBody();
	const VEC3 center = Vec3Scale(Vec3Add(body.Start, body.End), 0.5f);
	return Vec3Add(center, Vec3Scale(m_Spear.GetDirection(), STUCK_DEPTH));
}

void SpearArm::Draw()
{
	const bool raising = m_Throw && m_Phase.GetPhase() == SPEAR_RAISE;
	const VEC3 aim = raising ? ArmCalcPitchDirection(m_Direction, RAISE_PITCH) : m_Direction;
	VEC3 direction = ArmCalcBlendDirection(GetRestDirection(), aim, m_Straighten);
	VEC3 weapon = ArmCalcBlendDirection(ArmCalcPitchDirection(GetForward(), CARRY_PITCH), m_Direction, m_Straighten);
	BlendClipPose(direction, weapon);
	const XMFLOAT4 color = GetColor(IsActive());
	const float drawScale = GetDrawScale();
	const float scale = MODEL_SCALE * drawScale;
	const VEC3 shoulder = GetDrawShoulder();
	const VEC3 hand = Vec3Add(shoulder, Vec3Scale(direction, ARM_LENGTH * scale));
	if (m_ArmModel >= 0) DrawPiece(m_ArmModel, shoulder, hand, ARM_LENGTH, scale, color);
	// 槍は手の中か、飛んでいる・刺さっている所に描く（穂先から石突きへ）。
	VEC3 tip;
	VEC3 along;
	if (m_Spear.IsFlying() || m_Spear.IsStuck()) {
		tip = m_Spear.IsStuck() ? GetStuckTip() : m_Spear.GetTip();
		along = m_Spear.GetDirection();
	}
	else {
		// 突く間は届く距離まで槍を前へ出す。
		const float front = Lerp(SPEAR_FRONT * drawScale, fmaxf(SPEAR_FRONT * drawScale, THRUST_REACH - ARM_LENGTH * scale), m_Extend);
		tip = Vec3Add(hand, Vec3Scale(weapon, front));
		along = weapon;
	}
	const float total = (SPEAR_FRONT + SPEAR_BACK) * drawScale;
	const VEC3 headBase = Vec3Sub(tip, Vec3Scale(along, HEAD_LENGTH * drawScale));
	const VEC3 butt = Vec3Sub(tip, Vec3Scale(along, total));
	ShapeDrawCylinder({ butt.X, butt.Y, butt.Z }, { headBase.X, headBase.Y, headBase.Z }, SHAFT_RADIUS * drawScale, SHAFT_COLOR);
	ShapeDrawCylinder({ headBase.X, headBase.Y, headBase.Z }, { tip.X, tip.Y, tip.Z }, HEAD_RADIUS * drawScale, HEAD_COLOR);
}

bool SpearArm::IsActive() const
{
	return !m_Phase.IsIdle() || m_Spear.IsFlying() || m_Spear.IsStuck();
}

bool SpearArm::IsHolding() const
{
	return false;
}

ARM_BODY_CLIP SpearArm::GetBodyClip() const
{
	ARM_BODY_CLIP clip;
	if (m_Rushing) {
		clip.Active = true;
		clip.Kind = ATTACK_CLIP::KnuckleRush;
		clip.FullBody = true;
		clip.Time = RUSH_CLIP_TIME;
		return clip;
	}
	if (m_Phase.IsIdle()) return clip;
	clip.Active = true;
	clip.Kind = ATTACK_CLIP::KnucklePunch;
	clip.Time = ArmCalcClipTime(m_Phase.GetPhase(), m_Phase.GetTime(), m_Throw ? THROW_SECONDS : THRUST_SECONDS, PUNCH_CLIP_SECONDS);
	return clip;
}

float SpearArm::GetMoveScale() const
{
	return m_Throw && m_Phase.GetPhase() == SPEAR_RAISE ? THROW_MOVE_SCALE : 1.0f;
}

bool SpearArm::BlocksJump() const
{
	return m_Rushing;
}

bool SpearArm::BlocksDash() const
{
	return m_Rushing;
}

VEC3 SpearArm::GetRushVelocity() const
{
	if (!m_Lunging) return {};
	return Vec3Scale(m_Direction, LUNGE_SPEED);
}
