#include "game/arm.h"
#include "game/fighter.h"
#include "game/fightermanager.h"
#include "core/gametime.h"
#include "game/arm_bare.h"
#include "game/arm_hammer.h"
#include "game/arm_knuckle.h"
#include "game/arm_c4.h"
#include "game/arm_gunlance.h"
#include "game/arm_katana.h"
#include "game/tentacle.h"
#include "input/pad.h"
#include "game/camera.h"
#include "game/stage.h"
#include "physics/areacalc.h"
#include "physics/armcalc.h"
#include "physics/grapplecalc.h"
#include "physics/tentaclecalc.h"
#include "render/model.h"
#include "render/shape.h"
#include <memory>

#ifdef _DEBUG
#include "core/debug_ostream.h"
#include <iostream>
#endif

// 調整用の値（今後追加する腕も、取付位置と待機方向はここを使う）
static constexpr float ARM_REST_PITCH = XMConvertToRadians(-75.0f); // 待機の腕を正面から 75° 下げる（体の横に垂らす）
static constexpr float ARM_REST_SPREAD = XMConvertToRadians(8.0f); // 片側ずつ外へ開く角度
// メカの胸の高さと横幅に合わせ、Fighter の足元を基準に固定する。
static constexpr XMFLOAT4 LEFT_COLOR{ 0.8f, 0.3f, 0.65f, 1.0f };
static constexpr XMFLOAT4 RIGHT_COLOR{ 0.35f, 0.8f, 0.6f, 1.0f };
static constexpr XMFLOAT4 HOLD_COLOR{ 1.0f, 0.7f, 0.2f, 1.0f };
static constexpr float STUB_RADIUS = 0.06f; // モデルのない腕の円柱の半径。
static constexpr int RANGE_RING_POINTS = 32; // 当たる範囲の円に並べる球の数。
static constexpr float RANGE_RING_RADIUS = 0.08f; // 当たる範囲の円に並べる球の半径。
static constexpr ACTION ARM_ACTIONS[] = { ACTION_ARM_LEFT, ACTION_ARM_RIGHT };
static constexpr ACTION GUARD_ACTIONS[] = { ACTION_GUARD_LEFT, ACTION_GUARD_RIGHT };
static constexpr float GUARD_HALF_ANGLE = XMConvertToRadians(80.0f); // 前の左右それぞれの角度
static constexpr float GUARD_MOVE_SCALE = 0.5f; // 構えている間の歩く速さ
static constexpr float GUARD_MARKER_FORWARD = 0.5f; // 肩の前に球を出す距離
static constexpr float GUARD_MARKER_RADIUS = 0.2f;
static constexpr XMFLOAT4 GUARD_COLOR{ 0.3f, 0.6f, 0.9f, 0.8f };
static constexpr XMFLOAT4 PARRY_COLOR{ 1.0f, 1.0f, 0.6f, 1.0f };
static constexpr float PARRY_RUMBLE = 1.0f;
static constexpr float PARRY_RUMBLE_SECONDS = 0.1f;
static constexpr float BREAK_RUMBLE = 0.8f;
static constexpr float BREAK_RUMBLE_SECONDS = 0.3f;
// ARM_TYPE の順に、触手・ハンマー・ナックル・C4・ガンランス・刀・素手。
static constexpr int ARM_MAX_USES[] = { 3, 2, 3, 2, 8, 3, 0 };

static_assert(ARM_TYPE_COUNT <= ACTION_EQUIP_NUMBER_COUNT, "腕の番号は通信で送れる数までにする");
static_assert(sizeof(ARM_MAX_USES) / sizeof(ARM_MAX_USES[0]) == ARM_TYPE_COUNT, "腕ごとに使用回数を設定する");

template<class T>
static std::unique_ptr<Arm> CreateArm(int hand, const ARM_LINKS& links)
{
	return std::make_unique<T>(hand, links);
}

// 新しい腕は Arm を派生し、この1行を CreateArm<腕のクラス> にする。
// nullptr の種類は読み込まず、数字キーでも選べない。
using ArmFactory = std::unique_ptr<Arm> (*)(int hand, const ARM_LINKS& links);
static const ArmFactory ARM_FACTORIES[ARM_TYPE_COUNT] = {
	CreateArm<TentacleArm>, // 触手
	CreateArm<HammerArm>, // ハンマー
	CreateArm<KnuckleArm>, // ナックル
	CreateArm<C4Arm>, // C4
	CreateArm<GunlanceArm>, // ガンランス
	CreateArm<KatanaArm>, // 刀
	CreateArm<BareArm>, // 素手
};
static const char* const ARM_NAMES[ARM_TYPE_COUNT] = {
	"触手", "ハンマー", "ナックル", "C4", "ガンランス", "刀", "素手"
};

