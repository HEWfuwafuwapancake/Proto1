#include "game/fightermanager.h"
#include "core/gametime.h"
#include "game/stage.h"
#include "game/camera.h"
#include "game/arm.h"
#include "render/damagecolorcalc.h"
#include <math.h>

#ifdef _DEBUG
#include "core/debug_ostream.h"
#include <iostream>
#endif

// 調整用の値（1Pとダミー3体の配置は従来どおり）
static constexpr float SPAWN_X[FIGHTER_COUNT] = { 0.0f, 0.0f, -7.0f, 7.0f };
static constexpr float SPAWN_Z[FIGHTER_COUNT] = { 0.0f, 8.0f, 5.0f, 5.0f };
// GAME の復活先（浮島の中心と上面からの相対位置）
static constexpr VEC3 ISLAND_RESPAWN_OFFSETS[] = {
	{ 0.0f, 0.0f, -11.0f }, { 0.0f, 0.0f, 11.0f },
	{ -11.0f, 0.0f, 0.0f }, { 11.0f, 0.0f, 0.0f },
	{ -11.0f, 0.0f, -11.0f }, { -11.0f, 0.0f, 11.0f },
	{ 11.0f, 0.0f, -11.0f }, { 11.0f, 0.0f, 11.0f },
};
static constexpr int ISLAND_RESPAWN_COUNT = sizeof(ISLAND_RESPAWN_OFFSETS) / sizeof(ISLAND_RESPAWN_OFFSETS[0]);
static constexpr float SPAWN_SAFE_DISTANCE = 10.0f;
// ヒットストップ（当てた人と当てられた人だけ止める。plan/11_エフェクトと手ごたえ の 1-2）
static constexpr HITSTOP_PARAMS HITSTOP = {
	3.0f,	// これより小さいダメージ%は止めない（モーニングスターの回転の連続ヒットなど）
	6.0f,	// 「中」になるダメージ%（刀・ハンマーの単押し・ナックルの3段目など）
	11.0f,	// 「強」になるダメージ%（刀の突き・居合、ハンマーの空中・衝撃波、レーザーの砲）
	24.0f,	// 吹っ飛ぶ速さがこれ以上なら「決め手」（100% でハンマー・砲を受けたくらい）
	3, 6, 10, 14,	// 弱・中・強・決め手で止めるフレーム数
};
// 画面の揺れ（当てた人・当てられた人の画面だけ。強さ 0〜1）
static constexpr float SHAKE_MEDIUM = 0.3f;		// 「中」の当たり
static constexpr float SHAKE_HEAVY = 0.6f;		// 「強」の当たり
static constexpr float SHAKE_FINISH = 1.0f;		// 「決め手」の当たり
static constexpr float SHAKE_RING_OUT_WALL = 0.6f;	// 横の壁（DEAD LINE）を越えて場外になった（場外にした人・された人）
static constexpr float SHAKE_RING_OUT_FALL = 0.4f;	// 下へ落ちて場外になった
// 強い当たりで、当てた人・当てられた人の画面のふちを一瞬白く光らせる強さ（0〜1）
static constexpr float EDGE_FLASH_HEAVY = 0.6f;	// 「強」の当たり
static constexpr float EDGE_FLASH_FINISH = 1.0f;	// 「決め手」の当たり
// 当たりの火花を出す場所
static constexpr float HIT_EFFECT_HEIGHT = 0.55f;	// 当てられた人の背の高さに対する割合（胸のあたり）
static constexpr float HIT_EFFECT_FORWARD = 0.45f;	// 胸から当てた人の側へずらす距離（体の表面のあたり）
// % が高い人の煙（plan/11 の 2-1b）
static constexpr float SMOKE_GRAY_SECONDS = 0.3f;	// 100% 以上で灰色の煙を出す間隔
static constexpr float SMOKE_BLACK_SECONDS = 0.12f;	// 150% 以上で黒い煙を出す間隔
static constexpr int SMOKE_SPARK_EVERY = 4;		// 黒い煙の何回に1回、小さな火花も出すか
static constexpr float SMOKE_HEIGHT = 0.8f;		// 煙を出す高さ（背の高さに対する割合）
static constexpr float RESPAWN_PILLAR_HEIGHT = 4.0f;	// 復活の光の柱が降り始める高さ

