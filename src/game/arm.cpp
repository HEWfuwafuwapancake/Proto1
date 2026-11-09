#include "game/arm.h"
#include "game/fighter.h"
#include "game/fightermanager.h"
#include "core/gametime.h"
#include "game/arm_hammer.h"
#include "game/tentacle.h"
#include "input/pad.h"
#include "game/camera.h"
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

static_assert(ARM_TYPE_COUNT <= ACTION_EQUIP_NUMBER_COUNT, "腕の番号は通信で送れる数までにする");

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
	nullptr, // ナックル（No.97）
	nullptr, // C4（No.98）
	nullptr, // ガンランス（No.99）
	nullptr, // 刀（No.100）
};
static const char* const ARM_NAMES[ARM_TYPE_COUNT] = {
	"触手", "ハンマー", "ナックル", "C4", "ガンランス", "刀"
};

void ArmManager::ChangeType(int hand, int next, int direction)
{
	if (next < 0 || next >= ARM_TYPE_COUNT || ARM_FACTORIES[next] == nullptr || next == m_Types[hand]) return;
	m_Arms[hand][m_Types[hand]]->Reset();
	const Fighter& player = m_Links.Fighters->GetFighter(m_Links.Slot);
	const ACTION action = ARM_ACTIONS[hand];
	// 前の装備で押し始めた入力は持ち越さない。
	m_Presses[hand].Reset(player.IsPress(action) && !player.IsTrigger(action));
	m_Types[hand] = (ARM_TYPE)next;
	m_ChangeDirections[hand] = direction;
#ifdef _DEBUG
	hal::dout << "腕：" << (hand == 0 ? "左" : "右") << " → " << ArmGetName(m_Types[hand]) << std::endl;
#endif
}

void ArmManager::Initialize(const Stage& stage, FighterManager& fighters, int slot, const Camera& camera)
{
	m_Links = { &fighters, slot, &camera, &stage };
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
		m_Presses[hand].Reset(player.IsPress(ARM_ACTIONS[hand]));
	}
	m_LastActiveHand = -1;
}

void ArmManager::ResetEquip(void)
{
	for (int hand = 0; hand < 2; ++hand) ChangeType(hand, ARM_TYPE_TENTACLE);
	m_SelectedHand = 0;
}

void ArmManager::Update(bool playing)
{
	bool available[ARM_TYPE_COUNT];
	for (int type = 0; type < ARM_TYPE_COUNT; ++type) available[type] = ARM_FACTORIES[type] != nullptr;
	const Fighter& player = m_Links.Fighters->GetFighter(m_Links.Slot);
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
	if (!playing) return;
	// 中断中に押していた入力は、使えるようになっても一度離すまで引き継がない。
	if (!player.CanUseArms() || player.DidRespawn() || player.IsStatus(STATUS_STUN)) {
		for (int hand = 0; hand < 2; ++hand) {
			m_Arms[hand][m_Types[hand]]->Interrupt();
			m_Presses[hand].Reset(player.IsPress(ARM_ACTIONS[hand]));
		}
		m_LastActiveHand = -1;
		return;
	}
	const float dt = GameTimeGetDeltaTime();
	for (int hand = 0; hand < 2; ++hand) {
		Arm& arm = *m_Arms[hand][m_Types[hand]];
		const bool wasActive = arm.IsActive();
		arm.Update(m_Presses[hand].Step(player.IsPress(ARM_ACTIONS[hand]), dt));
		if (!wasActive && arm.IsActive()) m_LastActiveHand = hand;
	}
}

void ArmManager::Draw(void) const
{
	if (!m_Links.Fighters->IsActive(m_Links.Slot)) return;
	for (int hand = 0; hand < 2; ++hand) m_Arms[hand][m_Types[hand]]->Draw();
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

ARM_TYPE ArmManager::GetType(int hand) const
{
	return hand >= 0 && hand < 2 ? m_Types[hand] : ARM_TYPE_COUNT;
}

int ArmManager::GetChangeDirection(int hand) const
{
	return hand == 0 || hand == 1 ? m_ChangeDirections[hand] : 1;
}

float ArmManager::GetMoveScale() const
{
	return fminf(m_Arms[0][m_Types[0]]->GetMoveScale(), m_Arms[1][m_Types[1]]->GetMoveScale());
}

GUARD ArmManager::GetGuard() const
{
	return GuardCalcStronger(m_Arms[0][m_Types[0]]->GetGuard(), m_Arms[1][m_Types[1]]->GetGuard());
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

int Arm::Explode(const VEC3& center, float radius, float damage, float power, float up) const
{
	int hits = 0;
	for (int i = 0; i < FIGHTER_COUNT; ++i) {
		const TENTACLE_TARGET target = GetFighters().GetTarget(i, GetSlot());
		if (!target.Available || !CollisionSphereCapsule({ center, radius }, target.Body).Hit) continue;
		GetFighters().Launch(i, GetSlot(), AreaCalcRadialDirection(center, target.Body, up, GetForward()), damage, power);
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
