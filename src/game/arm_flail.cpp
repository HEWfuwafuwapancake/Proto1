#include "game/arm_flail.h"
#include "game/fightermanager.h"
#include "game/fighter.h"
#include "game/camera.h"
#include "core/gametime.h"
#include "physics/areacalc.h"
#include "physics/armcalc.h"
#include "render/shape.h"

// 調整用の値（ダメージは低く、吹っ飛ばしは強く）
// 耐久の減る量は physics/armcostcalc の表のモーニングスターの行（弱：扇、強：投げ）。回している間は当たりに関係なく1秒ごとに SPIN_DRAIN_COST 減る。
static constexpr ARM_COST_ROW COST_ROW = ARM_COST_ROW_FLAIL;
static constexpr float FAN_SECONDS[] = { 0.15f, 0.1f, 0.25f }; // 単押し：振りかぶり・振り・休み
static constexpr float FAN_CLIP_SECONDS[] = { 4.0f / 24.0f, 3.0f / 24.0f, 5.0f / 24.0f }; // ナックルの殴りのクリップの区切り
static constexpr float FAN_RADIUS = 3.0f; // 扇の半径（肩から）
static constexpr float FAN_HALF_ANGLE = XMConvertToRadians(45.0f); // 扇の左右それぞれの開き
static constexpr float FAN_HALF_HEIGHT = 1.2f;
static constexpr float HEAVY_SHAKE = 0.5f; // 鉄球が重く当たったときの画面の揺れ（当てた人・当てられた人）
static constexpr float FAN_DAMAGE = 5.0f;
static constexpr float FAN_POWER = 2.6f;
static constexpr float FAN_UP = 0.5f;
static constexpr float SPIN_FULL_SECONDS = 1.5f; // 最大までためる秒数
static constexpr float SPIN_SPEED = XM_2PI * 2.0f; // 回す速さ（1秒に2周）
static constexpr float SPIN_RADIUS = 2.2f; // 頭の上の輪の半径（この内側の全方向に当たる）
static constexpr float SPIN_HEIGHT = 1.6f; // 輪の足元からの高さ（頭の高さ）
static constexpr float SPIN_MOVE_SCALE = 0.5f; // 回している間の歩く速さの倍率
static constexpr float SPIN_DAMAGE = 2.0f;
static constexpr float SPIN_POWER = 1.0f;
static constexpr float SPIN_UP = 0.5f;
static constexpr float SPIN_HIT_COOLDOWN = 0.5f; // 同じ相手にまた当たるまでの秒数
static constexpr int SPIN_DRAIN_COST = 5; // 回している間、1秒ごとに減る耐久（持ちの倍率で割る）
static constexpr float THROW_RANGE_MIN = 6.0f; // ためなしの飛距離
static constexpr float THROW_RANGE_MAX = 14.0f; // 最大までためた飛距離
static constexpr float OUT_POWER_MIN = 2.6f; // 行きの強さ（ためなし）
static constexpr float OUT_POWER_MAX = 3.6f; // 行きの強さ（最大）
static constexpr float OUT_DAMAGE = 5.0f;
static constexpr float OUT_UP = 0.45f;
static constexpr float BACK_DAMAGE = 3.0f; // 帰りは自分の方へ引き寄せる
static constexpr float BACK_POWER = 1.6f;
static constexpr float BACK_UP = 0.3f;
static constexpr FLAIL_THROW_PARAMS THROW_PARAMS{ 28.0f, 24.0f, 0.4f }; // 行き・帰りの速さ・受け止める距離
static constexpr float THROW_PITCH_MAX = XMConvertToRadians(50.0f); // フリーカメラで上へ投げる角度の上限
static constexpr float BALL_RADIUS = 0.4f; // 当たり判定の半径
static constexpr float THROW_CLIP_SECONDS = 0.25f; // 投げた瞬間に見せる殴りのクリップの秒数（描画だけ）
// 見た目
static constexpr float BALL_DRAW_RADIUS = 0.3f;
static constexpr int BALL_SPIKE_COUNT = 8;
static constexpr float BALL_SPIKE_LENGTH = 0.22f;
static constexpr float BALL_SPIKE_RADIUS = 0.05f;
static constexpr float CHAIN_LINK_GAP = 0.18f; // 鎖の輪の間隔
static constexpr float CHAIN_LINK_RADIUS = 0.05f;
static constexpr float HANG_LENGTH = 0.55f; // 待機中に手からぶら下げる鎖の長さ
static constexpr float RAISE_PITCH = XMConvertToRadians(75.0f); // 回している間の腕の上下の向き
static constexpr float STRAIGHTEN_SECONDS = 0.1f;
static constexpr float MODEL_SCALE = 0.08f * Arm::MODEL_SIZE;
static constexpr float ARM_LENGTH = 19.250364f; // arm_shield_base の長さ（cm）
static constexpr XMFLOAT4 BALL_COLOR{ 0.35f, 0.35f, 0.4f, 1.0f };
static constexpr XMFLOAT4 SPIKE_COLOR{ 0.8f, 0.8f, 0.85f, 1.0f };
static constexpr XMFLOAT4 CHAIN_COLOR{ 0.6f, 0.6f, 0.65f, 1.0f };
static constexpr float HIT_RUMBLE_SMALL = 1.0f;
static constexpr float HIT_RUMBLE_LARGE = 1.0f;
static constexpr float HIT_RUMBLE_SECONDS = 0.35f;
static constexpr float FAN_SE_VOLUME = 0.7f; // 単押しで振るときに鳴らす「鎖」の音の大きさ（投げは 1.0）
static constexpr float SPIN_RUMBLE_SECONDS = 0.2f;

