#include "game/fightermanager.h"
#include "core/gametime.h"
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

void FighterManager::Initialize(const Stage& stage, const VersusCalc& participants)
{
	m_Versus = participants;
	m_Versus.ResetMatch();
	const STAGE& platform = stage.GetStage();
	for (int i = 0; i < FIGHTER_COUNT; ++i) {
		m_Cameras[i] = nullptr;
		m_Arms[i] = nullptr;
		m_Fighters[i].Initialize(stage, i, m_Versus.GetDevice(i),
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
	m_Versus.ResetMatch();
	for (int i = 0; i < FIGHTER_COUNT; ++i) {
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
	// 燃えた分の % は、燃やした人の攻撃として記録する（場外にしたらその人の得点）。
	for (int i = 0; i < FIGHTER_COUNT; ++i) {
		const STATUS_STEP status = m_Fighters[i].StepStatus(GameTimeGetDeltaTime());
		if (status.Damage <= 0.0f) continue;
		m_Fighters[i].AddDamage(status.Damage);
		if (status.Attacker >= 0 && m_Versus.IsOpponent(status.Attacker, i)) m_Versus.RecordAttack(status.Attacker, i);
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
	// ガードされた攻撃は効かない（No.80）。燃える % と投げ（Release）はここを通らない。
	return m_Versus.IsOpponent(attacker, index) && IsActive(attacker) && IsActive(index) && !IsGuarded(index, attacker);
}

bool FighterManager::IsGuarded(int index, int attacker) const
{
	if (index < 0 || index >= FIGHTER_COUNT || attacker < 0 || attacker >= FIGHTER_COUNT || m_Arms[index] == nullptr) return false;
	const CAPSULE body = m_Fighters[index].GetBody();
	const CAPSULE from = m_Fighters[attacker].GetBody();
	return GuardCalcBlocks(m_Arms[index]->GetGuard(), Vec3Scale(Vec3Add(body.Start, body.End), 0.5f),
		Vec3Scale(Vec3Add(from.Start, from.End), 0.5f));
}

bool FighterManager::BreakGuard(int index, int attacker, float seconds, float damage)
{
	if (!m_Versus.IsOpponent(attacker, index) || !IsActive(attacker) || !IsActive(index)) return false;
	m_Fighters[index].ApplyStatus(STATUS_STUN, seconds, 0.0f, attacker);
	m_Fighters[index].AddDamage(damage);
	m_Versus.RecordAttack(attacker, index);
	return true;
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

bool FighterManager::ApplyStatus(int index, int attacker, STATUS_TYPE type, float seconds, float value)
{
	if (type == STATUS_HIDDEN) {
		if (!IsActive(index)) return false;
		m_Fighters[index].ApplyStatus(type, seconds, value, -1);
		return true;
	}
	if (!CanAttack(index, attacker)) return false;
	m_Fighters[index].ApplyStatus(type, seconds, value, attacker);
	m_Versus.RecordAttack(attacker, index);
	return true;
}

bool FighterManager::IsHidden(int index) const
{
	return IsActive(index) && m_Fighters[index].IsStatus(STATUS_HIDDEN);
}

void FighterManager::Heal(int index, float amount)
{
	if (IsActive(index)) m_Fighters[index].Heal(amount);
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
