#include "game/arm_knuckle.h"
#include "audio/se.h"
#include "game/fightermanager.h"
#include "game/fighter.h"
#include "game/stage.h"
#include "game/camera.h"
#include "core/gametime.h"
#include "core/mathutil.h"
#include "physics/areacalc.h"
#include "physics/armcalc.h"

// 調整用の値
// 耐久の減る量は physics/armcostcalc の表のナックルの行（弱：単押し、強：突進。空中の突進も地上と同じく、当たりか外れで1回だけ減らす）。
static constexpr ARM_COST_ROW COST_ROW = ARM_COST_ROW_KNUCKLE;
// 単押しは3段：1段目・2段目は軽く殴って相手を近くに残し、3段目のアッパーで斜め上へ大きく飛ばす。
static constexpr int COMBO_STEPS = 3;
static constexpr float COMBO_COOLDOWN = 0.5f; // 最後の段を出し終えてから、この腕で攻撃できない秒数。
static constexpr float COMBO_WINDOW = 0.35f; // 休みが終わってから、次の段の単押しを受け付ける秒数（休みの間も受け付ける）。
static constexpr float PUNCH_SECONDS[COMBO_STEPS][3] = { // 単押しの段ごと：ため・殴り・休み
	{ 0.12f, 0.1f, 0.15f }, { 0.08f, 0.1f, 0.15f }, { 0.14f, 0.12f, 0.3f } };
static constexpr float PUNCH_CLIP_SECONDS[COMBO_STEPS][3] = { // 段ごとのクリップの区切り（knuckle_punch・knuckle_hook・knuckle_upper）
	{ 4.0f / 24.0f, 3.0f / 24.0f, 5.0f / 24.0f }, { 3.0f / 24.0f, 3.0f / 24.0f, 6.0f / 24.0f }, { 5.0f / 24.0f, 4.0f / 24.0f, 10.0f / 24.0f } };
static constexpr ATTACK_CLIP PUNCH_CLIPS[COMBO_STEPS] = { ATTACK_CLIP::KnucklePunch, ATTACK_CLIP::KnuckleHook, ATTACK_CLIP::KnuckleUpper };
static constexpr float RUSH_SECONDS[] = { 0.3f, 0.35f, 0.22f }; // 長押し：ため・突進・休み
static constexpr float RUSH_CLIP_SECONDS[] = { 10.0f / 24.0f, 8.0f / 24.0f, 8.0f / 24.0f }; // 長押しのクリップの区切り（knuckle_rush）
static constexpr float PUNCH_REACH = 1.6f; // 肩から殴りの届く距離。
static constexpr float PUNCH_RADIUS = 0.4f;
static constexpr float PUNCH_DAMAGE[COMBO_STEPS] = { 5.0f, 5.0f, 10.0f };
static constexpr float PUNCH_POWER[COMBO_STEPS] = { 0.6f, 0.6f, 2.6f };
static constexpr float PUNCH_UP[COMBO_STEPS] = { 0.2f, 0.2f, 1.4f }; // 水平の単位方向に足す上向きの量（アッパーは斜め上へ）。
static constexpr float RUSH_SPEED = 28.0f;
static constexpr float RUSH_REACH = 1.2f; // 突進の当たりの、肩から前への距離。
static constexpr float RUSH_RADIUS = 0.5f;
static constexpr float RUSH_DAMAGE = 10.0f;
static constexpr float RUSH_POWER = 2.2f;
static constexpr float RUSH_UP = 0.6f;
static constexpr float AIR_RUSH_PITCH = XMConvertToRadians(40.0f); // 空中の突進で、狙いの向きを上へ傾ける角度（場外から戻る）。
static constexpr float CHARGE_MOVE_SCALE = 0.5f; // 突進のためと休みの間の歩く速さの倍率。
static constexpr float RAISE_PITCH = XMConvertToRadians(-15.0f); // 構えの間の腕の上下の角度。
static constexpr float STRAIGHTEN_SECONDS = 0.1f;
static constexpr float EXTEND_SECONDS = 0.05f;
static constexpr float MODEL_SCALE = 0.08f * Arm::MODEL_SIZE;
static constexpr float ARM_LENGTH = 19.250364f; // arm_shield_base の長さ（cm）。
static constexpr float FIST_LENGTH = 11.7f; // arm_knuckle_fist の長さ（cm）。
static constexpr float FIST_SIZE = 1.5f; // 拳だけを大きく描く倍率。
static constexpr float HIT_RUMBLE_SMALL = 1.0f;
static constexpr float HIT_RUMBLE_LARGE = 1.0f;
static constexpr float HIT_RUMBLE_SECONDS = 0.25f;

