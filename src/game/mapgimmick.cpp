#include "game/mapgimmick.h"
#include "game/arm.h"
#include "game/fightermanager.h"
#include "game/stage.h"
#include "audio/se.h"
#include "physics/areacalc.h"
#include "render/shape.h"
#include <math.h>

//------------------------------------------------------------------------------
//	調整用の値（Ｇの動きは physics/liftcalc.cpp、柱の倒れ方は physics/pillarcalc.cpp）
//------------------------------------------------------------------------------
static constexpr float BOOST_SPEED = 20.0f;			// ダッシュ板が中央へ飛ばす水平の速さ（1秒に進む距離）
static constexpr float BOOST_UP_SPEED = 9.0f;		// ダッシュ板が飛ばす上向きの速さ
static constexpr float PAD_RISE_HEIGHT = 21.0f;		// ジャンプ台で、乗った高さから上がる高さ（屋上へ届く）
static constexpr float PAD_REACH = 0.3f;			// ジャンプ台の上面から、足がこれ以内なら乗っている
static constexpr int PAD_COOLDOWN_FRAMES = 48;		// 同じ人がまたジャンプ台で打ち上げられるまでの更新の数（0.8秒）
static constexpr float PILLAR_DAMAGE = 18.0f;		// 倒れる柱が当たったときのダメージ%
static constexpr float PILLAR_POWER = 2.5f;			// 倒れる柱が当たったときの吹っ飛ばす強さ
static constexpr float PILLAR_UP = 0.42f;			// 倒れる柱が吹っ飛ばす向き（倒れる向きに足す上向きの量）
static constexpr int PILLAR_IMPACT_MAX = PILLAR_MAX * FIGHTER_COUNT;	// 1回の更新で柱が体に当たる最大数
static constexpr float MARK_RADIUS = 0.5f;			// 地上のＧの印の半径（直径1.0）
static constexpr float MARK_HEIGHT = 0.03f;			// 地上のＧの印の厚さ
static constexpr XMFLOAT4 RING_COLOR = { 1.0f, 0.36f, 0.70f, 1.0f };		// Ｇの輪（触手と同じピンク）
static constexpr XMFLOAT4 RING_INNER = { 0.18f, 0.08f, 0.32f, 1.0f };		// 輪のすき間（濃い紫）
static constexpr XMFLOAT4 LIT_COLOR = { 0.78f, 0.96f, 0.20f, 1.0f };		// 起動できるＧの点灯（黄緑）
static constexpr XMFLOAT4 ROPE_COLOR = { 1.0f, 0.36f, 0.70f, 1.0f };		// Ｇで伸ばす触手
static constexpr float DIAMOND_SIZE = 0.35f;		// 着地点の菱形の大きさ
static constexpr float DIAMOND_LIFT = 0.9f;			// 着地点の菱形を浮かせる高さ
static constexpr float ROPE_RADIUS = 0.1f;			// Ｇで伸ばす触手の太さ
static constexpr float ROPE_HAND_HEIGHT = 1.4f;		// 触手を体から出す高さ

void MapGimmick::Initialize(Stage& stage, FighterManager& fighters, const ArmManager* arms)
{
	m_Stage = &stage;
	m_Fighters = &fighters;
	m_Arms = arms;
	Reset();
}

void MapGimmick::Reset()
{
	m_Lift.Reset();
	m_Frame = 0;
	for (bool& on : m_OnBelt) on = false;
	for (uint32_t& held : m_HeldButtons) held = 0;
	for (int& frame : m_PadReadyFrame) frame = 0;
	for (PILLAR_STATE& state : m_PillarStates) state = PILLAR_STANDING;
}

int MapGimmick::GetTentacleHand(int slot) const
{
	// Ｇは触手でしか使えない。左手・右手の順に、触手を持っていて使える手を探す。
	const ArmManager& arms = m_Arms[slot];
	for (int hand = 0; hand < 2; ++hand) {
		if (arms.GetType(hand) == ARM_TYPE_TENTACLE && arms.CanUseArm(hand)) return hand;
	}
	return -1;
}