void FighterManager::LoadModels()
{
	for (int type = 0; type < CHARACTER_COUNT; ++type) {
		const CHARACTER_DATA& character = CHARACTER_DATA::Get(static_cast<CHARACTER_TYPE>(type));
		m_Models[type].Load(character.Paths, character.Bones);
	}
}

void FighterManager::Finalize()
{
	for (SkinnedModel& model : m_Models) model.Finalize();
}

void FighterManager::Initialize(const Stage& stage, const VersusCalc& participants, EffectManager& effects)
{
	m_Stage = &stage;
	m_Effects = &effects;
	m_Versus = participants;
	m_Versus.ResetMatch();
	const STAGE& platform = stage.GetStage();
	for (int i = 0; i < FIGHTER_COUNT; ++i) {
		m_Cameras[i] = nullptr;
		m_Arms[i] = nullptr;
		m_Escapes[i].Reset();
		m_Fighters[i].Initialize(stage, i, m_Versus.GetDevice(i),
			{ platform.CenterX + SPAWN_X[i] * m_MapScale, platform.TopY, platform.CenterZ + SPAWN_Z[i] * m_MapScale }, m_Models);
	}
}

void FighterManager::BindPlayer(int slot, const Camera& camera, const ArmManager& arms)
{
	if (!m_Versus.IsHuman(slot)) return;
	m_Cameras[slot] = &camera;
	m_Arms[slot] = &arms;
}

void FighterManager::SetMatchSettings(GAME_RULE rule, uint32_t seed, const MATCH_TUNING& tuning)
{
	m_Rule = rule;
	m_Spawn.SetSeed(seed);
	m_MapScale = TuningCalcValue(tuning, TUNING_MAP_SIZE);
	const STAGE& stage = m_Stage->GetStage();
	for (int i = 0; i < FIGHTER_COUNT; ++i) {
		m_Fighters[i].SetTuningScales(TuningCalcValue(tuning, TUNING_DAMAGE), TuningCalcValue(tuning, TUNING_KNOCKBACK));
		if (i < stage.SpawnCount) {
			// 対戦マップは P1〜P4 の台帳の点から、体の正面を原点へ向けて始める。
			const VEC3& spawn = stage.Spawns[i];
			m_Fighters[i].SetSpawn(spawn, atan2f(-spawn.X, -spawn.Z));
		}
		else m_Fighters[i].SetSpawn({ stage.CenterX + SPAWN_X[i] * m_MapScale, stage.TopY, stage.CenterZ + SPAWN_Z[i] * m_MapScale });
	}
}

void FighterManager::Reset()
{
	// 得点だけでなく最後の攻撃者も消す。機器とスロットの対応は保つ。
	m_Versus.ResetMatch();
	for (int i = 0; i < FIGHTER_COUNT; ++i) {
		m_Fighters[i].Reset();
		m_GuardedHit[i] = false;
		m_GuardedBy[i] = -1;
		m_PendingShake[i] = 0.0f;
		m_PendingFlash[i] = {};
		m_Escapes[i].Reset();
	}
}

