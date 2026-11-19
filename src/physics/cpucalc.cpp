#include "physics/cpucalc.h"
#include <math.h>

//------------------------------------------------------------------------------
// 調整用の値
//------------------------------------------------------------------------------
static constexpr CPU_PARAMS LEVEL_PARAMS[VERSUS_CPU_LEVEL_COUNT] = {
	{ 0.35f, 0.70f, 0.25f, 0.85f }, // （ダミーの枠では使わない。範囲外と同じ「ふつう」）
	{ 0.60f, 1.30f, 0.00f, 0.70f }, // よわい：反応が遅く、攻撃が少なく、守らない
	{ 0.35f, 0.70f, 0.25f, 0.85f }, // ふつう
	{ 0.18f, 0.40f, 0.55f, 0.95f }, // つよい：すぐ反応し、よく攻撃し、よく守る
};
static constexpr float THINK_JITTER = 0.25f; // 考え直す間隔・攻撃の間隔のばらつき（±割合）
static constexpr float PERCENT_WEIGHT = 0.02f; // 相手を選ぶとき、%が1増えるごとに近いとみなす距離（m）
static constexpr float TARGET_KEEP_BONUS = 1.5f; // 相手を選び直すとき、今の相手をこの距離（m）だけ近いとみなす（同じくらいの相手で入れ替わり続けない）
static constexpr float CHASE_RANGE_MIN = 2.0f; // 腕を使えないときに近づく距離（m。体どうしが押し合わない距離）
static constexpr float CHASE_RESUME_MARGIN = 0.8f; // 止まったあと、止まる距離よりこの距離（m）離れてから歩き出す（歩く・止まるを毎フレーム繰り返さない）
static constexpr float CAPSULE_SEEK_RANGE = 14.0f; // 使える腕があるとき、この距離（m）以内のカプセルを拾いに行く（素手なら距離を問わない）
static constexpr float EDGE_LOOKAHEAD = 1.2f; // 足場のふちを避けるため、この距離（m）先に足場があるかを見る
static constexpr float FALL_MARGIN = 0.5f; // 浮島の上の面よりこの高さ（m）下なら落ちているとみなす
static constexpr float RECOVER_JUMP_SECONDS = 0.35f; // 場外で空中ジャンプを押す間隔（秒）
static constexpr float RECOVER_DASH_BELOW = 1.5f; // 浮島の上の面よりこの高さ（m）下ならダッシュで戻る
static constexpr float RECOVER_ARM_BELOW = 2.5f; // 浮島の上の面よりこの高さ（m）下なら腕の復帰技を使う
static constexpr float RECOVER_HOLD_SECONDS = 0.5f; // 腕の復帰技を長押しする秒数
static constexpr float GUARD_RANGE = 3.0f; // この距離（m）以内に相手がいるとガードを考える
static constexpr float GUARD_SECONDS = 0.5f; // ガードを続ける秒数
static constexpr float TARGET_SWITCH_SECONDS = 0.2f; // ロックオンを切り替えるボタンを押す間隔（秒）

static uint16_t Bit(ACTION action) { return static_cast<uint16_t>(1u << action); }
static ACTION ArmAction(int hand) { return hand == 0 ? ACTION_ARM_LEFT : ACTION_ARM_RIGHT; }
static ACTION GuardAction(int hand) { return hand == 0 ? ACTION_GUARD_LEFT : ACTION_GUARD_RIGHT; }

CPU_PARAMS CpuCalcParams(int level)
{
	return level > VERSUS_CPU_NONE && level < VERSUS_CPU_LEVEL_COUNT ? LEVEL_PARAMS[level] : LEVEL_PARAMS[VERSUS_CPU_NORMAL];
}

ACTION_AXIS CpuCalcWorldToMove(float worldX, float worldZ, float cameraYaw)
{
	// Fighter は X = cos·右 + sin·前、Z = −sin·右 + cos·前 で世界の向きにする。その逆の回転。
	const float c = cosf(cameraYaw), s = sinf(cameraYaw);
	return { c * worldX - s * worldZ, s * worldX + c * worldZ };
}

void CpuCalc::Reset(uint32_t seed, int slot, int level)
{
	*this = CpuCalc{};
	m_Random.SetSeed(seed);
	m_Slot = slot;
	m_Params = CpuCalcParams(level);
	// 全員が同じフレームに動き出さないよう、最初の考えるまでの時間をずらす。
	m_ThinkTimer = m_Random.NextFloat(0.0f, m_Params.ThinkSeconds);
	m_AttackTimer = m_Params.AttackSeconds;
}