void ArmManager::ChangeType(int hand, int next, int direction)
{
	if (next < 0 || next >= ARM_TYPE_COUNT || ARM_FACTORIES[next] == nullptr || next == m_Types[hand]) return;
	m_Arms[hand][m_Types[hand]]->Reset();
	m_Guards[hand].Step(false, 0.0f);
	const Fighter& player = m_Links.Fighters->GetFighter(m_Links.Slot);
	const ACTION action = ARM_ACTIONS[hand];
	// 前の装備で押し始めた入力は持ち越さない。
	m_Presses[hand].Reset(player.IsPress(action) && !player.IsTrigger(action));
	m_Types[hand] = (ARM_TYPE)next;
	m_Uses[hand].Reset(TuningCalcArmUses(ARM_MAX_USES[next], m_Tuning));
	m_ChangeDirections[hand] = direction;
#ifdef _DEBUG
	hal::dout << "腕：" << (hand == 0 ? "左" : "右") << " → " << ArmGetName(m_Types[hand]) << std::endl;
#endif
}

void ArmManager::Initialize(const Stage& stage, FighterManager& fighters, int slot, const Camera& camera)
{
	m_Links = { &fighters, slot, &camera, &stage, this };
	m_Rule = GAME_RULE_DEBUG;
	m_Types[0] = m_Types[1] = ARM_TYPE_TENTACLE;
	m_SelectedHand = 0;
	m_ChangeDirections[0] = m_ChangeDirections[1] = 1;
	for (int type = 0; type < ARM_TYPE_COUNT; ++type) {
		if (ARM_FACTORIES[type] == nullptr) continue;
		for (int hand = 0; hand < 2; ++hand) m_Arms[hand][type] = ARM_FACTORIES[type](hand, m_Links);
	}
	// 足場への引っぱりは両手で同時に行わず、後から付いた手へ受け持ちを渡す。
	static_cast<TentacleArm&>(*m_Arms[0][ARM_TYPE_TENTACLE]).ConnectOtherArm(static_cast<TentacleArm&>(*m_Arms[1][ARM_TYPE_TENTACLE]));
	Reset();
}

void ArmManager::Finalize(void)
{
	for (int type = 0; type < ARM_TYPE_COUNT; ++type) {
		for (int hand = 0; hand < 2; ++hand) m_Arms[hand][type].reset();
	}
}

void ArmManager::Reset(void)
{
	const Fighter& player = m_Links.Fighters->GetFighter(m_Links.Slot);
	for (int hand = 0; hand < 2; ++hand) {
		m_Arms[hand][m_Types[hand]]->Reset();
		BreakExhaustedArm(hand);
		m_Presses[hand].Reset(player.IsPress(ARM_ACTIONS[hand]));
		m_Guards[hand].Reset();
	}
	m_LastActiveHand = -1;
	m_GuardHand = -1;
}

void ArmManager::ResetForMatch(GAME_RULE rule, const MATCH_TUNING& tuning)
{
	m_Rule = rule;
	m_Tuning = tuning;
	for (int hand = 0; hand < 2; ++hand) {
		if (m_Rule == GAME_RULE_GAME) ChangeType(hand, ARM_TYPE_NONE);
		else if (m_Types[hand] == ARM_TYPE_NONE) ChangeType(hand, ARM_TYPE_TENTACLE);
	}
	Reset();
}

void ArmManager::ResetEquip(void)
{
	for (int hand = 0; hand < 2; ++hand) ChangeType(hand, ARM_TYPE_TENTACLE);
	m_SelectedHand = 0;
}

bool ArmManager::TryEquipPickup(ARM_TYPE type)
{
	if (m_Rule != GAME_RULE_GAME || type < 0 || type >= ARM_TYPE_NONE || ARM_FACTORIES[type] == nullptr) return false;
	for (int hand = 0; hand < 2; ++hand) {
		if (m_Types[hand] != ARM_TYPE_NONE) continue;
		ChangeType(hand, type);
		return true;
	}
	return false;
}

