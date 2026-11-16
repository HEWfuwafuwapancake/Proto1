#include "game/arm_spike.h"
#include "game/fightermanager.h"
#include "game/fighter.h"
#include "game/stage.h"
#include "core/gametime.h"
#include "physics/areacalc.h"
#include "physics/armcalc.h"
#include "physics/stagecalc.h"
#include "render/shape.h"

// 調整用の値
// 耐久の減る量は physics/armcostcalc の表のトゲの行（弱：前へ生えるトゲの当たり・外れ、強外れ：茨を置いた瞬間）。
static constexpr ARM_COST_ROW COST_ROW = ARM_COST_ROW_SPIKE;
static constexpr float DIVE_MAX_SECONDS = 3.0f; // 急降下を続ける最長の秒数（足場に着かなければやめる）
static constexpr float TAP_SECONDS[] = { DIVE_MAX_SECONDS, 0.25f, 0.1f, 0.3f }; // 単押し：急降下・ため・殴り・休み
static constexpr float HOLD_SECONDS[] = { DIVE_MAX_SECONDS, 0.35f, 0.1f, 0.35f }; // 長押し：急降下・ため・殴り・休み
static constexpr float TAP_CLIP_SECONDS[] = { 0.0f, 16.0f / 24.0f, 4.0f / 24.0f, 10.0f / 24.0f }; // ハンマーの叩きのクリップの区切り
static constexpr float HOLD_CLIP_SECONDS[] = { 0.0f, 22.0f / 24.0f, 4.0f / 24.0f, 14.0f / 24.0f }; // ハンマーの叩きつけのクリップの区切り
static constexpr float DIVE_SPEED = 30.0f; // 空中から真下へ落ちる速さ
static constexpr float MOVE_SCALE = 0.5f; // ため・殴り・休みの間の歩く速さの倍率
// 単押し：前へ手前から順に生えるトゲ（1m 先から 0.8m おきに 11 本 ≒ 9m）
static constexpr SPIKE_LINE_PARAMS LINE_PARAMS{ 1.0f, 0.8f, 11, 0.45f, 0.08f, 0.5f, 0.15f };
static constexpr float LINE_SPIKE_HEIGHT = 1.4f; // 伸びきったトゲの高さ
static constexpr float LINE_SPIKE_RADIUS = 0.35f; // 当たり判定と見た目の根元の太さ
static constexpr float LINE_HIT_GROWTH = 0.3f; // この伸び具合から当たる
static constexpr float LINE_DAMAGE = 9.0f;
static constexpr float LINE_POWER = 2.2f;
static constexpr float LINE_UP = 1.0f; // 水平の単位方向に足す上向きの量（1.0 で斜め 45°）
// 長押し：周りに残る茨
static constexpr float FIELD_RADIUS = 4.0f; // 茨の半径
static constexpr float FIELD_HALF_HEIGHT = 1.0f; // 茨の上下の幅（足元がこの中なら茨の中）
static constexpr float FIELD_SECONDS = 4.0f; // 茨が残る秒数
static constexpr float FIELD_POP_DAMAGE = 6.0f; // 生えた瞬間に中の相手を外へ弾く
static constexpr float FIELD_POP_POWER = 1.4f;
static constexpr float FIELD_POP_UP = 0.6f;
static constexpr float FIELD_MOVE_SCALE = 0.4f; // 茨の中の相手の歩く速さの倍率
static constexpr float FIELD_DAMAGE_PER_METER = 4.0f; // 茨の中で動いた距離 1m ごとのダメージ%
static constexpr float FIELD_STATUS_SECONDS = 0.15f; // 茨から出たあと、効き目が残る秒数
// 見た目
static constexpr float RAISE_PITCH = XMConvertToRadians(60.0f);
static constexpr float SLAM_PITCH = XMConvertToRadians(-60.0f);
static constexpr float STRAIGHTEN_SECONDS = 0.1f;
static constexpr float MODEL_SCALE = 0.08f * Arm::MODEL_SIZE;
static constexpr float ARM_LENGTH = 19.250364f; // arm_shield_base の長さ（cm）
static constexpr float FIST_LENGTH = 11.7f; // arm_knuckle_fist の長さ（cm）
static constexpr int FIST_SPIKE_COUNT = 4; // こぶしに付けるトゲの数
static constexpr float FIST_SPIKE_LENGTH = 0.18f;
static constexpr float FIST_SPIKE_RADIUS = 0.035f;
static constexpr int FIELD_SPIKE_RINGS = 3; // 茨のトゲを並べる輪の数
static constexpr int FIELD_SPIKES_PER_RING[FIELD_SPIKE_RINGS] = { 7, 13, 20 };
static constexpr float FIELD_SPIKE_HEIGHT = 0.7f;
static constexpr float FIELD_SPIKE_RADIUS = 0.13f;
static constexpr float FIELD_GROW_SECONDS = 0.15f; // 茨が生えきるまで（見た目だけ）
static constexpr float FIELD_FADE_SECONDS = 0.3f; // 茨が引っこむ秒数（見た目だけ）
static constexpr float FIELD_LEAN = 0.35f; // 茨のトゲを外へ傾ける量
static constexpr XMFLOAT4 LINE_COLOR{ 0.78f, 0.74f, 0.62f, 1.0f };
static constexpr XMFLOAT4 FIELD_COLOR{ 0.35f, 0.62f, 0.25f, 1.0f };
static constexpr XMFLOAT4 FIELD_RING_COLOR{ 0.45f, 0.85f, 0.3f, 1.0f };
static constexpr float HIT_RUMBLE_SMALL = 1.0f;
static constexpr float HIT_RUMBLE_LARGE = 1.0f;
static constexpr float HIT_RUMBLE_SECONDS = 0.3f;
static constexpr float SLAM_RUMBLE = 0.6f; // 地面を殴った手ごたえ
static constexpr float SLAM_RUMBLE_SECONDS = 0.15f;