enum FAN_PHASE { FAN_WINDUP, FAN_STRIKE, FAN_RECOVER, FAN_PHASE_COUNT };

FlailArm::FlailArm(int hand, const ARM_LINKS& links) : Arm(hand, links)
{
	m_ArmModel = LoadModel("assets/models/arm_shield_base.obj");
	Reset();
}

// 鉄球の重い衝撃：火花（大）・重い土ぼこり・画ぶれ（plan/11 の 2-6。見た目だけ）
void FlailArm::SpawnHeavyEffects(int target)
{
	FighterManager& fighters = GetFighters();
	fighters.SpawnEffect(EFFECT_HIT_SPARK_HEAVY, fighters.GetBodyPoint(target, 0.55f), { 0.0f, 1.0f, 0.0f });
	fighters.SpawnEffect(EFFECT_DUST_RING_LARGE, fighters.GetBodyPoint(target, 0.05f), { 0.0f, 1.0f, 0.0f });
	fighters.AddShake(target, HEAVY_SHAKE);
	fighters.AddShake(GetSlot(), HEAVY_SHAKE);
}

void FlailArm::Reset()
{
	if (!m_Phase.IsIdle() && !m_FanConsumed) ConsumeDurability(GetCost(COST_ROW, ARM_COST_TAP_MISS));
	if (m_Ball.IsFlying() && !m_ThrowHit) ConsumeDurability(GetCost(COST_ROW, ARM_COST_HOLD_MISS));
	m_Phase.Reset();
	m_Ball.Reset();
	m_FanConsumed = true;
	m_Spinning = false;
	m_SpinSe.Stop();
	m_SpinSeconds = 0.0f;
	m_ThrowHit = true;
	m_ThrowClipSeconds = -1.0f;
	m_Straighten = 0.0f;
}

void FlailArm::Interrupt()
{
	const float dt = fmaxf(0.0f, GameTimeGetDeltaTime());
	if (!m_Phase.IsIdle() && !m_FanConsumed) ConsumeDurability(GetCost(COST_ROW, ARM_COST_TAP_MISS));
	m_FanConsumed = true;
	m_Phase.Reset();
	m_Spinning = false;
	m_SpinSe.Stop();
	m_SpinSeconds = 0.0f;
	m_Straighten = 0.0f;
	StepBall(dt);
}

VEC3 FlailArm::GetSpinCenter() const
{
	const XMFLOAT3 feet = GetPlayer().GetPosition();
	return { feet.x, feet.y + SPIN_HEIGHT, feet.z };
}

