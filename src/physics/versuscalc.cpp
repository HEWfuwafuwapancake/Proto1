#include "physics/versuscalc.h"

#include "physics/dummycalc.h"

bool VersusCalcPushBodies(VEC3& first, VEC3& second, bool firstHuman, bool secondHuman, float radius, float height)
{
	if (!firstHuman && !secondHuman) return false;
	if (firstHuman && secondHuman && first.Y < second.Y) {
		// 上に乗った体を押し上げる。スロット番号で上下の扱いが変わらないようにする。
		return VersusCalcPushBodies(second, first, true, true, radius, height);
	}
	if (!firstHuman) return DummyCalcPushByPlayer(second, first, radius, height);
	const VEC3 before = second;
	if (!DummyCalcPushByPlayer(first, second, radius, height)) return false;
	if (secondHuman) {
		// 横からの押し出しだけ分ける。真上から乗った体の押し出しは従来どおり。
		const VEC3 half = Vec3Scale(Vec3Sub(second, before), 0.5f);
		first = Vec3Sub(first, half);
		second = Vec3Sub(second, half);
	}
	return true;
}

void VersusCalc::Reset()
{
	for (int i = 0; i < FIGHTER_COUNT; ++i) {
		m_Devices[i] = VERSUS_DEVICE_DUMMY;
		m_Ready[i] = false;
	}
	ResetMatch();
}

void VersusCalc::ResetMatch()
{
	for (int i = 0; i < FIGHTER_COUNT; ++i) {
		m_LastAttackers[i] = -1;
		m_Scores[i] = 0;
	}
}

VERSUS_LOBBY_DECIDE VersusCalc::LobbyDecide(int device, bool canJoin)
{
	if (device < 0 || device > 4) return VERSUS_LOBBY_DECIDE_NONE;
	const int slot = FindSlot(device);
	if (slot < 0) {
		if (!canJoin) return VERSUS_LOBBY_DECIDE_NONE;
		return AssignDevice(device) >= 0 ? VERSUS_LOBBY_DECIDE_JOIN : VERSUS_LOBBY_DECIDE_NONE;
	}
	if (!m_Ready[slot]) {
		m_Ready[slot] = true;
		return VERSUS_LOBBY_DECIDE_READY;
	}
	// 始めるのは 1P だけ。ほかの人が準備OKのまま押しても何も起きない。
	return slot == 0 && IsAllReady() ? VERSUS_LOBBY_DECIDE_START : VERSUS_LOBBY_DECIDE_NONE;
}

VERSUS_LOBBY_CANCEL VersusCalc::LobbyCancel(int device)
{
	const int slot = FindSlot(device);
	if (slot < 0) return VERSUS_LOBBY_CANCEL_NONE;
	if (m_Ready[slot]) {
		m_Ready[slot] = false;
		return VERSUS_LOBBY_CANCEL_UNREADY;
	}
	if (slot == 0) return VERSUS_LOBBY_CANCEL_BACK;
	m_Devices[slot] = VERSUS_DEVICE_DUMMY;
	return VERSUS_LOBBY_CANCEL_LEAVE;
}

void VersusCalc::ClearReady()
{
	for (bool& ready : m_Ready) ready = false;
}

bool VersusCalc::IsReady(int slot) const
{
	return IsHuman(slot) && m_Ready[slot];
}

bool VersusCalc::IsAllReady() const
{
	int humans = 0;
	for (int i = 0; i < FIGHTER_COUNT; ++i) {
		if (!IsHuman(i)) continue;
		if (!m_Ready[i]) return false;
		++humans;
	}
	return humans > 0;
}

int VersusCalc::FindSlot(int device) const
{
	if (device == VERSUS_DEVICE_DUMMY) return -1;
	for (int i = 0; i < FIGHTER_COUNT; ++i) {
		if (m_Devices[i] == device) return i;
	}
	return -1;
}

