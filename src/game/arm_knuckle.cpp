#include "game/arm_knuckle.h"
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
static constexpr float PUNCH_SECONDS[] = { 0.12f, 0.1f, 0.15f }; // 単押し：ため・殴り・休み
static constexpr float RUSH_SECONDS[] = { 0.3f, 0.35f, 0.22f }; // 長押し：ため・突進・休み
static constexpr float PUNCH_CLIP_SECONDS[] = { 4.0f / 24.0f, 3.0f / 24.0f, 5.0f / 24.0f }; // 単押しのクリップの区切り（knuckle_punch）
static constexpr float RUSH_CLIP_SECONDS[] = { 10.0f / 24.0f, 8.0f / 24.0f, 8.0f / 24.0f }; // 長押しのクリップの区切り（knuckle_rush）
static constexpr float PUNCH_REACH = 1.6f; // 肩から殴りの届く距離。
static constexpr float PUNCH_RADIUS = 0.4f;
static constexpr float PUNCH_DAMAGE = 10.0f;
static constexpr float PUNCH_POWER = 2.0f;
static constexpr float PUNCH_UP = 0.6f; // 水平の単位方向に足す上向きの量。
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
	m_AirRushClip = false;
	m_Straighten = 0.0f;
	m_Extend = 0.0f;
}

int KnuckleArm::FindTarget(float reach, float radius) const
{
	TENTACLE_TARGET targets[FIGHTER_COUNT];
	for (int i = 0; i < FIGHTER_COUNT; ++i) targets[i] = GetFighters().GetTarget(i, GetSlot());
	// 斜め上への空中の突進は、進む向きの先で当たりを探す（上にいる相手にも当たる）。
	const VEC3 direction = m_AirRush ? ArmCalcPitchDirection(m_Direction, m_AirRushPitch) : m_Direction;
	return AreaCalcFirstOnRay(GetShoulder(), direction, reach, radius, targets, FIGHTER_COUNT, nullptr);
}

// 浮島の外にいるか、浮島の上面より下にいるなら場外とみなす。
bool KnuckleArm::IsOffStage() const
{
	const XMFLOAT3 feet = GetPlayer().GetPosition();
	const STAGE& stage = GetStage().GetStage();
	return feet.y < stage.TopY || !StageCalcIsOnPlatform(stage, feet.x, feet.z);
}

// 空中の突進で上へ傾ける角度。0 なら地上と同じ水平の突進。場外は40°、フリーカメラはカメラが上を向いた分だけ上へ。
float KnuckleArm::GetAirRushPitch() const
{
	if (GetPlayer().IsOnGround() || GetPlayer().IsPulled()) return 0.0f;
	return ArmCalcAirRushPitch(IsOffStage(), GetCamera().IsFreeLook(), GetCamera().GetPitch(), AIR_RUSH_PITCH);
}

void KnuckleArm::Strike(int target, float up, float damage, float power)
{
	// 1動作で1回だけ減らす（同じ動作でもう減らしていたら重ねない）。
	if (GetFighters().Launch(target, GetSlot(), AreaCalcLaunchDirection(m_Direction, up), damage, power) && !m_UseConsumed) {
		ConsumeDurability(GetCost(COST_ROW, m_Rush ? ARM_COST_HOLD_HIT : ARM_COST_TAP_HIT));
		m_UseConsumed = true;
	}
	Rumble(HIT_RUMBLE_SMALL, HIT_RUMBLE_LARGE, HIT_RUMBLE_SECONDS);
}

void KnuckleArm::Update(const ARM_INPUT& input)
{
	const float dt = fmaxf(0.0f, GameTimeGetDeltaTime());
	if (GetPlayer().DidRespawn()) Reset();
	// 長押しは押している間ためた姿勢で待ち、離した瞬間に突進する。
	if (m_Phase.IsIdle() && (input.Tap || input.HoldStarted || input.HoldReleased)) {
		m_Rush = !input.Tap;
		// 判定で使う m_AirRush と分け、ため中に場外へ出入りしてもクリップは切り替えない。
		m_AirRushClip = m_Rush && GetAirRushPitch() > 0.0f;
		m_UseConsumed = false;
		m_Direction = GetAim(false);
		m_Phase.Start(KNUCKLE_CHARGE);
		m_Phase.SetHeld(m_Rush);
		if (input.HoldReleased) m_Phase.Release();
	}
	else if (input.HoldReleased && m_Rush) m_Phase.Release();
	// ためている間にボタンを受け付けなくなった（ガードなど）ときは、出さずにやめる。
	else if (!input.Pressed && m_Phase.IsHeld()) m_Phase.Reset();
	const ARM_PHASE_STEP step = m_Phase.Step(dt, m_Rush ? RUSH_SECONDS : PUNCH_SECONDS, KNUCKLE_PHASE_COUNT);
	if (step.Entered == KNUCKLE_STRIKE) {
		m_Direction = GetAim(false);
		if (!m_Rush) {
			const int target = FindTarget(PUNCH_REACH, PUNCH_RADIUS);
			if (target >= 0) Strike(target, PUNCH_UP, PUNCH_DAMAGE, PUNCH_POWER);
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
		const int target = FindTarget(RUSH_REACH, RUSH_RADIUS);
		if (target >= 0) {
			Strike(target, RUSH_UP, RUSH_DAMAGE, RUSH_POWER);
			m_Phase.Start(KNUCKLE_RECOVER); // 当たったら止まる。
			StopAirRush();
		}
	}
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
	clip.Kind = m_Rush ? (m_AirRushClip ? ATTACK_CLIP::KnuckleAirRush : ATTACK_CLIP::KnuckleRush) : ATTACK_CLIP::KnucklePunch;
	clip.FullBody = m_Rush;
	clip.Time = ArmCalcClipTime(m_Phase.GetPhase(), m_Phase.GetTime(), m_Rush ? RUSH_SECONDS : PUNCH_SECONDS, m_Rush ? RUSH_CLIP_SECONDS : PUNCH_CLIP_SECONDS);
	return clip;
}

bool KnuckleArm::IsHolding() const
{
	return false;
}

float KnuckleArm::GetMoveScale() const
{
	return m_Rush && !m_Phase.IsIdle() ? CHARGE_MOVE_SCALE : 1.0f;
}

VEC3 KnuckleArm::GetRushVelocity() const
{
	if (!m_Rush || m_AirRush || m_Phase.GetPhase() != KNUCKLE_STRIKE) return {};
	return Vec3Scale(m_Direction, RUSH_SPEED * TuningCalcValue(GetTuning(), TUNING_RUSH_DISTANCE));
}
