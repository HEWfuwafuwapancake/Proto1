#include "game/mapgimmick.h"
#include "core/gametime.h"
#include "game/arm.h"
#include "game/fightermanager.h"
#include "game/stage.h"
#include "render/shape.h"
#include <math.h>

//------------------------------------------------------------------------------
//	調整用の値（見た目だけ。動きの値は physics/liftcalc.cpp と physics/mapcalc.cpp）
//------------------------------------------------------------------------------
static constexpr float MARK_RADIUS = 0.5f;			// 地上のＧの印の半径（直径1.0）
static constexpr float MARK_HEIGHT = 0.03f;			// 地上のＧの印の厚さ
static constexpr XMFLOAT4 POST_RING_COLOR = { 0.55f, 0.60f, 0.66f, 1.0f };	// 郵便局の二重環（金属）
static constexpr XMFLOAT4 POST_RING_INNER = { 0.20f, 0.24f, 0.28f, 1.0f };	// 二重環のすき間
static constexpr XMFLOAT4 MARKET_KNOT_COLOR = { 0.98f, 0.98f, 0.96f, 1.0f };	// 市場の救助ロープの白い結び目
static constexpr XMFLOAT4 LIT_COLOR = { 0.78f, 0.96f, 0.20f, 1.0f };		// 起動できるＧの点灯（黄緑）
static constexpr XMFLOAT4 ROPE_COLOR = { 1.0f, 0.36f, 0.70f, 1.0f };		// 補助触腕
static constexpr XMFLOAT4 CAN_COLOR = { 0.72f, 0.74f, 0.78f, 1.0f };		// 空き缶
static constexpr float DIAMOND_SIZE = 0.35f;		// 着地点の菱形の大きさ
static constexpr float DIAMOND_LIFT = 0.9f;			// 着地点の菱形を浮かせる高さ
static constexpr float ROPE_RADIUS = 0.08f;			// 補助触腕の太さ
static constexpr float ROPE_HAND_HEIGHT = 1.4f;		// 補助触腕を体から出す高さ
static constexpr float CAN_RADIUS = 0.18f;			// 空き缶の半径
static constexpr float CAN_HEIGHT = 0.45f;			// 空き缶の高さ

void MapGimmick::Initialize(const Stage& stage, FighterManager& fighters, const ArmManager* arms)
{
	m_Stage = &stage;
	m_Fighters = &fighters;
	m_Arms = arms;
	Reset();
}

void MapGimmick::Reset()
{
	m_Lift.Reset();
	m_Cans.Reset();
	m_Frame = 0;
	for (bool& ready : m_BeltReady) ready = false;
}

bool MapGimmick::CanStartLift(int slot) const
{
	// 接地して、待機か通常の歩きのときだけ（攻撃・硬直・つかまれ・ガード・ダッシュ・吹っ飛び中は使えない）。
	const Fighter& fighter = m_Fighters->GetFighter(slot);
	return fighter.IsHuman() && fighter.IsActive() && fighter.IsOnGround() && !fighter.IsLaunched() && !fighter.IsHeld()
		&& !fighter.IsPulled() && fighter.CanUseArms() && !fighter.IsDashing() && !m_Arms[slot].IsActive()
		&& !fighter.IsPress(ACTION_GUARD_LEFT) && !fighter.IsPress(ACTION_GUARD_RIGHT);
}