// ロックオン中は相手の体の中心へ（上下も）。フリーカメラなどではカメラを上へ向けた分だけ上へ投げる。
VEC3 FlailArm::GetThrowDirection() const
{
	if (GetAimTarget() >= 0) return GetAim(true);
	const float pitch = fminf(THROW_PITCH_MAX, fmaxf(0.0f, GetCamera().GetPitch()));
	return ArmCalcPitchDirection(GetAim(false), pitch);
}

void FlailArm::StrikeFan()
{
	m_Direction = GetAim(false);
	bool hit = false;
	int hitTarget = -1; // 音を鳴らす相手（何人に当たっても音は1回）
	// ロックオン中は、高台の上・下の相手へも扇を傾けて振る。
	const VEC3 strike = GetStrikeDirection(m_Direction);
	const auto inside = [&](const CAPSULE& body) {
		return AreaCalcInAimedSector(GetShoulder(), strike, 0.0f, FAN_RADIUS, FAN_HALF_ANGLE, FAN_HALF_HEIGHT, body);
	};
	StrikePillars(inside, FAN_DAMAGE, m_Direction);
	for (int i = 0; i < FIGHTER_COUNT; ++i) {
		const TENTACLE_TARGET target = GetFighters().GetMeleeTarget(i, GetSlot());
		if (!target.Available || !inside(target.Body)) continue;
		if (GetFighters().Launch(i, GetSlot(), AreaCalcLaunchDirection(m_Direction, FAN_UP), FAN_DAMAGE, FAN_POWER)) {
			hit = true;
			SpawnHeavyEffects(i);
			hitTarget = i;
		}
	}
	if (hit) {
		Rumble(HIT_RUMBLE_SMALL, HIT_RUMBLE_LARGE, HIT_RUMBLE_SECONDS);
		PlayHitSe(SE_FLAIL_HIT, hitTarget);
		if (!m_FanConsumed) ConsumeDurability(GetCost(COST_ROW, ARM_COST_TAP_HIT));
		m_FanConsumed = true;
	}
}

// 頭の上で回し、輪の中（全方向）にいる相手を外へ小さく弾く（同じ相手には少しあけて当てる）。
// 回している間は1秒ごとに耐久が減る。
void FlailArm::StepSpin(float dt)
{
	m_SpinSeconds += dt;
	m_SpinDrainSeconds += dt;
	while (m_SpinDrainSeconds >= 1.0f) {
		m_SpinDrainSeconds -= 1.0f;
		ConsumeDurability(SPIN_DRAIN_COST);
	}
	m_SpinAngle = fmodf(m_SpinAngle + SPIN_SPEED * dt, XM_2PI);
	const VEC3 center = GetSpinCenter();
	bool hit = false;
	int hitTarget = -1;
	// 輪の半径（＋鉄球の太さ）の中なら、前後左右どこでも当たる（扇の開きを半周にして全方向）。
	const auto inside = [&](const CAPSULE& body) {
		return AreaCalcInSector(center, GetForward(), 0.0f, SPIN_RADIUS + BALL_RADIUS, XM_PI, BALL_RADIUS, body);
	};
	StrikePillarsOutward(inside, SPIN_DAMAGE, center);
	for (int i = 0; i < FIGHTER_COUNT; ++i) {
		m_SpinCooldowns[i] = fmaxf(0.0f, m_SpinCooldowns[i] - dt);
		const TENTACLE_TARGET target = GetFighters().GetMeleeTarget(i, GetSlot());
		if (!target.Available || m_SpinCooldowns[i] > 0.0f || !inside(target.Body)) continue;
		m_SpinCooldowns[i] = SPIN_HIT_COOLDOWN;
		if (GetFighters().Launch(i, GetSlot(), AreaCalcRadialDirection(center, target.Body, SPIN_UP, GetForward()), SPIN_DAMAGE, SPIN_POWER)) {
			hit = true;
			hitTarget = i;
		}
	}
	if (hit) {
		Rumble(HIT_RUMBLE_SMALL, HIT_RUMBLE_LARGE, SPIN_RUMBLE_SECONDS);
		PlayHitSe(SE_FLAIL_HIT, hitTarget);
	}
}