enum KNUCKLE_PHASE { KNUCKLE_CHARGE, KNUCKLE_STRIKE, KNUCKLE_RECOVER, KNUCKLE_PHASE_COUNT };

KnuckleArm::KnuckleArm(int hand, const ARM_LINKS& links) : Arm(hand, links)
{
	m_ArmModel = LoadModel("assets/models/arm_shield_base.obj");
	m_FistModel = LoadModel("assets/models/arm_knuckle_fist.obj");
	Reset();
}

// 空中の突進で止めていた体の計算を、ふつうに戻す。
void KnuckleArm::StopAirRush()
{
	if (!m_AirRush) return;
	m_AirRush = false;
	m_Rise.Reset();
	GetPlayer().SetPulled(false);
}

void KnuckleArm::Reset()
{
	if (!m_Phase.IsIdle() && !m_UseConsumed) ConsumeDurability(GetCost(COST_ROW, m_Rush ? ARM_COST_HOLD_MISS : ARM_COST_TAP_MISS));
	m_UseConsumed = true;
	StopAirRush();
	m_Phase.Reset();
	m_Rush = false;
	m_Combo.Reset();
	m_TapStep = 0;
	m_AirRushClip = false;
	m_Straighten = 0.0f;
	m_Extend = 0.0f;
}

// 斜め上への空中の突進は、進む向きの先で当たりを探す（上にいる相手にも当たる）。
// それ以外は、ロックオン中の相手の高さへ向けて探す（高台の上の相手にも当たる）。
VEC3 KnuckleArm::GetPunchDirection() const
{
	return m_AirRush ? ArmCalcPitchDirection(m_Direction, m_AirRushPitch) : GetStrikeDirection(m_Direction);
}

int KnuckleArm::FindTarget(float reach, float radius) const
{
	TENTACLE_TARGET targets[FIGHTER_COUNT];
	for (int i = 0; i < FIGHTER_COUNT; ++i) targets[i] = GetFighters().GetMeleeTarget(i, GetSlot());
	return AreaCalcFirstOnRay(GetShoulder(), GetPunchDirection(), reach, radius, targets, FIGHTER_COUNT, nullptr);
}

void KnuckleArm::StrikePillarsOnPunch(float reach, float radius, float damage) const
{
	const VEC3 shoulder = GetShoulder();
	StrikePillarsOnLine({ shoulder, Vec3Add(shoulder, Vec3Scale(GetPunchDirection(), reach)), radius }, damage, m_Direction);
}

// 浮島の外にいるか、浮島の上面より下にいるなら場外とみなす。
bool KnuckleArm::IsOffStage() const
{
	const XMFLOAT3 feet = GetPlayer().GetPosition();
	const STAGE& stage = GetStage().GetStage();
	return feet.y < stage.TopY || !StageCalcIsOnPlatform(stage, feet.x, feet.z);
}