enum SPIKE_PHASE { SPIKE_DIVE, SPIKE_WINDUP, SPIKE_STRIKE, SPIKE_RECOVER, SPIKE_PHASE_COUNT };

SpikeArm::SpikeArm(int hand, const ARM_LINKS& links) : Arm(hand, links)
{
	m_ArmModel = LoadModel("assets/models/arm_shield_base.obj");
	m_FistModel = LoadModel("assets/models/arm_knuckle_fist.obj");
	Reset();
}

const float* SpikeArm::GetSeconds() const
{
	return m_Hold ? HOLD_SECONDS : TAP_SECONDS;
}

// 急降下で止めていた体の計算を、ふつうに戻す。
void SpikeArm::StopDive()
{
	if (!m_Diving) return;
	m_Diving = false;
	GetPlayer().SetPulled(false);
}

void SpikeArm::StopAction()
{
	StopDive();
	m_Phase.Reset();
	m_Straighten = 0.0f;
}

void SpikeArm::Reset()
{
	StopAction();
	ClearLeftover();
}

void SpikeArm::ClearLeftover()
{
	m_Fields.Reset();
	m_LineHits.Clear();
	m_LineActive = false;
	m_LineConsumed = true;
	m_LineCount = 0;
	m_LineElapsed = 0.0f;
}

void SpikeArm::Interrupt()
{
	StopAction();
	StepEffects(fmaxf(0.0f, GameTimeGetDeltaTime()));
}

void SpikeArm::Unequip()
{
	StopAction();
}

void SpikeArm::UpdateLeftover()
{
	StepEffects(fmaxf(0.0f, GameTimeGetDeltaTime()));
}

// 空中：真下へ落ち、足場に着いた瞬間に殴る。足場がなければそのまま落ちる。
void SpikeArm::StepDive(float dt)
{
	Fighter& player = GetPlayer();
	const STAGE& stage = GetStage().GetStage();
	const XMFLOAT3 feet = player.GetPosition();
	const float nextY = feet.y - DIVE_SPEED * dt;
	float landingY = 0.0f;
	if (StageCalcFindLanding(stage, feet.x, feet.z, feet.y, nextY, landingY)) {
		player.SetPosition({ feet.x, landingY, feet.z });
		StopDive();
		m_Phase.Start(SPIKE_STRIKE);
		Strike();
		return;
	}
	player.SetPosition({ feet.x, nextY, feet.z });
	if (StageCalcIsOut(stage, nextY)) StopAction();
}