void FlailArm::ThrowBall()
{
	const float charge = FlailCalcCharge(m_SpinSeconds, SPIN_FULL_SECONDS);
	m_ThrowDirection = GetThrowDirection();
	m_Direction = GetAim(false);
	m_ThrowPower = FlailCalcMix(OUT_POWER_MIN, OUT_POWER_MAX, charge);
	m_Ball.Throw(GetShoulder(), m_ThrowDirection, FlailCalcMix(THROW_RANGE_MIN, THROW_RANGE_MAX, charge));
	m_OutHits.Clear();
	m_BackHits.Clear();
	m_ThrowHit = false;
	m_ThrowClipSeconds = 0.0f;
	m_Spinning = false;
	m_SpinSeconds = 0.0f;
	m_SpinSe.Stop();
	PlaySe(SE_FLAIL_CHAIN);
}

// 投げた鉄球を進め、行きは投げた向きへ、帰りは自分の方へ飛ばす（行きと帰りで同じ相手に1回ずつ当たる）。
void FlailArm::StepBall(float dt)
{
	if (!m_Ball.IsFlying()) return;
	const FLAIL_STATE moving = m_Ball.GetState();
	if (moving != FLAIL_IDLE) GetFighters().SpawnEffect(EFFECT_TRAIL, m_Ball.GetPosition(), { 0.0f, 1.0f, 0.0f }); // 飛んでいる鉄球の軌跡（見た目だけ）
	m_Ball.Step(dt, GetShoulder(), THROW_PARAMS);
	const CAPSULE sweep{ m_Ball.GetPrevious(), m_Ball.GetPosition(), BALL_RADIUS };
	StrikePillarsOnLine(sweep, moving == FLAIL_OUT ? OUT_DAMAGE : BACK_DAMAGE, m_Ball.GetMoveDirection());
	HitRecord& hits = moving == FLAIL_OUT ? m_OutHits : m_BackHits;
	bool hit = false;
	int hitTarget = -1;
	for (int i = 0; i < FIGHTER_COUNT; ++i) {
		const TENTACLE_TARGET target = GetFighters().GetTarget(i, GetSlot());
		if (!target.Available || hits.Contains(i) || !CollisionCapsuleCapsule(sweep, target.Body).Hit) continue;
		hits.TryAdd(i);
		const bool applied = moving == FLAIL_OUT
			? GetFighters().Launch(i, GetSlot(), AreaCalcLaunchDirection(m_ThrowDirection, OUT_UP), OUT_DAMAGE, m_ThrowPower)
			: GetFighters().Launch(i, GetSlot(), AreaCalcLaunchDirection(m_Ball.GetMoveDirection(), BACK_UP), BACK_DAMAGE, BACK_POWER);
		if (!applied) continue;
		hit = true;
		if (moving == FLAIL_OUT) SpawnHeavyEffects(i);
		hitTarget = i;
		if (!m_ThrowHit) ConsumeDurability(GetCost(COST_ROW, ARM_COST_HOLD_HIT));
		m_ThrowHit = true;
	}
	if (hit) {
		Rumble(HIT_RUMBLE_SMALL, HIT_RUMBLE_LARGE, HIT_RUMBLE_SECONDS);
		PlayHitSe(SE_FLAIL_HIT, hitTarget);
	}
	if (!m_Ball.IsFlying() && !m_ThrowHit) {
		ConsumeDurability(GetCost(COST_ROW, ARM_COST_HOLD_MISS));
		m_ThrowHit = true;
	}
}