// 空中の突進で上へ傾ける角度。0 なら地上と同じ水平の突進。場外は40°。
// 狙う相手がいれば相手が上にいる分だけ、フリーカメラ（狙う相手なし）はカメラが上を向いた分だけ上へ。
float KnuckleArm::GetAirRushPitch() const
{
	if (GetPlayer().IsOnGround() || GetPlayer().IsPulled()) return 0.0f;
	const bool aimed = GetAimTarget() >= 0;
	const float aimPitch = aimed ? asinf(GetStrikeDirection(m_Direction).Y) : GetCamera().GetPitch();
	return ArmCalcAirRushPitch(IsOffStage(), aimed || GetCamera().IsFreeLook(), aimPitch, AIR_RUSH_PITCH);
}

void KnuckleArm::Strike(int target, float up, float damage, float power)
{
	// 1動作で1回だけ減らす（同じ動作でもう減らしていたら重ねない）。
	const bool applied = GetFighters().Launch(target, GetSlot(), AreaCalcLaunchDirection(m_Direction, up), damage, power);
	if (applied && m_Rush) GetFighters().SpawnEffect(EFFECT_RUSH_STREAK, GetFighters().GetBodyPoint(target, 0.55f), m_Direction);
	if (applied && !m_UseConsumed) {
		ConsumeDurability(GetCost(COST_ROW, m_Rush ? ARM_COST_HOLD_HIT : ARM_COST_TAP_HIT));
		m_UseConsumed = true;
	}
	Rumble(HIT_RUMBLE_SMALL, HIT_RUMBLE_LARGE, HIT_RUMBLE_SECONDS);
}

