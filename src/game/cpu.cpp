#include "game/cpu.h"
#include "core/gametime.h"
#include "game/arm.h"
#include "game/armcapsule.h"
#include "game/camera.h"
#include "game/fightermanager.h"
#include "game/stage.h"

//------------------------------------------------------------------------------
// 調整用の値（腕ごとの CPU の使い方。届く距離は m、長押しは秒）
//------------------------------------------------------------------------------
static constexpr CPU_ARM ARM_STYLES[ARM_TYPE_COUNT] = {
	//  使う  届く距離 長押し  長押しの割合 ガード 復帰
	{ true, 4.0f, 0.0f, 0.0f, false, CPU_RECOVERY_NONE }, // 触手：近づいてつかむ（段階攻撃でたたきつけ・投げ）
	{ true, 2.2f, 0.8f, 0.25f, true, CPU_RECOVERY_TAP }, // ハンマー：空中の単押しの跳ねで戻る
	{ true, 1.8f, 0.6f, 0.20f, true, CPU_RECOVERY_HOLD }, // ナックル：空中の長押しの突進で戻る
	{ true, 14.0f, 3.2f, 0.30f, false, CPU_RECOVERY_NONE }, // レーザー：遠くから撃つ。たまに溜めきった砲
	{ true, 2.2f, 0.0f, 0.0f, true, CPU_RECOVERY_NONE }, // 刀：長押しのワープは使わない
	{ true, 3.0f, 1.0f, 0.20f, true, CPU_RECOVERY_NONE }, // モーニングスター：たまに回してから投げる
	{ true, 3.0f, 0.8f, 0.20f, true, CPU_RECOVERY_NONE }, // 槍：たまに投げて突っ込む
	{ true, 12.0f, 0.4f, 0.20f, false, CPU_RECOVERY_NONE }, // ミサイル：遠くから撃つ。たまに長押しで残りを全部撃つ
	{ false, 0.0f, 0.0f, 0.0f, false, CPU_RECOVERY_NONE }, // 素手：攻撃できない
};
static constexpr uint32_t SLOT_SEED_STEP = 0x27d4eb2fu; // スロットごとに乱数の並びを変える

void Cpu::Reset(uint32_t seed, int slot, int level)
{
	m_Slot = slot;
	m_Level = level;
	m_Seed = seed + SLOT_SEED_STEP * static_cast<uint32_t>(slot + 1);
	m_Calc.Reset(m_Seed, slot, level);
}

PLAYER_INPUT Cpu::Update(const Stage& stage, const FighterManager& fighters, const ArmManager& arms,
	const Camera& camera, const ArmCapsule& capsules, GAME_RULE rule)
{
	const Fighter& self = fighters.GetFighter(m_Slot);
	if (!self.IsActive()) {
		// 復活を待つ間は何もしない。押しっぱなしの長押し・ガードを残さないよう考えを最初に戻す。
		m_Seed += SLOT_SEED_STEP;
		m_Calc.Reset(m_Seed, m_Slot, m_Level);
		return {};
	}
	CPU_VIEW view;
	view.Dt = GameTimeGetDeltaTime();
	const XMFLOAT3 position = self.GetPosition();
	view.Position = { position.x, position.y, position.z };
	view.OnGround = self.IsOnGround();
	view.CanAct = self.CanUseArms();
	view.CameraYaw = camera.GetYaw();
	view.CameraTarget = camera.GetTarget();
	bool emptyHand = false;
	for (int hand = 0; hand < 2; ++hand) {
		const ARM_TYPE type = arms.GetType(hand);
		view.Arms[hand] = ARM_STYLES[type < ARM_TYPE_COUNT ? type : ARM_TYPE_NONE];
		if (!arms.CanUseArm(hand)) view.Arms[hand].Usable = false;
		if (type == ARM_TYPE_NONE) emptyHand = true;
	}
	view.WantsArm = rule == GAME_RULE_GAME && emptyHand;
	for (int slot = 0; slot < FIGHTER_COUNT; ++slot) {
		const Fighter& other = fighters.GetFighter(slot);
		CPU_OPPONENT& opponent = view.Opponents[slot];
		// カメラでロックオンできない相手（場外へ落ちている相手など）は狙わない。
		// 狙うと、ロックオンを合わせようと切り替えボタンを押し続け、カメラと体の向きが回り続ける。
		opponent.Available = slot != m_Slot && fighters.IsActive(slot) && !fighters.IsHidden(slot) && !other.IsInvincible()
			&& camera.CanLockOn(slot);
		const XMFLOAT3 p = other.GetPosition();
		opponent.Position = { p.x, p.y, p.z };
		opponent.Percent = fighters.GetPercent(slot);
	}
	for (const ARM_CAPSULE_ITEM& item : capsules.GetItems()) {
		if (!item.Active || view.CapsuleCount >= CPU_CAPSULE_MAX) continue;
		view.Capsules[view.CapsuleCount++] = item.Position;
	}
	view.Stage = stage.GetStage();
	return m_Calc.Step(view);
}
