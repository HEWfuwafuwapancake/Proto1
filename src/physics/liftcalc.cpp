#include <math.h>
#include "physics/liftcalc.h"

//------------------------------------------------------------------------------
//	調整用の値（設計書 第10節）
//------------------------------------------------------------------------------
static constexpr float HOVER_HEIGHT = 0.6f;		// 着地点の上面より、移動中に浮かせる高さ
static constexpr float SHIFT = 1.05f;			// 着地点がふさがれたときにずらす距離
static constexpr float EDGE_INSET = 0.5f;		// 着地点は目標の面のふちからこれ以上内側
static constexpr float CLEARANCE = 1.0f;		// 体どうしの水平の距離がこれより近いとふさがる
static constexpr float SURFACE_TOLERANCE = 0.01f;	// 同じ面とみなす高さの差
static constexpr float BASE_FLOOR_Y = 0.0f;		// Ｇを使える基準の床の高さ

void LiftCalc::Reset()
{
	*this = LiftCalc{};
}

int LiftCalc::FindStation(const STAGE& stage, const VEC3& feet)
{
	for (int i = 0; i < stage.LiftCount; ++i) {
		const float dx = feet.X - stage.Lifts[i].StartX;
		const float dz = feet.Z - stage.Lifts[i].StartZ;
		if (sqrtf(dx * dx + dz * dz) <= LIFT_ACCEPT_RADIUS) return i;
	}
	return -1;
}

bool LiftCalc::IsOccupied(const VEC3& point, int slot, const LIFT_FIGHTER* fighters) const
{
	// 体の高さの区間 [H, H＋高さ] が内部で重なり、水平の距離が CLEARANCE より近ければふさがっている。予約も同じ大きさ。
	const auto blocks = [&](const VEC3& other) {
		const float dx = other.X - point.X, dz = other.Z - point.Z;
		return fabsf(other.Y - point.Y) < STAGE_BODY_HEIGHT && sqrtf(dx * dx + dz * dz) < CLEARANCE;
	};
	for (int i = 0; i < FIGHTER_COUNT; ++i) {
		if (i == slot) continue;
		if (fighters[i].Present && blocks(fighters[i].Feet)) return true;
		if (m_Moving[i] && blocks(m_Target[i])) return true;
	}
	return false;
}

bool LiftCalc::ChooseLanding(const STAGE& stage, int station, int slot, const LIFT_FIGHTER* fighters, VEC3& landing) const
{
	const VEC3 nominal = stage.Lifts[station].Landing;
	const float side = stage.LiftSideSign * SHIFT * (nominal.X < 0.0f ? -1.0f : 1.0f);
	const VEC3 candidates[4] = {
		nominal,
		{ nominal.X + side, nominal.Y, nominal.Z },
		{ nominal.X, nominal.Y, nominal.Z + SHIFT },
		{ nominal.X, nominal.Y, nominal.Z - SHIFT },
	};
	for (const VEC3& point : candidates) {
		// 目標の面のふちから EDGE_INSET 以上内側（前後左右にずらしても同じ高さの床がある）。
		bool inside = true;
		const float probes[5][2] = { { 0, 0 }, { EDGE_INSET, 0 }, { -EDGE_INSET, 0 }, { 0, EDGE_INSET }, { 0, -EDGE_INSET } };
		for (const auto& probe : probes) {
			float height = 0.0f;
			if (!StageCalcGroundHeight(stage, point.X + probe[0], point.Z + probe[1], height)
				|| fabsf(height - nominal.Y) > SURFACE_TOLERANCE) {
				inside = false;
				break;
			}
		}
		if (!inside || IsOccupied(point, slot, fighters)) continue;
		landing = point;
		return true;
	}
	return false;
}

