#include "game/arm.h"
#include "game/fighter.h"
#include "game/fightermanager.h"
#include "game/arm_grapple.h"
#include "game/arm_drill.h"
#include "game/arm_magnet.h"
#include "game/arm_shield.h"
#include "game/arm_knuckle.h"
#include "game/arm_blade.h"
#include "game/tentacle.h"
#include "input/pad.h"
#include "game/camera.h"
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
// 肩の高さ・前後はキャラのリグの armMount（X ±14.6・高さ 110.5・Z 0 cm × 1.8 / 157.9）に合わせる。
// 左右は腕の付け根の箱が胸にめり込まないよう、取付口（0.165）より外へ出す。
static constexpr float SHOULDER_OFFSET = 0.24f;
static constexpr float SHOULDER_FORWARD = 0.0f;
static constexpr float HAND_HEIGHT = 1.26f;
static constexpr XMFLOAT4 LEFT_COLOR{ 0.8f, 0.3f, 0.65f, 1.0f };
static constexpr XMFLOAT4 RIGHT_COLOR{ 0.35f, 0.8f, 0.6f, 1.0f };
static constexpr XMFLOAT4 HOLD_COLOR{ 1.0f, 0.7f, 0.2f, 1.0f };
static constexpr float STUB_RADIUS = 0.06f; // モデルのない腕の円柱の半径。

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
	CreateArm<TentacleArm>, // 触腕
	CreateArm<GrappleArm>, // グラップリング（No.36）
	CreateArm<DrillArm>, // ドリル（No.37）
	CreateArm<MagnetArm>, // 磁石（No.38）
	CreateArm<ShieldArm>, // デカ盾（No.39）
	nullptr, // ハサミ（No.40）
	CreateArm<KnuckleArm>, // ナックル（No.60）
	CreateArm<BladeArm>, // ブレード（No.61）
	nullptr, // スピア（No.62）
	nullptr, // ハンマー（No.63）
	nullptr, // ウィップ（No.64）
	nullptr, // レーザー（No.65）
	nullptr, // ショットガン（No.66）
	nullptr, // スナイパー（No.67）
	nullptr, // ワープ（No.68）
	nullptr, // リペア（No.69）
	nullptr, // ロケットパンチ（No.71）
	nullptr, // キャノン（No.72）
	nullptr, // ガトリング（No.73）
	nullptr, // ミサイル（No.74）
	nullptr, // 火炎放射（No.76）
	nullptr, // フリーズ（No.77）
	nullptr, // サンダー（No.78）
	nullptr, // スモーク（No.79）
	nullptr, // バリア（No.81）
	nullptr, // アックス（No.82）
};
static const char* const ARM_NAMES[ARM_TYPE_COUNT] = {
	"触腕", "グラップリング", "ドリル", "磁石", "デカ盾", "ハサミ",
	"ナックル", "ブレード", "スピア", "ハンマー", "ウィップ", "レーザー", "ショットガン", "スナイパー",
	"ワープ", "リペア", "ロケットパンチ", "キャノン", "ガトリング", "ミサイル",
	"火炎放射", "フリーズ", "サンダー", "スモーク", "バリア", "アックス"
};

void ArmManager::ChangeType(int hand, int next)
{
	if (next < 0 || next >= ARM_TYPE_COUNT || ARM_FACTORIES[next] == nullptr || next == m_Types[hand]) return;
	m_Arms[hand][m_Types[hand]]->Reset();
	m_Types[hand] = (ARM_TYPE)next;
#ifdef _DEBUG
	hal::dout << "腕：" << (hand == 0 ? "左" : "右") << " → " << ArmGetName(m_Types[hand]) << std::endl;
#endif
}