void FighterManager::Update()
{
	const float dt = GameTimeGetDeltaTime();
	for (int i = 0; i < FIGHTER_COUNT; ++i) {
		const bool wasInvincible = m_Fighters[i].IsInvincible();
		const DUMMY_EVENT event = m_Fighters[i].Update(m_Cameras[i], m_Arms[i]);
		if (event == DUMMY_EVENT_OUT) {
			// 得点を数えると最後の攻撃者が消えるので、その前に場外にした相手を覚える（復活待ちのカメラが映す）。
			m_Fighters[i].SetKoBy(m_Versus.GetLastAttacker(i));
			const float shake = StageCalcIsOut(m_Stage->GetStage(), m_Fighters[i].GetOutPosition().Y) ? SHAKE_RING_OUT_FALL : SHAKE_RING_OUT_WALL;
			AddShake(i, shake);
			AddShake(m_Versus.GetLastAttacker(i), shake);
			// 場外になった所に、その人の色の光の柱を立てる（下へ落ちたときは下から上へ）。
			const PARTICLE_COLOR color = GetEffectColor(i);
			SpawnEffect(shake == SHAKE_RING_OUT_FALL ? EFFECT_PILLAR_FALL : EFFECT_PILLAR_WALL, m_Fighters[i].GetOutPosition(), { 0.0f, 1.0f, 0.0f }, &color);
			m_Versus.RecordOut(i);
		}
		else if (event == DUMMY_EVENT_RESPAWN && m_Rule == GAME_RULE_GAME) RespawnAtSafePoint(i);
		if (event == DUMMY_EVENT_RESPAWN) {
			// 復活した所へ、上から光の柱を降ろし、足元に輪を広げる。
			const PARTICLE_COLOR color = GetEffectColor(i);
			const VEC3 feet = GetBodyPoint(i, 0.0f);
			SpawnEffect(EFFECT_PILLAR_RESPAWN, { feet.X, feet.Y + RESPAWN_PILLAR_HEIGHT, feet.Z }, { 0.0f, -1.0f, 0.0f }, &color);
			SpawnEffect(EFFECT_FEET_RING, feet, { 0.0f, 1.0f, 0.0f }, &color);
		}
		else if (wasInvincible && !m_Fighters[i].IsInvincible() && m_Fighters[i].IsActive()) {
			SpawnEffect(EFFECT_SPARKLE, GetBodyPoint(i, HIT_EFFECT_HEIGHT), { 0.0f, 1.0f, 0.0f });
		}
		UpdateDamageEffects(i, dt);
#ifdef _DEBUG
		if (event == DUMMY_EVENT_OUT || event == DUMMY_EVENT_RESPAWN) {
			hal::dout << "体" << i + 1 << (event == DUMMY_EVENT_OUT ? "：場外" : "：復活（0%）");
			if (event == DUMMY_EVENT_OUT) {
				// 下の線と横の壁のどちらで場外になったかと、その場所。
				const VEC3& out = m_Fighters[i].GetOutPosition();
				hal::dout << (StageCalcIsOut(m_Stage->GetStage(), out.Y) ? "（下の線）" : "（横の壁）")
					<< " x " << out.X << " y " << out.Y << " z " << out.Z;
			}
			hal::dout << std::endl;
		}
#endif
	}
	UpdateEscape();
	// 燃えた分の % は、燃やした人の攻撃として記録する（場外にしたらその人の得点）。
	for (int i = 0; i < FIGHTER_COUNT; ++i) {
		const STATUS_STEP status = m_Fighters[i].StepStatus(GameTimeGetDeltaTime());
		if (status.Damage <= 0.0f) continue;
		m_Fighters[i].AddDamage(status.Damage);
		if (status.Attacker >= 0 && m_Versus.IsOpponent(status.Attacker, i)) m_Versus.RecordAttack(status.Attacker, i);
	}
	PushBodies();
}

// 体の更新後、掴まれている人の入力だけを使って連打を数える。
void FighterManager::UpdateEscape()
{
	for (int i = 0; i < FIGHTER_COUNT; ++i) {
		Fighter& fighter = m_Fighters[i];
		if (fighter.IsInHitstop()) continue; // ヒットストップで止まっている間は連打を数えない
		if (!fighter.IsHeld()) {
			m_Escapes[i].Reset();
			continue;
		}
		const int presses = int(fighter.IsTrigger(ACTION_JUMP)) + int(fighter.IsTrigger(ACTION_ARM_LEFT))
			+ int(fighter.IsTrigger(ACTION_ARM_RIGHT)) + int(fighter.IsTrigger(ACTION_DASH));
		const ACTION_AXIS move = fighter.GetMoveInput();
		if (!m_Escapes[i].Step(presses, sqrtf(move.X * move.X + move.Y * move.Y), fighter.GetPercent())) continue;
		for (int holder = 0; holder < FIGHTER_COUNT; ++holder) {
			if (fighter.IsHeldBy(holder)) {
				fighter.Release(holder); // ダメージも吹っ飛びも足さず、その場で離す。
				m_Escapes[i].Reset();
				break;
			}
		}
	}
}