bool MapGimmick::CanStartLift(int slot) const
{
	// 接地して、待機か通常の歩きのときだけ（攻撃・硬直・つかまれ・ガード・ダッシュ・吹っ飛び中は使えない）。
	const Fighter& fighter = m_Fighters->GetFighter(slot);
	return fighter.IsHuman() && fighter.IsActive() && fighter.IsOnGround() && !fighter.IsLaunched() && !fighter.IsHeld()
		&& !fighter.IsPulled() && fighter.CanUseArms() && !fighter.IsDashing() && !m_Arms[slot].IsActive()
		&& !fighter.IsPress(ACTION_GUARD_LEFT) && !fighter.IsPress(ACTION_GUARD_RIGHT) && GetTentacleHand(slot) >= 0;
}

int MapGimmick::PreUpdate()
{
	const STAGE& stage = m_Stage->GetStage();
	LIFT_FIGHTER fighters[FIGHTER_COUNT];
	uint32_t requestButtons[FIGHTER_COUNT] = {};
	for (int slot = 0; slot < FIGHTER_COUNT; ++slot) {
		const Fighter& fighter = m_Fighters->GetFighter(slot);
		const XMFLOAT3 p = fighter.GetPosition();
		// 触手を持つ手のボタンを押した瞬間が、Ｇの申し込み（Ｇの始点の近くでなければ、ふつうの触手になる）。
		bool request = false;
		const int hand = stage.LiftCount > 0 && fighter.IsHuman() ? GetTentacleHand(slot) : -1;
		if (hand >= 0) {
			const ACTION button = hand == 0 ? ACTION_ARM_LEFT : ACTION_ARM_RIGHT;
			request = fighter.IsTrigger(button);
			requestButtons[slot] = 1u << button;
		}
		fighters[slot] = { { p.x, p.y, p.z }, fighter.IsActive(), request, CanStartLift(slot) };
		// 使ったボタンは、離すまで腕に渡さない（着地したあとに触手が出ないように）。
		if (!m_Lift.IsMoving(slot)) {
			if (!fighter.IsPress(ACTION_ARM_LEFT)) m_HeldButtons[slot] &= ~(1u << ACTION_ARM_LEFT);
			if (!fighter.IsPress(ACTION_ARM_RIGHT)) m_HeldButtons[slot] &= ~(1u << ACTION_ARM_RIGHT);
		}
	}
	const int started = m_Lift.Accept(stage, m_Frame, fighters);
	int masked = 0;
	for (int slot = 0; slot < FIGHTER_COUNT; ++slot) {
		if (started & (1 << slot)) {
			m_Fighters->GetFighter(slot).SetPulled(true); // 体の通常の動きを止める（被弾で外れる）
			m_HeldButtons[slot] |= requestButtons[slot];
		}
		if (m_Lift.IsMoving(slot)) masked |= 1 << slot;
	}
	return masked;
}

void MapGimmick::PostUpdate()
{
	const STAGE& stage = m_Stage->GetStage();
	LIFT_FIGHTER fighters[FIGHTER_COUNT];
	for (int slot = 0; slot < FIGHTER_COUNT; ++slot) {
		Fighter& fighter = m_Fighters->GetFighter(slot);
		const XMFLOAT3 p = fighter.GetPosition();
		fighters[slot] = { { p.x, p.y, p.z }, fighter.IsActive(), false, false };
		// 被弾・つかまれ（体の引っぱられ中が外れた）なら、その位置で止めて予約を消し、通常の処理へ戻す。
		if (m_Lift.IsMoving(slot) && (!fighter.IsPulled() || fighter.IsHeld() || !fighter.IsActive())) {
			m_Lift.Cancel(slot);
			if (fighter.IsPulled()) fighter.SetPulled(false);
		}
	}
	m_Lift.Step(stage, fighters);
	for (int slot = 0; slot < FIGHTER_COUNT; ++slot) {
		Fighter& fighter = m_Fighters->GetFighter(slot);
		if (m_Lift.IsMoving(slot)) {
			const VEC3 feet = m_Lift.GetFeet(slot);
			fighter.SetPosition({ feet.X, feet.Y, feet.Z });
		}
		else if (m_Lift.DidLand(slot)) {
			fighter.SetPulled(false);
			fighter.PlaceOnGround(m_Lift.GetFeet(slot));
		}
		else if (m_Lift.DidDrop(slot)) {
			fighter.SetPulled(false);
			const VEC3 feet = m_Lift.GetFeet(slot);
			fighter.SetPosition({ feet.X, feet.Y, feet.Z });
		}
	}

	UpdateBelts();
	UpdatePads();
	UpdatePillars();
	++m_Frame;
}