void KnuckleArm::Update(const ARM_INPUT& input)
{
	const float dt = fmaxf(0.0f, GameTimeGetDeltaTime());
	if (GetPlayer().DidRespawn()) Reset();
	m_Combo.Step(dt);
	// 段階攻撃の休みの間に単押しされたら、休みを切り上げて次の段を出す。
	if (input.Tap && !m_Rush && m_Phase.GetPhase() == KNUCKLE_RECOVER && m_Combo.IsOpen()) {
		if (!m_UseConsumed) ConsumeDurability(GetCost(COST_ROW, ARM_COST_TAP_MISS));
		m_Phase.Reset();
	}
	// 長押しは押している間ためた姿勢で待ち、離した瞬間に突進する。
	if (m_Phase.IsIdle() && (input.Tap || input.HoldStarted || input.HoldReleased)) {
		m_Rush = !input.Tap;
		// 単押しは受付の間なら次の段、そうでなければ1段目。長押しは段を切る。
		if (m_Rush) m_Combo.Reset();
		else m_TapStep = m_Combo.Start(COMBO_STEPS);
		// 判定で使う m_AirRush と分け、ため中に場外へ出入りしてもクリップは切り替えない。
		m_AirRushClip = m_Rush && GetAirRushPitch() > 0.0f;
		m_UseConsumed = false;
		m_Direction = GetAim(false);
		m_Phase.Start(KNUCKLE_CHARGE);
		m_Phase.SetHeld(m_Rush);
		if (input.HoldReleased) m_Phase.Release();
		else if (m_Rush) PlaySe(SE_ARM_HOLD);
	}
	else if (input.HoldReleased && m_Rush) m_Phase.Release();
	// ためている間にボタンを受け付けなくなった（ガードなど）ときは、出さずにやめる。
	else if (!input.Pressed && m_Phase.IsHeld()) m_Phase.Reset();
	const ARM_PHASE_STEP step = m_Phase.Step(dt, GetSeconds(), KNUCKLE_PHASE_COUNT);
	// 最後の段でなければ、休みに入ったときに次の段の受付を開く。
	if (step.Entered == KNUCKLE_RECOVER && !m_Rush && m_TapStep < COMBO_STEPS - 1) {
		m_Combo.Open(PUNCH_SECONDS[m_TapStep][KNUCKLE_RECOVER] + COMBO_WINDOW);
	}
	if (step.Entered == KNUCKLE_STRIKE) {
		m_Direction = GetAim(false);
		if (m_Rush) PlaySe(SE_KNUCKLE_HOLD);
		// 地上の突進は、足元から後ろへ土ぼこり（見た目だけ）。
		if (m_Rush && GetPlayer().IsOnGround()) {
			GetFighters().SpawnEffect(EFFECT_DUST_KICK, GetFighters().GetBodyPoint(GetSlot(), 0.05f), Vec3Scale(m_Direction, -1.0f));
		}
		if (!m_Rush) {
			// 拳の先に小さな白い輪（空振りでも出す。見た目だけ）。
			GetFighters().SpawnEffect(EFFECT_PUNCH_RING, Vec3Add(GetShoulder(), Vec3Scale(GetPunchDirection(), PUNCH_REACH)), GetPunchDirection());
			StrikePillarsOnPunch(PUNCH_REACH, PUNCH_RADIUS, PUNCH_DAMAGE[m_TapStep]);
			const int target = FindTarget(PUNCH_REACH, PUNCH_RADIUS);
			if (target >= 0) Strike(target, PUNCH_UP[m_TapStep], PUNCH_DAMAGE[m_TapStep], PUNCH_POWER[m_TapStep]);
			// 空振りは刀の空振りの音を使う（No.195。ナックル空振り.wav は表に残す）。
			if (target >= 0) PlayHitSe(SE_KNUCKLE_TAP, target);
			else PlaySe(SE_KATANA_MISS);
		}
		else if (GetAirRushPitch() > 0.0f) {
			const float pitch = GetAirRushPitch();
			// 場外の空中の突進は前斜め上へ進む（場外から戻れる）。フリーカメラではカメラを上へ向けた分だけ上へ進む。
			// 水平の速度しか体に伝わらないので、自分で体を動かす。角度が 0 なら、地上と同じく水平に突進する。
			const XMFLOAT3 feet = GetPlayer().GetPosition();
			const float distance = RUSH_SPEED * RUSH_SECONDS[KNUCKLE_STRIKE] * TuningCalcValue(GetTuning(), TUNING_RUSH_DISTANCE);
			m_Rise.Start({ feet.x, feet.y, feet.z }, RiseCalcSlopeOffset(m_Direction, pitch, distance), RUSH_SECONDS[KNUCKLE_STRIKE], false);
			m_AirRush = true;
			m_AirRushPitch = pitch;
			GetPlayer().SetPulled(true);
		}
	}
	if (m_AirRush) {
		if (m_Phase.GetPhase() == KNUCKLE_STRIKE && m_Rise.IsActive()) {
			const VEC3 next = m_Rise.Step(dt);
			GetPlayer().SetPosition({ next.X, next.Y, next.Z });
		}
		else StopAirRush();
	}
	if (m_Rush && m_Phase.GetPhase() == KNUCKLE_STRIKE) {
		GetPlayer().RequestGhosts(); // 突進の間は体の後ろに残像（見た目だけ）。
		StrikePillarsOnPunch(RUSH_REACH, RUSH_RADIUS, RUSH_DAMAGE);
		const int target = FindTarget(RUSH_REACH, RUSH_RADIUS);
		if (target >= 0) {
			Strike(target, RUSH_UP, RUSH_DAMAGE, RUSH_POWER);
			m_Phase.Start(KNUCKLE_RECOVER); // 当たったら止まる。
			StopAirRush();
		}
	}
	// アッパーを出し終えたら、しばらくこの腕で攻撃できない。
	if (step.Finished && !m_Rush && m_TapStep == COMBO_STEPS - 1) m_Combo.StartCooldown(COMBO_COOLDOWN);
	if (step.Finished && !m_UseConsumed) {
		ConsumeDurability(GetCost(COST_ROW, m_Rush ? ARM_COST_HOLD_MISS : ARM_COST_TAP_MISS));
		m_UseConsumed = true;
	}
	const float speed = dt / STRAIGHTEN_SECONDS;
	m_Straighten = m_Phase.IsIdle() ? fmaxf(0.0f, m_Straighten - speed) : fminf(1.0f, m_Straighten + speed);
	const float extendSpeed = dt / EXTEND_SECONDS;
	m_Extend = (!m_Rush && m_Phase.GetPhase() == KNUCKLE_STRIKE) ? fminf(1.0f, m_Extend + extendSpeed) : fmaxf(0.0f, m_Extend - extendSpeed);
}

