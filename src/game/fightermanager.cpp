#include "game/fightermanager.h"
#include "game/stage.h"
#include "game/camera.h"
#include "game/arm.h"

#ifdef _DEBUG
#include "core/debug_ostream.h"
#include <iostream>
#endif

// 調整用の値（1Pとダミー3体の配置は従来どおり）
static constexpr float SPAWN_X[FIGHTER_COUNT] = { 0.0f, 0.0f, -7.0f, 7.0f };
static constexpr float SPAWN_Z[FIGHTER_COUNT] = { 0.0f, 8.0f, 5.0f, 5.0f };

void FighterManager::Initialize(const Stage& stage)
{
	m_Versus.Reset();
	m_Versus.AssignDevice(VERSUS_DEVICE_COMBINED);
	const STAGE& platform = stage.GetStage();
	for (int i = 0; i < FIGHTER_COUNT; ++i) {
		m_Fighters[i].Initialize(stage, m_Versus.GetDevice(i),
			{ platform.CenterX + SPAWN_X[i], platform.TopY, platform.CenterZ + SPAWN_Z[i] });
	}
}

void FighterManager::BindPlayer(int slot, const Camera& camera, const ArmManager& arms)
{
	if (!m_Versus.IsHuman(slot)) return;
	m_Cameras[slot] = &camera;
	m_Arms[slot] = &arms;
}

void FighterManager::Reset()
{
	// 得点だけでなく最後の攻撃者も消す。機器とスロットの対応は保つ。
	const VersusCalc previous = m_Versus;
	m_Versus.Reset();
	for (int i = 0; i < FIGHTER_COUNT; ++i) {
		if (previous.IsHuman(i)) m_Versus.AssignDevice(previous.GetDevice(i));
		m_Fighters[i].Reset();
	}
}

void FighterManager::Update()
{
	for (int i = 0; i < FIGHTER_COUNT; ++i) {
		const DUMMY_EVENT event = m_Fighters[i].Update(m_Cameras[i], m_Arms[i]);
		if (event == DUMMY_EVENT_OUT) m_Versus.RecordOut(i);
#ifdef _DEBUG
		if (event == DUMMY_EVENT_OUT || event == DUMMY_EVENT_RESPAWN) {
			hal::dout << "体" << i + 1 << (event == DUMMY_EVENT_OUT ? "：場外" : "：復活（0%）") << std::endl;
		}
#endif
	}
	PushBodies();
}

void FighterManager::PushBodies()
{
	for (int i = 0; i < FIGHTER_COUNT; ++i) {
		for (int j = i + 1; j < FIGHTER_COUNT; ++j) {
			Fighter& a = m_Fighters[i];
			Fighter& b = m_Fighters[j];
			if (!a.CanUseArms() || !b.CanUseArms() || a.IsPulled() || b.IsPulled()) continue;
			const XMFLOAT3 pa = a.GetPosition(), pb = b.GetPosition();
			VEC3 first{ pa.x, pa.y, pa.z }, second{ pb.x, pb.y, pb.z };
			if (VersusCalcPushBodies(first, second, a.IsHuman(), b.IsHuman(), a.GetBody().Radius, a.GetHeight())) {
				a.SetPosition({ first.X, first.Y, first.Z });
				b.SetPosition({ second.X, second.Y, second.Z });
			}
		}
	}
}

void FighterManager::UpdateFacing()
{
	for (int i = 0; i < FIGHTER_COUNT; ++i) {
		if (m_Versus.IsHuman(i) && m_Cameras[i] != nullptr && m_Arms[i] != nullptr) {
			m_Fighters[i].UpdateFacing(*m_Cameras[i], *m_Arms[i], *this, i);
		}
	}
}

void FighterManager::Draw(int target) const
{
	for (int i = 0; i < FIGHTER_COUNT; ++i) m_Fighters[i].Draw(i == target);
}

bool FighterManager::IsActive(int index) const
{
	return index >= 0 && index < FIGHTER_COUNT && m_Fighters[index].IsActive();
}

float FighterManager::GetPercent(int index) const
{
	return index >= 0 && index < FIGHTER_COUNT ? m_Fighters[index].GetPercent() : 0.0f;
}

TENTACLE_TARGET FighterManager::GetTarget(int index, int self) const
{
	if (!m_Versus.IsOpponent(self, index) || !IsActive(index)) return {};
	const Fighter& fighter = m_Fighters[index];
	return { fighter.GetBody(), !fighter.IsHeld() };
}

bool FighterManager::CanAttack(int index, int attacker) const
{
	return m_Versus.IsOpponent(attacker, index) && IsActive(attacker) && IsActive(index);
}

bool FighterManager::Grab(int index, int attacker)
{
	if (!CanAttack(index, attacker) || !m_Fighters[index].Grab(attacker)) return false;
	m_Versus.RecordAttack(attacker, index);
	return true;
}

bool FighterManager::IsHeldBy(int index, int holder) const
{
	return m_Versus.IsOpponent(holder, index) && IsActive(index) && m_Fighters[index].IsHeldBy(holder);
}

void FighterManager::MoveHeld(int index, int holder, const VEC3& center)
{
	if (!IsHeldBy(index, holder)) return;
	m_Fighters[index].MoveHeld(holder, center);
}

void FighterManager::Release(int index, int holder, bool throwTarget, const VEC3& direction, float damage, float power)
{
	if (!IsHeldBy(index, holder)) return;
	m_Fighters[index].Release(holder);
	if (throwTarget) Launch(index, holder, direction, damage, power);
}

void FighterManager::AddDamage(int index, int attacker, float damage)
{
	if (!CanAttack(index, attacker)) return;
	m_Fighters[index].AddDamage(damage);
	m_Versus.RecordAttack(attacker, index);
}

void FighterManager::Hit(int index, int attacker, const XMFLOAT3& direction, float damage, float power)
{
	if (!CanAttack(index, attacker)) return;
	m_Fighters[index].Hit({ direction.x, direction.y, direction.z }, damage, power);
	m_Versus.RecordAttack(attacker, index);
}

void FighterManager::Launch(int index, int attacker, const VEC3& direction, float damage, float power)
{
	if (!CanAttack(index, attacker)) return;
	m_Fighters[index].Launch(direction, damage, power);
	m_Versus.RecordAttack(attacker, index);
}

void FighterManager::Push(int index, int attacker, const VEC3& velocity)
{
	if (!CanAttack(index, attacker) || m_Fighters[index].IsHeld()) return;
	m_Fighters[index].Push(velocity);
	m_Versus.RecordAttack(attacker, index);
}