void ArmManager::Initialize(const Stage& stage, FighterManager& fighters, int slot, const Camera& camera)
{
	m_Links = { &fighters, slot, &camera, &stage };
	m_Types[0] = m_Types[1] = ARM_TYPE_TENTACLE;
	for (int type = 0; type < ARM_TYPE_COUNT; ++type) {
		if (ARM_FACTORIES[type] == nullptr) continue;
		for (int hand = 0; hand < 2; ++hand) m_Arms[hand][type] = ARM_FACTORIES[type](hand, m_Links);
	}
	// 引き寄せは両手で同時に行わず、後から刺さった手へ所有権を渡す。
	GrappleArm& left = static_cast<GrappleArm&>(*m_Arms[0][ARM_TYPE_GRAPPLE]);
	GrappleArm& right = static_cast<GrappleArm&>(*m_Arms[1][ARM_TYPE_GRAPPLE]);
	left.ConnectOtherArm(right);
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
	for (int hand = 0; hand < 2; ++hand) m_Arms[hand][m_Types[hand]]->Reset();
}

void ArmManager::Update(bool playing)
{
	bool available[ARM_TYPE_COUNT];
	for (int type = 0; type < ARM_TYPE_COUNT; ++type) available[type] = ARM_FACTORIES[type] != nullptr;
	const ACTION_EQUIP_KEY key = m_Links.Fighters->GetFighter(m_Links.Slot).GetEquipInput();
	for (int hand = 0; hand < 2; ++hand) {
		if (key.Hand == hand) ChangeType(hand, ArmCalcSelect(key.Number, available, ARM_TYPE_COUNT));
	}
	if (!playing) return;
	const Fighter& player = m_Links.Fighters->GetFighter(m_Links.Slot);
	if (!player.CanUseArms() || player.DidRespawn()) {
		Reset();
		return;
	}
	for (int hand = 0; hand < 2; ++hand) {
		m_Arms[hand][m_Types[hand]]->Update(player.IsTrigger(hand == 0 ? ACTION_ARM_LEFT : ACTION_ARM_RIGHT));
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

ARM_TYPE ArmManager::GetType(int hand) const
{
	return hand >= 0 && hand < 2 ? m_Types[hand] : ARM_TYPE_COUNT;
}

float ArmManager::GetMoveScale() const
{
	return fminf(m_Arms[0][m_Types[0]]->GetMoveScale(), m_Arms[1][m_Types[1]]->GetMoveScale());
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
	const int hand = GetHand();
	const XMFLOAT3 p = GetPlayer().GetPosition();
	const VEC3 mount = GetMountOffset();
	return ArmCalcShoulder({ p.x, p.y, p.z }, GetPlayer().GetYaw(), hand,
		SHOULDER_OFFSET + mount.X, HAND_HEIGHT + mount.Y, SHOULDER_FORWARD + mount.Z);
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
	const VEC3 shoulder = GetShoulder();
	const VEC3 hand = Vec3Add(shoulder, Vec3Scale(direction, length));
	ShapeDrawCylinder({ shoulder.X, shoulder.Y, shoulder.Z }, { hand.X, hand.Y, hand.Z }, STUB_RADIUS, color);
	return hand;
}

XMFLOAT4 Arm::GetColor(bool active) const
{
	return active ? HOLD_COLOR : (GetHand() == 0 ? LEFT_COLOR : RIGHT_COLOR);
}

void Arm::DrawPiece(int model, VEC3 start, VEC3 end, float sourceLength, float thickness, const XMFLOAT4& color, float roll) const
{
	const TENTACLE_PIECE_AXES axes = ArmCalcRollAxes(TentacleCalcPieceAxes(start, end, { 0, -1, 0 }), roll);
	const VEC3 x = Vec3Scale(axes.X, thickness);
	const VEC3 y = Vec3Scale(axes.Y, Vec3Length(Vec3Sub(end, start)) / sourceLength);
	const VEC3 z = Vec3Scale(axes.Z, thickness);
	ModelDraw(model, { start.X, start.Y, start.Z }, { x.X, x.Y, x.Z }, { y.X, y.Y, y.Z }, { z.X, z.Y, z.Z }, color);
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