void SpikeArm::Strike()
{
	m_Direction = GetAim(false);
	const XMFLOAT3 position = GetPlayer().GetPosition();
	const VEC3 feet{ position.x, position.y, position.z };
	Rumble(SLAM_RUMBLE, SLAM_RUMBLE, SLAM_RUMBLE_SECONDS);
	if (!m_Hold) {
		m_LineFeet = feet;
		m_LineDirection = m_Direction;
		m_LineCount = SpikeCalcLineCount(GetStage().GetStage(), feet, m_Direction, LINE_PARAMS);
		m_LineElapsed = 0.0f;
		m_LineHits.Clear();
		m_LineActive = true;
		m_LineConsumed = false;
		return;
	}
	// 茨は置いた瞬間に耐久を減らし、中の相手を外へ小さく弾く。
	m_Fields.Place(feet, FIELD_SECONDS);
	ConsumeDurability(GetCost(COST_ROW, ARM_COST_HOLD_MISS));
	bool hit = false;
	for (int i = 0; i < FIGHTER_COUNT; ++i) {
		const TENTACLE_TARGET target = GetFighters().GetTarget(i, GetSlot());
		if (!target.Available
			|| !AreaCalcInSector(feet, m_Direction, 0.0f, FIELD_RADIUS, XM_PI, FIELD_HALF_HEIGHT, target.Body)) continue;
		if (GetFighters().Launch(i, GetSlot(), AreaCalcRadialDirection(feet, target.Body, FIELD_POP_UP, m_Direction),
			FIELD_POP_DAMAGE, FIELD_POP_POWER)) hit = true;
	}
	if (hit) Rumble(HIT_RUMBLE_SMALL, HIT_RUMBLE_LARGE, HIT_RUMBLE_SECONDS);
}

void SpikeArm::StepLine(float dt)
{
	if (!m_LineActive) return;
	m_LineElapsed += dt;
	bool hit = false;
	for (int s = 0; s < m_LineCount; ++s) {
		const float growth = SpikeCalcLineGrowth(LINE_PARAMS, s, m_LineElapsed);
		if (growth < LINE_HIT_GROWTH) continue;
		const VEC3 base = SpikeCalcLinePosition(m_LineFeet, m_LineDirection, LINE_PARAMS, s);
		const CAPSULE spike{ base, Vec3Add(base, { 0.0f, LINE_SPIKE_HEIGHT * growth, 0.0f }), LINE_SPIKE_RADIUS };
		for (int i = 0; i < FIGHTER_COUNT; ++i) {
			const TENTACLE_TARGET target = GetFighters().GetTarget(i, GetSlot());
			if (!target.Available || m_LineHits.Contains(i) || !CollisionCapsuleCapsule(spike, target.Body).Hit) continue;
			m_LineHits.TryAdd(i);
			if (!GetFighters().Launch(i, GetSlot(), AreaCalcLaunchDirection(m_LineDirection, LINE_UP), LINE_DAMAGE, LINE_POWER)) continue;
			hit = true;
			if (!m_LineConsumed) {
				ConsumeDurability(GetCost(COST_ROW, ARM_COST_TAP_HIT));
				m_LineConsumed = true;
			}
		}
	}
	if (hit) Rumble(HIT_RUMBLE_SMALL, HIT_RUMBLE_LARGE, HIT_RUMBLE_SECONDS);
	if (SpikeCalcLineFinished(LINE_PARAMS, m_LineCount, m_LineElapsed)) {
		if (!m_LineConsumed) ConsumeDurability(GetCost(COST_ROW, ARM_COST_TAP_MISS));
		m_LineConsumed = true;
		m_LineActive = false;
	}
}

// 茨の中の相手：跳べない・遅い・ダッシュが短い。動いた距離だけ % が増える（吹っ飛び中・つかまれ中は数えない）。
void SpikeArm::StepFields(float dt)
{
	m_Fields.Step(dt);
	for (int i = 0; i < FIGHTER_COUNT; ++i) {
		if (i == GetSlot()) continue;
		const Fighter& fighter = GetFighters().GetFighter(i);
		const XMFLOAT3 position = fighter.GetPosition();
		const VEC3 feet{ position.x, position.y, position.z };
		const bool inside = m_Fields.GetCount() > 0 && fighter.IsActive() && !fighter.IsLaunched() && !fighter.IsHeld()
			&& m_Fields.IsInside(feet, FIELD_RADIUS, FIELD_HALF_HEIGHT);
		const float moved = m_Fields.TrackMove(i, feet, inside);
		if (inside) {
			GetFighters().ApplyHazard(i, GetSlot(), STATUS_THORN, FIELD_STATUS_SECONDS, FIELD_MOVE_SCALE, moved * FIELD_DAMAGE_PER_METER);
		}
	}
}