void ArmManager::UpdateEquip(const Fighter& player)
{
	bool available[ARM_TYPE_COUNT];
	for (int type = 0; type < ARM_TYPE_COUNT; ++type) available[type] = ARM_FACTORIES[type] != nullptr;
	available[ARM_TYPE_NONE] = false;
	if (player.IsTrigger(ACTION_EQUIP_HAND_LEFT)) m_SelectedHand = 0;
	if (player.IsTrigger(ACTION_EQUIP_HAND_RIGHT)) m_SelectedHand = 1;
	const int direction = int(player.IsTrigger(ACTION_EQUIP_NEXT)) - int(player.IsTrigger(ACTION_EQUIP_PREV));
	const ACTION_EQUIP_KEY key = player.GetEquipInput();
	const int direct = ArmCalcSelect(key.Number, available, ARM_TYPE_COUNT);
	if (direction != 0 && (key.Hand != m_SelectedHand || direct < 0)) {
		ChangeType(m_SelectedHand, ArmCalcCycle(m_Types[m_SelectedHand], direction, available, ARM_TYPE_COUNT), direction);
	}
	for (int hand = 0; hand < 2; ++hand) {
		if (key.Hand == hand) ChangeType(hand, direct);
	}
}

bool ArmManager::HasUnlimitedUses() const
{
	return m_Rule == GAME_RULE_DEBUG || TuningCalcIsUnlimitedUses(m_Tuning);
}

void ArmManager::BreakExhaustedArm(int hand)
{
	if (HasUnlimitedUses() || m_Types[hand] == ARM_TYPE_NONE || !m_Uses[hand].IsExhausted()) return;
	const Arm& arm = *m_Arms[hand][m_Types[hand]];
	if (!arm.IsActive() && !arm.IsBusy()) ChangeType(hand, ARM_TYPE_NONE);
}

// 前の更新から防いだ攻撃を処理してから、このフレームの構えを決める。
void ArmManager::UpdateGuards(const Fighter& player, float dt, bool interrupted)
{
	const bool hadGuard = m_GuardHand >= 0 && m_Guards[m_GuardHand].IsGuarding();
	if (hadGuard && m_Links.Fighters->ConsumeGuardedHit(m_Links.Slot)) {
		const int hand = m_GuardHand;
		const PARRY_RESULT result = m_Guards[hand].OnHit(true);
		Arm& arm = *m_Arms[hand][m_Types[hand]];
		if (result == PARRY_SUCCESS) arm.Rumble(PARRY_RUMBLE, PARRY_RUMBLE, PARRY_RUMBLE_SECONDS);
		else if (result == PARRY_BREAK_ARM && !HasUnlimitedUses()) {
			arm.Rumble(BREAK_RUMBLE, BREAK_RUMBLE, BREAK_RUMBLE_SECONDS);
			ChangeType(hand, ARM_TYPE_NONE);
		}
	}
	bool any = false;
	for (int hand = 0; hand < 2; ++hand) {
		const Arm& arm = *m_Arms[hand][m_Types[hand]];
		const bool wasGuarding = m_Guards[hand].IsGuarding();
		const bool guarding = !interrupted && player.IsPress(GUARD_ACTIONS[hand])
			&& m_Types[hand] != ARM_TYPE_NONE && !arm.IsActive() && !arm.IsBusy();
		m_Guards[hand].Step(guarding, dt);
		if (guarding && !wasGuarding) {
			// 刀の無敵など、構える前に防いだ記録を今回のガードへ持ち越さない。
			if (!hadGuard && !any) m_Links.Fighters->ConsumeGuardedHit(m_Links.Slot);
			m_GuardHand = hand;
		}
		any = any || guarding;
	}
	if (!any) m_GuardHand = -1;
	else if (m_GuardHand < 0 || !m_Guards[m_GuardHand].IsGuarding()) m_GuardHand = m_Guards[1].IsGuarding() ? 1 : 0;
}