void FighterManager::RespawnAtSafePoint(int slot)
{
	const STAGE& stage = m_Stage->GetStage();
	const VEC3 center{ stage.CenterX, stage.TopY, stage.CenterZ };
	VEC3 points[ISLAND_RESPAWN_COUNT + STAGE_PLATFORM_MAX];
	int pointCount = 0;
	if (stage.SpawnCount > 0) {
		// 対戦マップの復活先は P1〜P4 の4点だけ（台帳の順に候補へ渡す）。
		for (int i = 0; i < stage.SpawnCount; ++i) points[pointCount++] = stage.Spawns[i];
	}
	else {
		for (const VEC3& offset : ISLAND_RESPAWN_OFFSETS) points[pointCount++] = Vec3Add(center, Vec3Scale(offset, m_MapScale));
		// ダミー CPU は高台には出さず、低い浮島の上だけから選ぶ。
		const int platformCount = m_Versus.IsHuman(slot) ? stage.PlatformCount : 0;
		for (int i = 0; i < platformCount; ++i) {
			const STAGE_PLATFORM& platform = stage.Platforms[i];
			points[pointCount++] = { platform.CenterX, platform.TopY, platform.CenterZ };
		}
	}

	VEC3 opponents[FIGHTER_COUNT - 1];
	int opponentCount = 0;
	for (int i = 0; i < FIGHTER_COUNT; ++i) {
		if (i == slot || !m_Fighters[i].IsActive()) continue;
		const XMFLOAT3 position = m_Fighters[i].GetPosition();
		opponents[opponentCount++] = { position.x, position.y, position.z };
	}

	const int point = m_Spawn.ChoosePoint(points, pointCount, opponents, opponentCount, SPAWN_SAFE_DISTANCE);
	if (point >= 0) m_Fighters[slot].Teleport(points[point]);
}