void FlailArm::Update(const ARM_INPUT& input)
{
	const float dt = fmaxf(0.0f, GameTimeGetDeltaTime());
	if (GetPlayer().DidRespawn()) Reset();
	if (m_Phase.IsIdle() && !m_Spinning && !m_Ball.IsFlying()) {
		if (input.Tap) {
			m_Direction = GetAim(false);
			m_FanConsumed = false;
			m_Phase.Start(FAN_WINDUP);
		}
		else if (input.HoldStarted) {
			m_Spinning = true;
			m_SpinSeconds = 0.0f;
			m_SpinAngle = atan2f(GetForward().X, GetForward().Z);
			m_SpinDrainSeconds = 0.0f;
			for (float& cooldown : m_SpinCooldowns) cooldown = 0.0f;
		}
	}
	if (m_Phase.GetPhase() == FAN_WINDUP) m_Direction = GetAim(false);
	const ARM_PHASE_STEP step = m_Phase.Step(dt, FAN_SECONDS, FAN_PHASE_COUNT);
	if (step.Entered == FAN_STRIKE) {
		PlaySe(SE_FLAIL_CHAIN, FAN_SE_VOLUME);
		StrikeFan();
	}
	if (step.Finished && !m_FanConsumed) {
		ConsumeDurability(GetCost(COST_ROW, ARM_COST_TAP_MISS));
		m_FanConsumed = true;
	}
	if (m_Spinning) {
		// 離したら、ためた分だけ遠くへ投げる。回している間に耐久が尽きたら、その場で投げる。
		if (!input.Pressed || !CanUse()) ThrowBall();
		else {
			m_SpinSe.KeepAt(SE_FLAIL_SPIN, GetSpinCenter(), GetSlot()); // 鳴り終わったらもう一度鳴らす（動いたら音の場所もついてくる）
			StepSpin(dt);
		}
	}
	StepBall(dt);
	if (m_ThrowClipSeconds >= 0.0f) {
		m_ThrowClipSeconds += dt;
		if (m_ThrowClipSeconds >= THROW_CLIP_SECONDS) m_ThrowClipSeconds = -1.0f;
	}
	const float speed = dt / STRAIGHTEN_SECONDS;
	const bool posed = !m_Phase.IsIdle() || m_Spinning || m_Ball.IsFlying();
	m_Straighten = posed ? fminf(1.0f, m_Straighten + speed) : fmaxf(0.0f, m_Straighten - speed);
}

void FlailArm::DrawChain(const VEC3& from, const VEC3& to, float scale) const
{
	const VEC3 span = Vec3Sub(to, from);
	const float length = Vec3Length(span);
	const float gap = CHAIN_LINK_GAP * scale;
	const int links = length > gap ? static_cast<int>(length / gap) : 1;
	for (int k = 1; k < links; ++k) {
		const VEC3 p = Vec3Add(from, Vec3Scale(span, static_cast<float>(k) / links));
		ShapeDrawSphere({ p.X, p.Y, p.Z }, CHAIN_LINK_RADIUS * scale, CHAIN_COLOR);
	}
}

void FlailArm::DrawBall(const VEC3& center, float scale) const
{
	ShapeDrawSphere({ center.X, center.Y, center.Z }, BALL_DRAW_RADIUS * scale, BALL_COLOR);
	// 上下・左右・前後と斜めにトゲを出す（回して見えるように、回す角度で少しひねる）。
	for (int k = 0; k < BALL_SPIKE_COUNT; ++k) {
		const float yaw = XM_2PI * k / 4.0f + m_SpinAngle * 0.5f;
		const float pitch = k < 4 ? XMConvertToRadians(35.0f) : XMConvertToRadians(-35.0f);
		const VEC3 out{ sinf(yaw) * cosf(pitch), sinf(pitch), cosf(yaw) * cosf(pitch) };
		const VEC3 start = Vec3Add(center, Vec3Scale(out, BALL_DRAW_RADIUS * scale * 0.8f));
		const VEC3 end = Vec3Add(center, Vec3Scale(out, (BALL_DRAW_RADIUS + BALL_SPIKE_LENGTH) * scale));
		ShapeDrawCylinder({ start.X, start.Y, start.Z }, { end.X, end.Y, end.Z }, BALL_SPIKE_RADIUS * scale, SPIKE_COLOR);
	}
}