void KnuckleArm::Draw()
{
	const bool striking = m_Phase.GetPhase() == KNUCKLE_STRIKE;
	const VEC3 aim = striking ? m_Direction : ArmCalcPitchDirection(m_Direction, RAISE_PITCH);
	VEC3 direction = ArmCalcBlendDirection(GetRestDirection(), aim, m_Straighten);
	VEC3 unusedWeapon = direction; // 拳は腕の向きのまま描くので、武器の向きは使わない。
	BlendClipPose(direction, unusedWeapon);
	const XMFLOAT4 color = GetColor(!m_Phase.IsIdle());
	const float scale = MODEL_SCALE * GetDrawScale();
	const float fistLength = FIST_LENGTH * scale * FIST_SIZE;
	const float armLength = Lerp(ARM_LENGTH * scale, fmaxf(ARM_LENGTH * scale, PUNCH_REACH - fistLength), m_Extend);
	const VEC3 shoulder = GetDrawShoulder();
	const VEC3 hand = Vec3Add(shoulder, Vec3Scale(direction, armLength));
	if (m_ArmModel >= 0) DrawPiece(m_ArmModel, shoulder, hand, ARM_LENGTH, scale, color);
	DrawAlong(m_FistModel, hand, direction, FIST_LENGTH, scale * FIST_SIZE, color);
}

bool KnuckleArm::IsActive() const
{
	return !m_Phase.IsIdle();
}

ARM_BODY_CLIP KnuckleArm::GetBodyClip() const
{
	ARM_BODY_CLIP clip;
	if (m_Phase.IsIdle()) return clip;
	clip.Active = true;
	clip.Kind = m_Rush ? (m_AirRushClip ? ATTACK_CLIP::KnuckleAirRush : ATTACK_CLIP::KnuckleRush) : PUNCH_CLIPS[m_TapStep];
	// アッパーは膝から伸び上がるので全身で見せる。
	clip.FullBody = m_Rush || m_TapStep == COMBO_STEPS - 1;
	clip.Time = ArmCalcClipTime(m_Phase.GetPhase(), m_Phase.GetTime(), GetSeconds(), m_Rush ? RUSH_CLIP_SECONDS : PUNCH_CLIP_SECONDS[m_TapStep]);
	return clip;
}

// 今の動作の、段階ごとの秒数の並びを返す。
const float* KnuckleArm::GetSeconds() const
{
	return m_Rush ? RUSH_SECONDS : PUNCH_SECONDS[m_TapStep];
}

bool KnuckleArm::CanTakeTap() const
{
	if (m_Combo.IsCoolingDown()) return false;
	return m_Phase.IsIdle() || (!m_Rush && m_Phase.GetPhase() == KNUCKLE_RECOVER && m_Combo.IsOpen());
}

bool KnuckleArm::IsHolding() const
{
	return false;
}

float KnuckleArm::GetMoveScale() const
{
	return m_Rush && !m_Phase.IsIdle() ? CHARGE_MOVE_SCALE : 1.0f;
}

bool KnuckleArm::CanTakeHold() const
{
	return !m_Combo.IsCoolingDown() && Arm::CanTakeHold();
}

VEC3 KnuckleArm::GetRushVelocity() const
{
	if (!m_Rush || m_AirRush || m_Phase.GetPhase() != KNUCKLE_STRIKE) return {};
	return Vec3Scale(m_Direction, RUSH_SPEED * TuningCalcValue(GetTuning(), TUNING_RUSH_DISTANCE));
}