void ArmManager::Update(bool playing)
{
	const Fighter& player = m_Links.Fighters->GetFighter(m_Links.Slot);
	if (m_Rule == GAME_RULE_DEBUG) UpdateEquip(player);
	if (!playing) return;
	// GAME の復活時の装備リセットは、カプセルの取得前に Game が済ませる。
	if (m_Rule == GAME_RULE_GAME && player.DidRespawn()) return;
	// 中断中に押していた入力は、使えるようになっても一度離すまで引き継がない。
	const bool interrupted = !player.CanUseArms() || player.DidRespawn() || player.IsStatus(STATUS_STUN);
	if (interrupted) m_LastActiveHand = -1;
	const float dt = GameTimeGetDeltaTime();
	UpdateGuards(player, dt, interrupted);
	for (int hand = 0; hand < 2; ++hand) {
		Arm& arm = *m_Arms[hand][m_Types[hand]];
		const bool pressed = player.IsPress(ARM_ACTIONS[hand]);
		if (interrupted) {
			arm.Interrupt();
			m_Presses[hand].Reset(pressed);
		}
		else {
			const bool wasActive = arm.IsActive();
			ARM_INPUT input{};
			if (m_Guards[hand].IsGuarding()) m_Presses[hand].Reset(pressed);
			else if (HasUnlimitedUses() || !m_Uses[hand].IsExhausted() || arm.IsHolding()) { // 最後の1回でつかんだ相手も投げられるようにする。
				input = m_Presses[hand].Step(pressed, dt, arm.CanTakeTap(), arm.CanTakeHold());
			}
			else m_Presses[hand].Reset(pressed);
			arm.Update(input);
			if (!wasActive && arm.IsActive()) m_LastActiveHand = hand;
		}
		if (!HasUnlimitedUses()) m_Uses[hand].Consume(arm.m_Used);
		arm.m_Used = 0;
		BreakExhaustedArm(hand);
	}
}

void ArmManager::Draw(void) const
{
	if (!m_Links.Fighters->IsActive(m_Links.Slot)) return;
	for (int hand = 0; hand < 2; ++hand) m_Arms[hand][m_Types[hand]]->Draw();
	const Fighter& player = m_Links.Fighters->GetFighter(m_Links.Slot);
	for (int hand = 0; hand < 2; ++hand) {
		if (!m_Guards[hand].IsGuarding()) continue;
		const VEC3 shoulder = m_Arms[hand][m_Types[hand]]->GetDrawShoulder();
		ShapeDrawSphere({ shoulder.X + sinf(player.GetYaw()) * GUARD_MARKER_FORWARD, shoulder.Y,
			shoulder.Z + cosf(player.GetYaw()) * GUARD_MARKER_FORWARD }, GUARD_MARKER_RADIUS,
			m_Guards[hand].IsParryWindow() ? PARRY_COLOR : GUARD_COLOR);
	}
}

bool ArmManager::IsActive(void) const
{
	for (int hand = 0; hand < 2; ++hand) {
		if (m_Arms[hand][m_Types[hand]]->IsActive()) return true;
	}
	return false;
}

bool ArmManager::IsHolding(void) const
{
	for (int hand = 0; hand < 2; ++hand) {
		if (m_Arms[hand][m_Types[hand]]->IsHolding()) return true;
	}
	return false;
}

int ArmManager::GetActiveHand() const
{
	if (m_LastActiveHand >= 0 && m_Arms[m_LastActiveHand][m_Types[m_LastActiveHand]]->IsActive()) return m_LastActiveHand;
	for (int hand = 0; hand < 2; ++hand) {
		if (m_Arms[hand][m_Types[hand]]->IsActive()) return hand;
	}
	return -1;
}

ARM_BODY_CLIP ArmManager::GetBodyClip(int hand) const
{
	return hand == 0 || hand == 1 ? m_Arms[hand][m_Types[hand]]->GetBodyClip() : ARM_BODY_CLIP{};
}

ARM_TYPE ArmManager::GetType(int hand) const
{
	return hand >= 0 && hand < 2 ? m_Types[hand] : ARM_TYPE_COUNT;
}

int ArmManager::GetRemainingUses(int hand) const
{
	if (hand < 0 || hand >= 2) return 0;
	return HasUnlimitedUses() ? -1 : m_Uses[hand].GetRemaining();
}

int ArmManager::GetChangeDirection(int hand) const
{
	return hand == 0 || hand == 1 ? m_ChangeDirections[hand] : 1;
}

float ArmManager::GetMoveScale() const
{
	const float scale = fminf(m_Arms[0][m_Types[0]]->GetMoveScale(), m_Arms[1][m_Types[1]]->GetMoveScale());
	return m_GuardHand >= 0 ? fminf(scale, GUARD_MOVE_SCALE) : scale;
}

GUARD ArmManager::GetGuard() const
{
	const GUARD native = GuardCalcStronger(m_Arms[0][m_Types[0]]->GetGuard(), m_Arms[1][m_Types[1]]->GetGuard());
	const Fighter& player = m_Links.Fighters->GetFighter(m_Links.Slot);
	if (m_GuardHand < 0 || !player.CanUseArms() || player.IsStatus(STATUS_STUN)) return native;
	GUARD guard;
	guard.Kind = GUARD_FRONT;
	guard.Forward = { sinf(player.GetYaw()), 0.0f, cosf(player.GetYaw()) };
	guard.HalfAngle = GUARD_HALF_ANGLE;
	return GuardCalcStronger(native, guard);
}