void FlailArm::Draw()
{
	const float drawScale = GetDrawScale();
	// 回している間は腕を上へ、投げた鉄球へは腕を向け、扇は前へ振る。
	VEC3 aim = ArmCalcPitchDirection(m_Direction, 0.0f);
	if (m_Spinning) aim = ArmCalcPitchDirection(GetForward(), RAISE_PITCH);
	else if (m_Ball.IsFlying()) {
		const VEC3 toBall = Vec3Sub(m_Ball.GetPosition(), GetDrawShoulder());
		const float length = Vec3Length(toBall);
		if (length > 0.001f) aim = Vec3Scale(toBall, 1.0f / length);
	}
	VEC3 armDirection = ArmCalcBlendDirection(GetRestDirection(), aim, m_Straighten);
	VEC3 unusedWeapon = armDirection;
	BlendClipPose(armDirection, unusedWeapon);
	const XMFLOAT4 color = GetColor(!m_Phase.IsIdle() || m_Spinning);
	const VEC3 hand = DrawAlong(m_ArmModel, GetDrawShoulder(), armDirection, ARM_LENGTH, MODEL_SCALE * drawScale, color);
	VEC3 ball = Vec3Add(hand, { 0.0f, -HANG_LENGTH * drawScale, 0.0f });
	if (m_Ball.IsFlying()) ball = m_Ball.GetPosition();
	else if (m_Spinning) ball = FlailCalcSpinPosition(GetSpinCenter(), m_SpinAngle, SPIN_RADIUS);
	else if (!m_Phase.IsIdle()) {
		// 扇：振りかぶりで横へ引き、振りで反対の横まで振り抜く。
		const float swing = m_Phase.GetPhase() == FAN_WINDUP ? -1.0f
			: (m_Phase.GetPhase() == FAN_STRIKE ? -1.0f + 2.0f * fminf(1.0f, m_Phase.GetTime() / FAN_SECONDS[FAN_STRIKE]) : 1.0f);
		const float side = GetHand() == 0 ? -1.0f : 1.0f;
		const VEC3 out = AreaCalcRotateYaw(m_Direction, swing * side * FAN_HALF_ANGLE);
		ball = Vec3Add(GetDrawShoulder(), Vec3Scale(out, FAN_RADIUS * 0.8f));
	}
	DrawChain(hand, ball, drawScale);
	DrawBall(ball, m_Ball.IsFlying() || m_Spinning || !m_Phase.IsIdle() ? 1.0f : drawScale);
}

bool FlailArm::IsActive() const
{
	return !m_Phase.IsIdle() || m_Spinning;
}

bool FlailArm::IsBusy() const
{
	return m_Ball.IsFlying();
}

bool FlailArm::IsHolding() const
{
	return false;
}

bool FlailArm::CanTakeTap() const
{
	return !IsActive() && !m_Ball.IsFlying();
}

bool FlailArm::CanTakeHold() const
{
	return !IsActive() && !m_Ball.IsFlying();
}

float FlailArm::GetMoveScale() const
{
	return m_Spinning ? SPIN_MOVE_SCALE : 1.0f;
}

bool FlailArm::BlocksJump() const
{
	return m_Spinning; // 回している間もダッシュ（ステップ）はできる
}

ARM_BODY_CLIP FlailArm::GetBodyClip() const
{
	ARM_BODY_CLIP clip;
	if (!m_Phase.IsIdle()) {
		clip.Active = true;
		clip.Kind = ATTACK_CLIP::KnucklePunch;
		clip.Time = ArmCalcClipTime(m_Phase.GetPhase(), m_Phase.GetTime(), FAN_SECONDS, FAN_CLIP_SECONDS);
	}
	else if (m_ThrowClipSeconds >= 0.0f) {
		// 投げた瞬間は、殴りの振り抜きから見せる。
		clip.Active = true;
		clip.Kind = ATTACK_CLIP::KnucklePunch;
		clip.Time = FAN_CLIP_SECONDS[0] + (FAN_CLIP_SECONDS[1] + FAN_CLIP_SECONDS[2]) * (m_ThrowClipSeconds / THROW_CLIP_SECONDS);
	}
	return clip;
}