void MapGimmick::UpdateBelts()
{
	// ダッシュ板：基準の床の板の上に立って乗った瞬間、Z の中央向きに移動として吹っ飛ばす（ダメージはない）。
	const STAGE& stage = m_Stage->GetStage();
	if (stage.BeltCount == 0) return;
	for (int slot = 0; slot < FIGHTER_COUNT; ++slot) {
		Fighter& fighter = m_Fighters->GetFighter(slot);
		bool onBelt = false;
		const XMFLOAT3 p = fighter.GetPosition();
		if (fighter.IsActive() && fighter.IsOnGround() && !fighter.IsHeld() && !fighter.IsPulled()) {
			const int region = StageCalcFindFloor(stage, p.x, p.y, p.z);
			for (int i = 0; i < stage.BeltCount && region >= 0 && stage.Regions[region].Surface == STAGE_SURFACE_FLOOR; ++i) {
				const STAGE_BELT& belt = stage.Belts[i];
				if (p.x >= belt.MinX && p.x <= belt.MaxX && p.z >= belt.MinZ && p.z <= belt.MaxZ) onBelt = true;
			}
		}
		if (onBelt && !m_OnBelt[slot]) fighter.Boost({ 0.0f, BOOST_UP_SPEED, (p.z > 0.0f ? -1.0f : 1.0f) * BOOST_SPEED });
		m_OnBelt[slot] = onBelt;
	}
}

void MapGimmick::UpdatePads()
{
	// ジャンプ台：立って乗っていたら、横の速さを消して真上へ打ち上げる（空中で通っただけでは動かない。移動なのでダメージはない）。
	const STAGE& stage = m_Stage->GetStage();
	const float upSpeed = sqrtf(2.0f * Fighter::GetBoostGravity() * PAD_RISE_HEIGHT);
	for (int slot = 0; slot < FIGHTER_COUNT; ++slot) {
		Fighter& fighter = m_Fighters->GetFighter(slot);
		if (m_Frame < m_PadReadyFrame[slot] || !fighter.IsActive() || !fighter.IsOnGround() || fighter.IsHeld() || fighter.IsPulled()) continue;
		const XMFLOAT3 p = fighter.GetPosition();
		const int region = StageCalcFindFloor(stage, p.x, p.y, p.z);
		if (region < 0 || stage.Regions[region].Surface != STAGE_SURFACE_PAD
			|| fabsf(p.y - StageCalcRegionHeight(stage.Regions[region], p.x, p.z)) > PAD_REACH) continue;
		fighter.Boost({ 0.0f, upSpeed, 0.0f });
		m_PadReadyFrame[slot] = m_Frame + PAD_COOLDOWN_FRAMES;
		SePlayAt(SE_KATANA_DASH, { p.x, p.y, p.z }, slot);
	}
}

void MapGimmick::UpdatePillars()
{
	const PillarCalc& pillars = m_Stage->GetPillars();
	if (pillars.GetCount() == 0) return;
	// 倒れる柱は、攻撃した本人も含めて全員に当たる（無敵・つかまれ中・復活待ちには当たらない）。
	CAPSULE bodies[FIGHTER_COUNT];
	bool hittable[FIGHTER_COUNT];
	for (int slot = 0; slot < FIGHTER_COUNT; ++slot) {
		const Fighter& fighter = m_Fighters->GetFighter(slot);
		bodies[slot] = fighter.GetBody();
		hittable[slot] = fighter.IsActive() && !fighter.IsInvincible() && !fighter.IsHeld();
	}
	PILLAR_IMPACT impacts[PILLAR_IMPACT_MAX];
	const int count = m_Stage->StepPillars(bodies, hittable, impacts, PILLAR_IMPACT_MAX);
	for (int i = 0; i < count; ++i) {
		const PILLAR_IMPACT& impact = impacts[i];
		const VEC3 launch = AreaCalcLaunchDirection(impact.Direction, PILLAR_UP);
		// 柱を倒した人がほかの人に当てたら、その人の攻撃として数える（撃墜の記録・ガード）。本人と、倒した人がいないときは地形の攻撃。
		const bool byAttacker = impact.Attacker >= 0 && impact.Attacker != impact.Victim && m_Fighters->IsActive(impact.Attacker);
		if (byAttacker) m_Fighters->Launch(impact.Victim, impact.Attacker, launch, PILLAR_DAMAGE, PILLAR_POWER);
		else m_Fighters->GetFighter(impact.Victim).Launch(launch, PILLAR_DAMAGE, PILLAR_POWER);
		const XMFLOAT3 p = m_Fighters->GetFighter(impact.Victim).GetPosition();
		SePlayAt(SE_HAMMER_TAP, { p.x, p.y, p.z }, impact.Attacker, impact.Victim);
	}
	// 耐久が尽きたとき（ひびの音）と、倒れ終わったとき（地響き）に音を出す。
	for (int i = 0; i < pillars.GetCount(); ++i) {
		const PILLAR_STATE state = pillars.GetState(i);
		if (state == m_PillarStates[i]) continue;
		const PILLAR_POSE pose = pillars.GetPose(i);
		if (state == PILLAR_WARNING) SePlayAt(SE_ARM_BREAK, pose.Base, -1);
		if (state == PILLAR_FALLEN) SePlayAt(SE_HAMMER_HOLD, pose.Base, -1);
		m_PillarStates[i] = state;
	}
}