VEC3 ArmManager::GetRushVelocity() const
{
	const int first = m_LastActiveHand == 1 ? 1 : 0;
	const VEC3 velocity = m_Arms[first][m_Types[first]]->GetRushVelocity();
	if (velocity.X != 0.0f || velocity.Z != 0.0f) return velocity;
	const int other = 1 - first;
	return m_Arms[other][m_Types[other]]->GetRushVelocity();
}

const char* ArmGetName(ARM_TYPE type)
{
	return type >= 0 && type < ARM_TYPE_COUNT ? ARM_NAMES[type] : "不明";
}

void Arm::ConsumeUse(int count)
{
	if (GetRemainingUses() >= 0) m_Used += count;
}

int Arm::GetRemainingUses() const
{
	return m_Links.Arms->GetRemainingUses(m_Hand);
}

const MATCH_TUNING& Arm::GetTuning() const
{
	return m_Links.Arms->GetTuning();
}

Fighter& Arm::GetPlayer() const
{
	return GetFighters().GetFighter(GetSlot());
}

VEC3 Arm::GetShoulder() const
{
	// 攻撃の起点はキャラによらず従来のメカの肩を使う。
	return GetShoulderAt(CHARACTER_DATA::Get(CHARACTER_MEKA).Shoulder);
}

VEC3 Arm::GetDrawShoulder() const
{
	VEC3 shoulder, direction;
	float length = 0.0f;
	if (!GetPlayer().GetArmBone(GetHand(), shoulder, direction, length)) {
		return GetShoulderAt(CHARACTER_DATA::Get(GetPlayer().GetCharacter()).Shoulder);
	}
	// アニメの肩の骨に、腕ごとの取付のずらしをキャラの腕の大きさに合わせて足す（描画だけ）。
	const VEC3 mount = Vec3Scale(GetMountOffset(), GetDrawScale());
	const float outward = CHARACTER_DATA::Get(GetPlayer().GetCharacter()).ArmOutward;
	return ArmCalcShoulder(shoulder, GetPlayer().GetYaw(), GetHand(), mount.X + outward, mount.Y, mount.Z);
}

void Arm::BlendClipPose(VEC3& arm, VEC3& weapon) const
{
	VEC3 clipArm, clipWeapon;
	bool own = false;
	const float weight = GetPlayer().GetAttackArm(GetHand(), clipArm, clipWeapon, own);
	if (weight <= 0.0f) return;
	if (own) {
		arm = ArmCalcBlendDirection(arm, clipArm, weight);
		weapon = ArmCalcBlendDirection(weapon, clipWeapon, weight);
		return;
	}
	// もう一方の手の攻撃につられるときは、武器は今の持ち方のまま腕の動いた分だけ向きを変える。
	const VEC3 movedArm = ArmCalcBlendDirection(arm, clipArm, weight);
	weapon = ArmCalcBlendDirection(weapon, Vec3Add(weapon, Vec3Sub(movedArm, arm)), 1.0f);
	arm = movedArm;
}

float Arm::GetDrawScale() const
{
	return CHARACTER_DATA::Get(GetPlayer().GetCharacter()).ArmScale;
}

VEC3 Arm::GetShoulderAt(const VEC3& offset) const
{
	const int hand = GetHand();
	const XMFLOAT3 p = GetPlayer().GetPosition();
	const VEC3 mount = GetMountOffset();
	return ArmCalcShoulder({ p.x, p.y, p.z }, GetPlayer().GetYaw(), hand,
		offset.X + mount.X, offset.Y + mount.Y, offset.Z + mount.Z);
}

VEC3 Arm::GetRestDirection(float pitchOffset) const
{
	return ArmCalcRestDirection(GetPlayer().GetYaw(), GetHand(), ARM_REST_PITCH + pitchOffset, ARM_REST_SPREAD);
}

VEC3 Arm::GetForward() const
{
	const float yaw = GetPlayer().GetYaw();
	return { sinf(yaw), 0.0f, cosf(yaw) };
}

int Arm::GetAimTarget() const
{
	const int target = GetCamera().GetTarget();
	if (target < 0 || !GetFighters().GetTarget(target, GetSlot()).Available) return -1;
	return target;
}