void SpikeArm::StepEffects(float dt)
{
	StepLine(dt);
	StepFields(dt);
}

void SpikeArm::Update(const ARM_INPUT& input)
{
	const float dt = fmaxf(0.0f, GameTimeGetDeltaTime());
	if (GetPlayer().DidRespawn()) StopAction();
	if (m_Phase.IsIdle() && (input.Tap || input.HoldStarted)) {
		m_Hold = input.HoldStarted;
		m_Direction = GetAim(false);
		// 空中では真下へ急降下し、足場に着いた瞬間に殴る。
		if (GetPlayer().IsOnGround()) m_Phase.Start(SPIKE_WINDUP);
		else if (!GetPlayer().IsPulled()) {
			m_Phase.Start(SPIKE_DIVE);
			m_Diving = true;
			GetPlayer().SetPulled(true);
		}
	}
	if (m_Phase.GetPhase() == SPIKE_DIVE && m_Diving) StepDive(dt);
	// ためている間は狙いを追い続ける（ロックオン中は相手の方へ）。
	if (m_Phase.GetPhase() == SPIKE_WINDUP) m_Direction = GetAim(false);
	const ARM_PHASE_STEP step = m_Phase.Step(dt, GetSeconds(), SPIKE_PHASE_COUNT);
	// 急降下のまま足場に着かなかった（長く落ち続けた）ら、殴らずにやめる。
	if (step.Entered == SPIKE_WINDUP && m_Diving) StopAction();
	else if (step.Entered == SPIKE_STRIKE) Strike();
	StepEffects(dt);
	const float speed = dt / STRAIGHTEN_SECONDS;
	m_Straighten = m_Phase.IsIdle() ? fmaxf(0.0f, m_Straighten - speed) : fminf(1.0f, m_Straighten + speed);
}

void SpikeArm::DrawSpike(const VEC3& base, float height, float radius, const VEC3& lean, const XMFLOAT4& color) const
{
	if (height <= 0.0f) return;
	// 細くなる円柱を3段重ねて、とがった形に見せる。
	static constexpr int STEPS = 3;
	for (int k = 0; k < STEPS; ++k) {
		const float a = static_cast<float>(k) / STEPS;
		const float b = static_cast<float>(k + 1) / STEPS;
		const VEC3 start = Vec3Add(base, Vec3Add(Vec3Scale(lean, height * a), { 0.0f, height * a, 0.0f }));
		const VEC3 end = Vec3Add(base, Vec3Add(Vec3Scale(lean, height * b), { 0.0f, height * b, 0.0f }));
		ShapeDrawCylinder({ start.X, start.Y, start.Z }, { end.X, end.Y, end.Z }, radius * (1.0f - a * 0.8f), color);
	}
}

void SpikeArm::Draw()
{
	float pitch = SLAM_PITCH;
	if (m_Phase.GetPhase() == SPIKE_WINDUP) pitch = RAISE_PITCH;
	const VEC3 aim = m_Phase.GetPhase() == SPIKE_DIVE ? VEC3{ 0.0f, -1.0f, 0.0f } : ArmCalcPitchDirection(m_Direction, pitch);
	VEC3 armDirection = ArmCalcBlendDirection(GetRestDirection(), aim, m_Straighten);
	VEC3 fistDirection = armDirection;
	BlendClipPose(armDirection, fistDirection);
	const XMFLOAT4 color = GetColor(!m_Phase.IsIdle());
	const float scale = MODEL_SCALE * GetDrawScale();
	const VEC3 hand = DrawAlong(m_ArmModel, GetDrawShoulder(), armDirection, ARM_LENGTH, scale, color);
	const VEC3 tip = DrawAlong(m_FistModel, hand, fistDirection, FIST_LENGTH, scale, color);
	// こぶしの先から、横へ開いたトゲを出す。
	const VEC3 side = Vec3Length({ fistDirection.Z, 0.0f, -fistDirection.X }) > 0.001f
		? Vec3Scale(VEC3{ fistDirection.Z, 0.0f, -fistDirection.X }, 1.0f / Vec3Length({ fistDirection.Z, 0.0f, -fistDirection.X }))
		: VEC3{ 1.0f, 0.0f, 0.0f };
	const float drawScale = GetDrawScale();
	for (int k = 0; k < FIST_SPIKE_COUNT; ++k) {
		const float angle = XM_2PI * k / FIST_SPIKE_COUNT;
		const VEC3 up{ 0.0f, 1.0f, 0.0f };
		const VEC3 out = Vec3Add(Vec3Scale(side, cosf(angle)), Vec3Scale(up, sinf(angle)));
		const VEC3 dir = Vec3Add(Vec3Scale(out, 0.7f), Vec3Scale(fistDirection, 0.7f));
		const VEC3 end = Vec3Add(tip, Vec3Scale(dir, FIST_SPIKE_LENGTH * drawScale));
		ShapeDrawCylinder({ tip.X, tip.Y, tip.Z }, { end.X, end.Y, end.Z }, FIST_SPIKE_RADIUS * drawScale, LINE_COLOR);
	}
}