void MapGimmick::DrawLift(int index, bool lit) const
{
	const STAGE& stage = m_Stage->GetStage();
	const STAGE_LIFT& lift = stage.Lifts[index];
	const XMFLOAT3 base{ lift.StartX, 0.0f, lift.StartZ };
	const XMFLOAT3 top{ lift.StartX, MARK_HEIGHT, lift.StartZ };
	const XMFLOAT3 over{ lift.StartX, MARK_HEIGHT * 2.0f, lift.StartZ };
	const XMFLOAT3 overTop{ lift.StartX, MARK_HEIGHT * 3.0f, lift.StartZ };
	// 二重の輪（触手で使う場所なので触手のピンク。点灯中は黄緑）。
	ShapeDrawCylinder(base, top, MARK_RADIUS, lit ? LIT_COLOR : RING_COLOR);
	ShapeDrawCylinder(top, over, MARK_RADIUS * 0.72f, RING_INNER);
	ShapeDrawCylinder(over, overTop, MARK_RADIUS * 0.5f, lit ? LIT_COLOR : RING_COLOR);
	if (!lit) return;
	// 点灯中は着地点に菱形を出す。
	const VEC3& land = lift.Landing;
	ShapeDrawBoxAlong({ land.X, land.Y + DIAMOND_LIFT - DIAMOND_SIZE, land.Z }, { land.X, land.Y + DIAMOND_LIFT + DIAMOND_SIZE, land.Z },
		DIAMOND_SIZE, DIAMOND_SIZE, LIT_COLOR);
}

void MapGimmick::Draw(int viewer) const
{
	const STAGE& stage = m_Stage->GetStage();
	// 見ている人が今触手を押せば起動できるＧ（始点の近くで、再使用の時刻を過ぎていて、触手を持って起動できる状態）。
	int litStation = -1;
	if (viewer >= 0 && viewer < FIGHTER_COUNT && !m_Lift.IsMoving(viewer) && m_Lift.IsReady(viewer, m_Frame) && CanStartLift(viewer)) {
		const XMFLOAT3 p = m_Fighters->GetFighter(viewer).GetPosition();
		float height = 0.0f;
		if (StageCalcGroundHeight(stage, p.x, p.z, height) && fabsf(height) < 0.01f) litStation = LiftCalc::FindStation(stage, { p.x, p.y, p.z });
	}
	for (int i = 0; i < stage.LiftCount; ++i) DrawLift(i, i == litStation);

	// Ｇで動いている人は、体から着地点へ触手を伸ばす。
	for (int slot = 0; slot < FIGHTER_COUNT; ++slot) {
		if (!m_Lift.IsMoving(slot)) continue;
		const XMFLOAT3 p = m_Fighters->GetFighter(slot).GetPosition();
		const VEC3 target = m_Lift.GetTarget(slot);
		ShapeDrawCylinder({ p.x, p.y + ROPE_HAND_HEIGHT, p.z }, { target.X, target.Y, target.Z }, ROPE_RADIUS, ROPE_COLOR);
	}
}