VEC3 Arm::GetAim(bool keepHeight) const
{
	const float yaw = GetCamera().GetYaw();
	const VEC3 fallback{ sinf(yaw), 0.0f, cosf(yaw) };
	const int target = GetAimTarget();
	VEC3 center{};
	if (target >= 0) {
		const CAPSULE body = GetFighters().GetTarget(target, GetSlot()).Body;
		center = Vec3Scale(Vec3Add(body.Start, body.End), 0.5f);
	}
	// TentacleCalcAim は高さを捨てて水平にし、GrappleCalcAim は上下も含める。
	return keepHeight ? GrappleCalcAim(GetShoulder(), center, target >= 0, fallback)
		: TentacleCalcAim(GetShoulder(), center, target >= 0, fallback);
}

VEC3 Arm::DrawStubArm(const VEC3& direction, float length, const XMFLOAT4& color) const
{
	const VEC3 shoulder = GetDrawShoulder();
	const VEC3 hand = Vec3Add(shoulder, Vec3Scale(direction, length));
	ShapeDrawCylinder({ shoulder.X, shoulder.Y, shoulder.Z }, { hand.X, hand.Y, hand.Z }, STUB_RADIUS, color);
	return hand;
}

int Arm::Explode(const VEC3& center, float radius, float damage, float power, float up, int* affected) const
{
	int hits = 0;
	for (int i = 0; i < FIGHTER_COUNT; ++i) {
		const TENTACLE_TARGET target = GetFighters().GetTarget(i, GetSlot());
		if (!target.Available || !CollisionSphereCapsule({ center, radius }, target.Body).Hit) continue;
		// 浮島や高台をはさんだ向こう（高台の下で爆発したときの上の相手など）には届かない。
		const VEC3 bodyCenter = Vec3Scale(Vec3Add(target.Body.Start, target.Body.End), 0.5f);
		if (StageCalcIsBlocked(GetStage().GetStage(), center, bodyCenter)) continue;
		const bool applied = GetFighters().Launch(i, GetSlot(), AreaCalcRadialDirection(center, target.Body, up, GetForward()), damage, power);
		if (applied && affected != nullptr) ++*affected;
		++hits;
	}
	return hits;
}

XMFLOAT4 Arm::GetColor(bool active) const
{
	return active ? HOLD_COLOR : (GetHand() == 0 ? LEFT_COLOR : RIGHT_COLOR);
}

void Arm::DrawPiece(int model, VEC3 start, VEC3 end, float sourceLength, float thickness, const XMFLOAT4& color, float roll,
	const VEC3& down) const
{
	const TENTACLE_PIECE_AXES axes = ArmCalcRollAxes(TentacleCalcPieceAxes(start, end, down), roll);
	const VEC3 x = Vec3Scale(axes.X, thickness);
	const VEC3 y = Vec3Scale(axes.Y, Vec3Length(Vec3Sub(end, start)) / sourceLength);
	const VEC3 z = Vec3Scale(axes.Z, thickness);
	ModelDraw(model, { start.X, start.Y, start.Z }, { x.X, x.Y, x.Z }, { y.X, y.Y, y.Z }, { z.X, z.Y, z.Z }, color);
}

VEC3 Arm::DrawAlong(int model, const VEC3& start, const VEC3& direction, float sourceLength, float scale,
	const XMFLOAT4& color, float roll) const
{
	const VEC3 end = Vec3Add(start, Vec3Scale(direction, sourceLength * scale));
	if (model >= 0) DrawPiece(model, start, end, sourceLength, scale, color, roll);
	else ShapeDrawCylinder({ start.X, start.Y, start.Z }, { end.X, end.Y, end.Z }, STUB_RADIUS, color);
	return end;
}

void Arm::DrawRangeRing(const VEC3& center, float radius, const XMFLOAT4& color) const
{
	for (int i = 0; i < RANGE_RING_POINTS; ++i) {
		const float angle = XM_2PI * i / RANGE_RING_POINTS;
		ShapeDrawSphere({ center.X + sinf(angle) * radius, center.Y, center.Z + cosf(angle) * radius }, RANGE_RING_RADIUS, color);
	}
}

int Arm::LoadModel(const char* path) const
{
	return ModelLoad(path, GetHand() == 1);
}

void Arm::Rumble(float smallMotor, float largeMotor, float seconds) const
{
	const int device = GetPlayer().GetDevice();
	PadRumble(device == VERSUS_DEVICE_COMBINED ? 0 : device, smallMotor, largeMotor, seconds);
}