void SpikeArm::DrawLeftover()
{
	if (m_LineActive) {
		for (int s = 0; s < m_LineCount; ++s) {
			const float growth = SpikeCalcLineGrowth(LINE_PARAMS, s, m_LineElapsed);
			const VEC3 base = SpikeCalcLinePosition(m_LineFeet, m_LineDirection, LINE_PARAMS, s);
			// 少し前へ傾けて、地面から突き上がる勢いを見せる。
			DrawSpike(base, LINE_SPIKE_HEIGHT * growth, LINE_SPIKE_RADIUS, Vec3Scale(m_LineDirection, 0.25f), LINE_COLOR);
		}
	}
	for (int f = 0; f < THORN_FIELD_MAX; ++f) {
		const THORN_FIELD& field = m_Fields.Get(f);
		if (!field.Active) continue;
		const float grow = fminf(1.0f, field.Age / FIELD_GROW_SECONDS) * fminf(1.0f, field.Remaining / FIELD_FADE_SECONDS);
		for (int ring = 0; ring < FIELD_SPIKE_RINGS; ++ring) {
			const float radius = FIELD_RADIUS * (ring + 1) / (FIELD_SPIKE_RINGS + 0.3f);
			const int count = FIELD_SPIKES_PER_RING[ring];
			for (int k = 0; k < count; ++k) {
				// 輪ごとに少しずらし、高さも少しずつ変えて、絡まった茨に見せる。
				const float angle = XM_2PI * (k + 0.37f * ring) / count;
				const VEC3 out{ sinf(angle), 0.0f, cosf(angle) };
				const VEC3 base = Vec3Add(field.Center, Vec3Scale(out, radius));
				const float height = FIELD_SPIKE_HEIGHT * (0.75f + 0.25f * sinf(angle * 3.0f + ring)) * grow;
				DrawSpike(base, height, FIELD_SPIKE_RADIUS, Vec3Scale(out, FIELD_LEAN), FIELD_COLOR);
			}
		}
		DrawRangeRing(Vec3Add(field.Center, { 0.0f, 0.05f, 0.0f }), FIELD_RADIUS, FIELD_RING_COLOR);
	}
}

bool SpikeArm::IsActive() const
{
	return !m_Phase.IsIdle();
}

bool SpikeArm::IsHolding() const
{
	return false;
}

float SpikeArm::GetMoveScale() const
{
	return m_Phase.IsIdle() || m_Phase.GetPhase() == SPIKE_DIVE ? 1.0f : MOVE_SCALE;
}

ARM_BODY_CLIP SpikeArm::GetBodyClip() const
{
	ARM_BODY_CLIP clip;
	if (m_Phase.IsIdle()) return clip;
	clip.Active = true;
	clip.Kind = m_Hold ? ATTACK_CLIP::HammerSlam : ATTACK_CLIP::HammerTap;
	clip.FullBody = m_Hold;
	clip.Time = ArmCalcClipTime(m_Phase.GetPhase(), m_Phase.GetTime(), GetSeconds(), m_Hold ? HOLD_CLIP_SECONDS : TAP_CLIP_SECONDS);
	return clip;
}