int CpuCalc::ChooseHand(const CPU_VIEW& view, float distance) const
{
	// 届く手のうち届く距離の短い手（近い相手には近接）、届く手がなければ届く距離のいちばん長い手。
	int best = -1;
	for (int hand = 0; hand < 2; ++hand) {
		const CPU_ARM& arm = view.Arms[hand];
		if (!arm.Usable) continue;
		const float range = arm.Range * m_Params.RangeScale;
		if (best < 0) { best = hand; continue; }
		const float bestRange = view.Arms[best].Range * m_Params.RangeScale;
		const bool reach = distance <= range, bestReach = distance <= bestRange;
		if ((reach && (!bestReach || range < bestRange)) || (!reach && !bestReach && range > bestRange)) best = hand;
	}
	return best;
}

void CpuCalc::Think(const CPU_VIEW& view)
{
	m_ThinkTimer = m_Params.ThinkSeconds * m_Random.NextFloat(1.0f - THINK_JITTER, 1.0f + THINK_JITTER);
	// 近くて%の高い相手を狙う（高い%の相手ほど場外にしやすい）。今の相手は少し優先する。
	const int previous = m_Target;
	m_Target = -1;
	float bestScore = 0.0f;
	for (int slot = 0; slot < FIGHTER_COUNT; ++slot) {
		const CPU_OPPONENT& opponent = view.Opponents[slot];
		if (slot == m_Slot || !opponent.Available) continue;
		const float dx = opponent.Position.X - view.Position.X, dz = opponent.Position.Z - view.Position.Z;
		float score = sqrtf(dx * dx + dz * dz) - opponent.Percent * PERCENT_WEIGHT;
		if (slot == previous) score -= TARGET_KEEP_BONUS;
		if (m_Target < 0 || score < bestScore) { m_Target = slot; bestScore = score; }
	}
	// 相手が近ければ、ある割合でガードする。
	if (m_Target >= 0 && m_GuardTimer <= 0.0f && m_HoldHand < 0) {
		const VEC3& p = view.Opponents[m_Target].Position;
		const float dx = p.X - view.Position.X, dz = p.Z - view.Position.Z;
		if (dx * dx + dz * dz <= GUARD_RANGE * GUARD_RANGE && m_Random.NextFloat(0.0f, 1.0f) < m_Params.GuardChance) {
			for (int hand = 0; hand < 2; ++hand) {
				if (!view.Arms[hand].CanGuard) continue;
				m_GuardHand = hand;
				m_GuardTimer = GUARD_SECONDS;
				break;
			}
		}
	}
}