int VersusCalc::GetHumanCount() const
{
	int count = 0;
	for (int i = 0; i < FIGHTER_COUNT; ++i) {
		if (IsHuman(i)) ++count;
	}
	return count;
}

int VersusCalc::AssignDevice(int device)
{
	if (device != VERSUS_DEVICE_COMBINED && (device < 0 || device > 4)) return -1;
	for (int i = 0; i < FIGHTER_COUNT; ++i) {
		if (m_Devices[i] == device) return i;
	}
	for (int i = 0; i < FIGHTER_COUNT; ++i) {
		if (m_Devices[i] != VERSUS_DEVICE_DUMMY) continue;
		m_Devices[i] = device;
		return i;
	}
	return -1;
}

void VersusCalc::SetSlotDevice(int slot, int device)
{
	if (slot < 0 || slot >= FIGHTER_COUNT || device < VERSUS_DEVICE_REMOTE || device > 4) return;
	m_Devices[slot] = device;
}

int VersusCalc::GetDevice(int slot) const
{
	return slot >= 0 && slot < FIGHTER_COUNT ? m_Devices[slot] : VERSUS_DEVICE_DUMMY;
}

bool VersusCalc::IsHuman(int slot) const
{
	return GetDevice(slot) != VERSUS_DEVICE_DUMMY;
}

bool VersusCalc::IsOpponent(int self, int target) const
{
	return self >= 0 && self < FIGHTER_COUNT && target >= 0 && target < FIGHTER_COUNT && self != target;
}

void VersusCalc::RecordAttack(int attacker, int target)
{
	if (IsOpponent(attacker, target)) m_LastAttackers[target] = attacker;
}

void VersusCalc::RecordOut(int target)
{
	if (target < 0 || target >= FIGHTER_COUNT) return;
	const int attacker = m_LastAttackers[target];
	if (IsOpponent(attacker, target) && IsHuman(attacker)) ++m_Scores[attacker];
	m_LastAttackers[target] = -1;
}

int VersusCalc::GetScore(int slot) const
{
	return slot >= 0 && slot < FIGHTER_COUNT ? m_Scores[slot] : 0;
}

int VersusCalc::GetLastAttacker(int slot) const
{
	return slot >= 0 && slot < FIGHTER_COUNT ? m_LastAttackers[slot] : -1;
}

int VersusCalc::GetRank(int slot) const
{
	if (!IsHuman(slot)) return 0;
	int rank = 1;
	for (int i = 0; i < FIGHTER_COUNT; ++i) {
		if (IsHuman(i) && m_Scores[i] > m_Scores[slot]) ++rank;
	}
	return rank;
}

int VersusCalc::GetRankedSlot(int index) const
{
	for (int slot = 0; slot < FIGHTER_COUNT; ++slot) {
		if (!IsHuman(slot)) continue;
		int order = 0;
		for (int other = 0; other < FIGHTER_COUNT; ++other) {
			if (!IsHuman(other)) continue;
			if (m_Scores[other] > m_Scores[slot]
				|| (m_Scores[other] == m_Scores[slot] && other < slot)) ++order;
		}
		if (order == index) return slot;
	}
	return -1;
}

VERSUS_VIEWPORT VersusCalcViewport(int humanCount, int index, float screenWidth, float screenHeight)
{
	if (humanCount <= 1 || humanCount > FIGHTER_COUNT || index < 0 || index >= FIGHTER_COUNT) {
		return { 0.0f, 0.0f, screenWidth, screenHeight };
	}
	const float halfWidth = screenWidth * 0.5f;
	if (humanCount == 2) {
		if (index >= 2) return { 0.0f, 0.0f, screenWidth, screenHeight };
		return { halfWidth * (float)index, 0.0f, halfWidth, screenHeight };
	}
	const float halfHeight = screenHeight * 0.5f;
	return { halfWidth * (float)(index % 2), halfHeight * (float)(index / 2), halfWidth, halfHeight };
}