int LiftCalc::Accept(const STAGE& stage, int frame, const LIFT_FIGHTER* fighters)
{
	int started = 0;
	// 同時に押した人はスロットの順に割り当てる（先に受け付けた人の予約は、あとの人の着地点をふさぐ）。
	for (int slot = 0; slot < FIGHTER_COUNT; ++slot) {
		const LIFT_FIGHTER& fighter = fighters[slot];
		if (!fighter.Request || !fighter.Present || !fighter.Eligible || m_Moving[slot] || !IsReady(slot, frame)) continue;
		// 基準の床（高さ 0）に立っているときだけ。
		float height = 0.0f;
		if (!StageCalcGroundHeight(stage, fighter.Feet.X, fighter.Feet.Z, height)
			|| fabsf(height - BASE_FLOOR_Y) > SURFACE_TOLERANCE || fabsf(fighter.Feet.Y - height) > SURFACE_TOLERANCE) continue;
		const int station = FindStation(stage, fighter.Feet);
		if (station < 0) continue;
		VEC3 landing{};
		if (!ChooseLanding(stage, station, slot, fighters, landing)) continue; // 不発。再使用までの時間も使わない
		m_Moving[slot] = true;
		m_Step[slot] = 0;
		m_Station[slot] = station;
		m_Start[slot] = fighter.Feet;
		m_Target[slot] = landing;
		m_Feet[slot] = fighter.Feet;
		m_ReadyFrame[slot] = frame + LIFT_COOLDOWN_FRAMES;
		started |= 1 << slot;
	}
	return started;
}

VEC3 LiftCalc::PositionAt(int slot, int step) const
{
	const VEC3& start = m_Start[slot];
	const VEC3& target = m_Target[slot];
	const float hover = target.Y + HOVER_HEIGHT;
	if (step <= LIFT_RISE_FRAMES) {
		// n = 1〜9：始点の真上へ、まっすぐ等速で浮く。
		return { start.X, start.Y + (hover - start.Y) * step / LIFT_RISE_FRAMES, start.Z };
	}
	if (step <= LIFT_TRAVEL_END) {
		// n = 10〜24：浮いた高さのまま、着地点の真上へ15等分で移る。
		const float t = (float)(step - LIFT_RISE_FRAMES) / (LIFT_TRAVEL_END - LIFT_RISE_FRAMES);
		return { start.X + (target.X - start.X) * t, hover, start.Z + (target.Z - start.Z) * t };
	}
	// n = 25〜27：着地点の上で3等分して降りる。
	const float t = (float)(step - LIFT_TRAVEL_END) / (LIFT_TOTAL_FRAMES - LIFT_TRAVEL_END);
	return { target.X, hover - HOVER_HEIGHT * t, target.Z };
}

void LiftCalc::Step(const STAGE& stage, const LIFT_FIGHTER* fighters)
{
	for (int slot = 0; slot < FIGHTER_COUNT; ++slot) {
		m_Landed[slot] = false;
		m_Dropped[slot] = false;
		if (!m_Moving[slot]) continue;
		const int next = m_Step[slot] + 1;
		// 降り始める前に、起動したあとで着地点へ入ってきた人がいないか確かめる。
		if (next == LIFT_TRAVEL_END + 1 && IsOccupied(m_Target[slot], slot, fighters)) {
			VEC3 again{};
			if (ChooseLanding(stage, m_Station[slot], slot, fighters, again)) m_Target[slot] = again;
			else {
				// 全部ふさがれていれば、予約した点の上から速さ 0 で落とす。
				m_Feet[slot] = { m_Target[slot].X, m_Target[slot].Y + STAGE_BODY_HEIGHT, m_Target[slot].Z };
				m_Dropped[slot] = true;
				m_Moving[slot] = false;
				continue;
			}
		}
		m_Step[slot] = next;
		m_Feet[slot] = PositionAt(slot, next);
		if (next >= LIFT_TOTAL_FRAMES) {
			m_Landed[slot] = true;
			m_Moving[slot] = false;
		}
	}
}

void LiftCalc::Cancel(int slot)
{
	m_Moving[slot] = false;
	m_Step[slot] = 0;
}