int MapGimmick::PreUpdate()
{
	const STAGE& stage = m_Stage->GetStage();
	LIFT_FIGHTER fighters[FIGHTER_COUNT];
	for (int slot = 0; slot < FIGHTER_COUNT; ++slot) {
		const Fighter& fighter = m_Fighters->GetFighter(slot);
		const XMFLOAT3 p = fighter.GetPosition();
		fighters[slot] = { { p.x, p.y, p.z }, fighter.IsActive(),
			stage.LiftCount > 0 && fighter.IsHuman() && fighter.IsTrigger(ACTION_LIFT), CanStartLift(slot) };
	}
	const int started = m_Lift.Accept(stage, m_Frame, fighters);
	int masked = 0;
	for (int slot = 0; slot < FIGHTER_COUNT; ++slot) {
		Fighter& fighter = m_Fighters->GetFighter(slot);
		if (started & (1 << slot)) fighter.SetPulled(true); // 体の通常の動きを止める（被弾で外れる）
		if (m_Lift.IsMoving(slot)) masked |= 1 << slot;
		// ベルトは、更新の最初に立っていて、その回に跳ばない・ダッシュしない人だけを運ぶ。
		m_BeltReady[slot] = stage.BeltCount > 0 && !m_Lift.IsMoving(slot) && fighter.IsActive() && fighter.IsOnGround()
			&& !fighter.IsLaunched() && !fighter.IsHeld() && !fighter.IsPulled() && !fighter.IsDashing()
			&& !fighter.IsTrigger(ACTION_JUMP) && !fighter.IsTrigger(ACTION_DASH);
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

	// 荷送りベルト：基準の床のベルトの上なら、Z の中央向きに運んでから側面で止める（仕分け台に当たれば止まる）。
	const float dt = GameTimeGetDeltaTime();
	for (int slot = 0; slot < FIGHTER_COUNT; ++slot) {
		Fighter& fighter = m_Fighters->GetFighter(slot);
		if (!m_BeltReady[slot] || !fighter.IsOnGround() || fighter.IsPulled()) continue;
		const XMFLOAT3 p = fighter.GetPosition();
		const int region = StageCalcFindRegion(stage, p.x, p.z);
		if (region < 0 || stage.Regions[region].Surface != STAGE_SURFACE_FLOOR) continue;
		for (int i = 0; i < stage.BeltCount; ++i) {
			const STAGE_BELT& belt = stage.Belts[i];
			if (p.x < belt.MinX || p.x > belt.MaxX || p.z < belt.MinZ || p.z > belt.MaxZ) continue;
			const float dz = (p.z > 0.0f ? -1.0f : 1.0f) * stage.BeltSpeed * dt;
			float x = p.x, z = p.z, y = p.y;
			StageCalcMoveBody(stage, p.x, p.y, p.z, 0.0f, dz, true, x, z);
			float ground = y;
			if (StageCalcFollowGround(stage, x, y, z, ground)) y = ground;
			fighter.SetPosition({ x, y, z });
			break;
		}
	}

	// 南の橋の空き缶：だれかが新しく橋の上面に立ったら1回鳴って揺れる。
	if (stage.HasCans) {
		bool onBridge[FIGHTER_COUNT] = {};
		for (int slot = 0; slot < FIGHTER_COUNT; ++slot) {
			const Fighter& fighter = m_Fighters->GetFighter(slot);
			if (!fighter.IsActive() || !fighter.IsOnGround()) continue;
			const XMFLOAT3 p = fighter.GetPosition();
			const int region = StageCalcFindRegion(stage, p.x, p.z);
			onBridge[slot] = region >= 0 && stage.Regions[region].Surface == STAGE_SURFACE_SHUTTER
				&& StageCalcHasSupport(stage, p.x, p.y, p.z);
		}
		m_Cans.Step(m_Frame, onBridge, FIGHTER_COUNT);
	}
	++m_Frame;
}

void MapGimmick::DrawLift(int index, bool lit) const
{
	const STAGE& stage = m_Stage->GetStage();
	const STAGE_LIFT& lift = stage.Lifts[index];
	const bool post = m_Stage->GetMap() == STAGE_MAP_POST_OFFICE;
	const XMFLOAT3 base{ lift.StartX, 0.0f, lift.StartZ };
	const XMFLOAT3 top{ lift.StartX, MARK_HEIGHT, lift.StartZ };
	const XMFLOAT3 over{ lift.StartX, MARK_HEIGHT * 2.0f, lift.StartZ };
	const XMFLOAT3 overTop{ lift.StartX, MARK_HEIGHT * 3.0f, lift.StartZ };
	if (post) {
		// 金属の二重環。
		ShapeDrawCylinder(base, top, MARK_RADIUS, lit ? LIT_COLOR : POST_RING_COLOR);
		ShapeDrawCylinder(top, over, MARK_RADIUS * 0.72f, POST_RING_INNER);
		ShapeDrawCylinder(over, overTop, MARK_RADIUS * 0.5f, lit ? LIT_COLOR : POST_RING_COLOR);
	}
	else {
		// 救助ロープの白い結び目。
		ShapeDrawCylinder(base, top, MARK_RADIUS, lit ? LIT_COLOR : MARKET_KNOT_COLOR);
		ShapeDrawSphere({ lift.StartX, MARK_HEIGHT + 0.15f, lift.StartZ }, 0.2f, MARKET_KNOT_COLOR);
	}
	if (!lit) return;
	// 点灯中は着地点に菱形を出す。
	const VEC3& land = lift.Landing;
	ShapeDrawBoxAlong({ land.X, land.Y + DIAMOND_LIFT - DIAMOND_SIZE, land.Z }, { land.X, land.Y + DIAMOND_LIFT + DIAMOND_SIZE, land.Z },
		DIAMOND_SIZE, DIAMOND_SIZE, LIT_COLOR);
}

void MapGimmick::Draw(int viewer) const
{
	const STAGE& stage = m_Stage->GetStage();
	// 見ている人が今押せば起動できるＧ（始点の近くで、再使用の時刻を過ぎていて、起動できる状態）。
	int litStation = -1;
	if (viewer >= 0 && viewer < FIGHTER_COUNT && !m_Lift.IsMoving(viewer) && m_Lift.IsReady(viewer, m_Frame) && CanStartLift(viewer)) {
		const XMFLOAT3 p = m_Fighters->GetFighter(viewer).GetPosition();
		float height = 0.0f;
		if (StageCalcGroundHeight(stage, p.x, p.z, height) && fabsf(height) < 0.01f) litStation = LiftCalc::FindStation(stage, { p.x, p.y, p.z });
	}
	for (int i = 0; i < stage.LiftCount; ++i) DrawLift(i, i == litStation);

	// Ｇで動いている人は、体から着地点へ補助触腕を伸ばす。
	for (int slot = 0; slot < FIGHTER_COUNT; ++slot) {
		if (!m_Lift.IsMoving(slot)) continue;
		const XMFLOAT3 p = m_Fighters->GetFighter(slot).GetPosition();
		const VEC3 target = m_Lift.GetTarget(slot);
		ShapeDrawCylinder({ p.x, p.y + ROPE_HAND_HEIGHT, p.z }, { target.X, target.Y, target.Z }, ROPE_RADIUS, ROPE_COLOR);
	}

	if (!stage.HasCans) return;
	// 空き缶は鳴ったあと、X 軸のまわりに揺れる。
	const float tilt = m_Cans.GetTilt(m_Frame);
	for (const VEC3& can : stage.Cans) {
		const XMFLOAT3 bottom{ can.X, can.Y, can.Z };
		const XMFLOAT3 top{ can.X, can.Y + cosf(tilt) * CAN_HEIGHT, can.Z + sinf(tilt) * CAN_HEIGHT };
		ShapeDrawCylinder(bottom, top, CAN_RADIUS, CAN_COLOR);
	}
}