PLAYER_INPUT CpuCalc::Step(const CPU_VIEW& view)
{
	const float dt = view.Dt > 0.0f ? view.Dt : 0.0f;
	m_ThinkTimer -= dt;
	m_AttackTimer -= dt;
	m_JumpTimer -= dt;
	m_TargetTimer -= dt;
	if (m_ThinkTimer <= 0.0f) Think(view);
	if (m_Target >= 0 && !view.Opponents[m_Target].Available) m_Target = -1;

	uint16_t buttons = 0;
	float worldX = 0.0f, worldZ = 0.0f;
	const STAGE& stage = view.Stage;
	const VEC3& self = view.Position;
	if (view.OnGround) m_AirDashed = m_AirRecovered = false;
	const bool overStage = StageCalcIsOnPlatform(stage, self.X, self.Z);
	const float below = stage.TopY - self.Y; // 浮島の上の面より何 m 下にいるか
	if (!overStage || below > FALL_MARGIN) {
		// 場外：浮島の中心へ向かい、空中ジャンプ・ダッシュ・腕の復帰技で戻る。攻撃はしない。
		m_GuardTimer = 0.0f;
		worldX = stage.CenterX - self.X;
		worldZ = stage.CenterZ - self.Z;
		if (!view.OnGround && m_JumpTimer <= 0.0f) {
			buttons |= Bit(ACTION_JUMP);
			m_JumpTimer = RECOVER_JUMP_SECONDS;
		}
		if (!view.OnGround && !m_AirDashed && below > RECOVER_DASH_BELOW) {
			buttons |= Bit(ACTION_DASH);
			m_AirDashed = true;
		}
		if (!view.OnGround && !m_AirRecovered && below > RECOVER_ARM_BELOW && view.CanAct && m_HoldHand < 0) {
			for (int hand = 0; hand < 2; ++hand) {
				const CPU_RECOVERY recovery = view.Arms[hand].Recovery;
				if (recovery == CPU_RECOVERY_NONE) continue;
				m_AirRecovered = true;
				if (recovery == CPU_RECOVERY_TAP) buttons |= Bit(ArmAction(hand));
				else { m_HoldHand = hand; m_HoldTimer = RECOVER_HOLD_SECONDS; }
				break;
			}
		}
	}
	else {
		// 足場の上：カプセルを拾いに行くか、相手へ近づいて腕の届く距離で攻撃する。
		float goalX = self.X, goalZ = self.Z, stopDistance = 0.0f;
		bool seeking = false;
		if (view.WantsArm) {
			const bool armed = view.Arms[0].Usable || view.Arms[1].Usable;
			float bestSq = armed ? CAPSULE_SEEK_RANGE * CAPSULE_SEEK_RANGE : 1.0e9f;
			for (int i = 0; i < view.CapsuleCount && i < CPU_CAPSULE_MAX; ++i) {
				const float dx = view.Capsules[i].X - self.X, dz = view.Capsules[i].Z - self.Z;
				const float sq = dx * dx + dz * dz;
				if (sq > bestSq) continue;
				bestSq = sq;
				goalX = view.Capsules[i].X;
				goalZ = view.Capsules[i].Z;
				seeking = true;
			}
		}
		int hand = -1;
		float distance = 0.0f;
		if (m_Target >= 0) {
			const VEC3& p = view.Opponents[m_Target].Position;
			const float dx = p.X - self.X, dz = p.Z - self.Z;
			distance = sqrtf(dx * dx + dz * dz);
			hand = ChooseHand(view, distance);
			if (!seeking) {
				goalX = p.X;
				goalZ = p.Z;
				stopDistance = hand >= 0 ? view.Arms[hand].Range * m_Params.RangeScale * 0.8f : CHASE_RANGE_MIN;
			}
		}
		const float gx = goalX - self.X, gz = goalZ - self.Z;
		const float goalDistance = sqrtf(gx * gx + gz * gz);
		// 止まる距離まで近づいたら止まり、少し離れるまでは歩き出さない（カプセルは近づき続ける）。
		if (goalDistance <= stopDistance) m_Chasing = false;
		else if (seeking || goalDistance > stopDistance + CHASE_RESUME_MARGIN) m_Chasing = true;
		if (m_Chasing && goalDistance > 0.0f) {
			worldX = gx / goalDistance;
			worldZ = gz / goalDistance;
			// 自分から場外へ歩き出さないよう、少し先に足場がなければ止まる。
			if (!StageCalcIsOnPlatform(stage, self.X + worldX * EDGE_LOOKAHEAD, self.Z + worldZ * EDGE_LOOKAHEAD)) worldX = worldZ = 0.0f;
		}
		// 攻撃：ガード中・長押し中でなく、攻撃の間隔があいていて、腕が届けば押す。
		if (m_GuardTimer <= 0.0f && m_HoldHand < 0 && view.CanAct && hand >= 0 && m_AttackTimer <= 0.0f
			&& distance <= view.Arms[hand].Range * m_Params.RangeScale) {
			const CPU_ARM& arm = view.Arms[hand];
			if (arm.HoldSeconds > 0.0f && m_Random.NextFloat(0.0f, 1.0f) < arm.HoldChance) {
				m_HoldHand = hand;
				m_HoldTimer = arm.HoldSeconds;
			}
			else buttons |= Bit(ArmAction(hand));
			m_AttackTimer = m_Params.AttackSeconds * m_Random.NextFloat(1.0f - THINK_JITTER, 1.0f + THINK_JITTER);
		}
	}
	// 長押し：決めた秒数だけ押し続け、離した瞬間に出す。
	if (m_HoldHand >= 0) {
		if (m_HoldTimer > 0.0f && view.CanAct) buttons |= Bit(ArmAction(m_HoldHand));
		else m_HoldHand = -1;
		m_HoldTimer -= dt;
	}
	// ガード：決めた秒数だけ押し続ける。
	if (m_GuardTimer > 0.0f && m_GuardHand >= 0) {
		buttons |= Bit(GuardAction(m_GuardHand));
		m_GuardTimer -= dt;
	}
	// ロックオンの相手を、狙っている相手にそろえる（攻撃の向きはハードロックが合わせる）。
	// カメラがロックしていないときは、カメラが自分で正面の相手を選ぶので押さない。
	// 狙う相手が今のロックの相手より右にいれば右、左にいれば左へ切り替える（遠回りして画面を回さない）。
	if (m_Target >= 0 && view.CameraTarget >= 0 && view.CameraTarget < FIGHTER_COUNT
		&& view.CameraTarget != m_Target && m_TargetTimer <= 0.0f) {
		const VEC3& locked = view.Opponents[view.CameraTarget].Position;
		const VEC3& wanted = view.Opponents[m_Target].Position;
		float turn = atan2f(wanted.X - self.X, wanted.Z - self.Z) - atan2f(locked.X - self.X, locked.Z - self.Z);
		turn = atan2f(sinf(turn), cosf(turn));
		buttons |= Bit(turn >= 0.0f ? ACTION_TARGET_RIGHT : ACTION_TARGET_LEFT);
		m_TargetTimer = TARGET_SWITCH_SECONDS;
	}
	return ActionCalcPlayerInput(CpuCalcWorldToMove(worldX, worldZ, view.CameraYaw), {}, buttons);
}