void FighterManager::PushBodies()
{
	for (int i = 0; i < FIGHTER_COUNT; ++i) {
		for (int j = i + 1; j < FIGHTER_COUNT; ++j) {
			Fighter& a = m_Fighters[i];
			Fighter& b = m_Fighters[j];
			if (!a.CanPushBodies() || !b.CanPushBodies() || a.IsPulled() || b.IsPulled()) continue;
			const XMFLOAT3 pa = a.GetPosition(), pb = b.GetPosition();
			VEC3 first{ pa.x, pa.y, pa.z }, second{ pb.x, pb.y, pb.z };
			if (VersusCalcPushBodies(first, second, a.IsHuman(), b.IsHuman(), a.GetBody().Radius, a.GetHeight())) {
				// 押された横の分は、地形の側面（仕分け台・屋根など）で止める。押し込まれて台の中に入らないように。
				const STAGE& stage = m_Stage->GetStage();
				StageCalcMoveBody(stage, pa.x, first.Y, pa.z, first.X - pa.x, first.Z - pa.z, a.IsOnGround(), first.X, first.Z);
				StageCalcMoveBody(stage, pb.x, second.Y, pb.z, second.X - pb.x, second.Z - pb.z, b.IsOnGround(), second.X, second.Z);
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

void FighterManager::UpdateAnimation()
{
	const float dt = GameTimeGetDeltaTime();
	for (int i = 0; i < FIGHTER_COUNT; ++i) {
		if (!m_Fighters[i].IsInHitstop()) m_Fighters[i].UpdateAnimation(m_Arms[i], dt);
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

TENTACLE_TARGET FighterManager::GetMeleeTarget(int index, int self) const
{
	if (!m_Versus.IsOpponent(self, index) || !IsActive(index)) return {};
	return { m_Fighters[index].GetBody(), true };
}

bool FighterManager::CanAttack(int index, int attacker)
{
	if (!m_Versus.IsOpponent(attacker, index) || !IsActive(attacker) || !IsActive(index)) return false;
	// 復活したあとの無敵時間中は攻撃を受けない。
	if (m_Fighters[index].IsInvincible()) return false;
	// ガードされた攻撃は効かない（No.80）。燃える % と投げ（Release）はここを通らない。
	if (IsGuarded(index, attacker)) {
		m_GuardedHit[index] = true;
		m_GuardedBy[index] = attacker;
		return false;
	}
	return true;
}

int FighterManager::GetGuardedAttacker(int index) const
{
	return index >= 0 && index < FIGHTER_COUNT ? m_GuardedBy[index] : -1;
}

void FighterManager::StartHitstop(int index, int frames, bool shake)
{
	if (index < 0 || index >= FIGHTER_COUNT) return;
	m_Fighters[index].StartHitstop(frames, shake);
}

void FighterManager::AddShake(int index, float strength)
{
	if (index < 0 || index >= FIGHTER_COUNT || strength <= 0.0f) return;
	m_PendingShake[index] = fminf(1.0f, m_PendingShake[index] + strength); // カメラの揺れも上限 1
}

void FighterManager::SpawnEffect(EFFECT_ID id, const VEC3& position, const VEC3& direction, const PARTICLE_COLOR* tint)
{
	if (m_Effects != nullptr) m_Effects->Spawn(id, position, direction, tint);
}

VEC3 FighterManager::GetBodyPoint(int index, float heightRate) const
{
	const XMFLOAT3 feet = m_Fighters[index].GetPosition();
	return { feet.x, feet.y + m_Fighters[index].GetHeight() * heightRate, feet.z };
}

PARTICLE_COLOR FighterManager::GetEffectColor(int index) const
{
	const XMFLOAT4 color = Fighter::GetPlayerColor(index);
	return { color.x, color.y, color.z, color.w };
}

void FighterManager::UpdateDamageEffects(int index, float dt)
{
	const DAMAGE_SMOKE smoke = DamageColorCalcSmoke(m_Fighters[index].GetPercent());
	if (smoke == DAMAGE_SMOKE_NONE || !m_Fighters[index].IsActive()) {
		m_SmokeSeconds[index] = 0.0f;
		return;
	}
	m_SmokeSeconds[index] -= dt;
	if (m_SmokeSeconds[index] > 0.0f) return;
	const bool black = smoke == DAMAGE_SMOKE_BLACK;
	m_SmokeSeconds[index] = black ? SMOKE_BLACK_SECONDS : SMOKE_GRAY_SECONDS;
	const VEC3 point = GetBodyPoint(index, SMOKE_HEIGHT);
	SpawnEffect(black ? EFFECT_SMOKE_BLACK : EFFECT_SMOKE_GRAY, point, { 0.0f, 1.0f, 0.0f });
	if (black && ++m_SmokeCount[index] % SMOKE_SPARK_EVERY == 0) SpawnEffect(EFFECT_SPARK_TINY, point, { 0.0f, 1.0f, 0.0f });
}

// 当たった所（当てられた人の胸の、当てた人の側）に、強さに合わせた火花・輪・光の筋を出す。
void FighterManager::SpawnHitEffects(int index, int attacker, const VEC3& direction, int frames)
{
	const XMFLOAT3 feet = m_Fighters[index].GetPosition();
	VEC3 point{ feet.x, feet.y + m_Fighters[index].GetHeight() * HIT_EFFECT_HEIGHT, feet.z };
	const XMFLOAT3 from = m_Fighters[attacker].GetPosition();
	const VEC3 toward{ from.x - feet.x, 0.0f, from.z - feet.z };
	const float length = Vec3Length(toward);
	if (length > 1.0e-4f) point = Vec3Add(point, Vec3Scale(toward, HIT_EFFECT_FORWARD / length));
	if (frames >= HITSTOP.HeavyFrames) {
		SpawnEffect(EFFECT_HIT_SPARK_HEAVY, point, direction);
		SpawnEffect(EFFECT_HIT_RING_LARGE, point, direction);
		if (frames >= HITSTOP.FinishFrames) SpawnEffect(EFFECT_FINISH_STREAK, point, direction);
	}
	else if (frames >= HITSTOP.MediumFrames) {
		SpawnEffect(EFFECT_HIT_SPARK_MEDIUM, point, direction);
		SpawnEffect(EFFECT_HIT_RING, point, direction);
	}
	else SpawnEffect(EFFECT_HIT_SPARK_LIGHT, point, direction);
}

void FighterManager::AddScreenFlash(int index, const SCREEN_FLASH& flash)
{
	if (index < 0 || index >= FIGHTER_COUNT) return;
	m_PendingFlash[index].Edge = fminf(1.0f, m_PendingFlash[index].Edge + flash.Edge);
	m_PendingFlash[index].Full = fminf(1.0f, m_PendingFlash[index].Full + flash.Full);
}

SCREEN_FLASH FighterManager::TakeScreenFlash(int index)
{
	if (index < 0 || index >= FIGHTER_COUNT) return {};
	const SCREEN_FLASH flash = m_PendingFlash[index];
	m_PendingFlash[index] = {};
	return flash;
}

float FighterManager::TakeShake(int index)
{
	if (index < 0 || index >= FIGHTER_COUNT) return 0.0f;
	const float shake = m_PendingShake[index];
	m_PendingShake[index] = 0.0f;
	return shake;
}

bool FighterManager::ConsumeGuardedHit(int index)
{
	if (index < 0 || index >= FIGHTER_COUNT) return false;
	const bool guarded = m_GuardedHit[index];
	m_GuardedHit[index] = false;
	return guarded;
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
	m_Escapes[index].Reset(); // 同じフレームで掴まれ直した場合も前の回数を引き継がない。
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

bool FighterManager::ApplyHazard(int index, int attacker, STATUS_TYPE type, float seconds, float value, float damage)
{
	// 置いた人が復活待ちでも、残っている茨は効く。
	if (!m_Versus.IsOpponent(attacker, index) || !IsActive(index)) return false;
	m_Fighters[index].ApplyStatus(type, seconds, value, attacker);
	if (damage > 0.0f) {
		m_Fighters[index].AddDamage(damage);
		m_Versus.RecordAttack(attacker, index);
	}
	return true;
}

bool FighterManager::AddDamage(int index, int attacker, float damage)
{
	if (!CanAttack(index, attacker)) return false;
	m_Fighters[index].AddDamage(damage);
	m_Versus.RecordAttack(attacker, index);
	return true;
}

void FighterManager::Hit(int index, int attacker, const XMFLOAT3& direction, float damage, float power)
{
	if (!CanAttack(index, attacker)) return;
	m_Fighters[index].Hit({ direction.x, direction.y, direction.z }, damage, power);
	m_Versus.RecordAttack(attacker, index);
}

bool FighterManager::Launch(int index, int attacker, const VEC3& direction, float damage, float power)
{
	if (!CanAttack(index, attacker)) return false;
	// つかまれている相手は飛ばさず、ダメージだけ足す（つかんでいる側が殴り続けて投げられる）。
	float speed = 0.0f;
	if (m_Fighters[index].IsHeld()) m_Fighters[index].AddDamage(damage);
	else speed = m_Fighters[index].Launch(direction, damage, power);
	const int frames = HitstopCalcFrames(damage, speed, HITSTOP);
	StartHitstop(index, frames, true);
	StartHitstop(attacker, frames, false);
	const float shake = frames >= HITSTOP.FinishFrames ? SHAKE_FINISH : frames >= HITSTOP.HeavyFrames ? SHAKE_HEAVY
		: frames >= HITSTOP.MediumFrames ? SHAKE_MEDIUM : 0.0f;
	AddShake(index, shake);
	AddShake(attacker, shake);
	if (frames >= HITSTOP.HeavyFrames) {
		const SCREEN_FLASH edge{ frames >= HITSTOP.FinishFrames ? EDGE_FLASH_FINISH : EDGE_FLASH_HEAVY, 0.0f };
		AddScreenFlash(index, edge);
		AddScreenFlash(attacker, edge);
	}
	SpawnHitEffects(index, attacker, direction, frames);
	m_Versus.RecordAttack(attacker, index);
	return true;
}

void FighterManager::Push(int index, int attacker, const VEC3& velocity)
{
	if (!CanAttack(index, attacker) || m_Fighters[index].IsHeld()) return;
	m_Fighters[index].Push(velocity);
	m_Versus.RecordAttack(attacker, index);
}
