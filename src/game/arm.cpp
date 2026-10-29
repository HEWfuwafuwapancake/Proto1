#include "game/arm.h"
#include "game/fighter.h"
#include "game/fightermanager.h"
#include "game/arm_grapple.h"
#include "game/arm_drill.h"
#include "game/arm_magnet.h"
#include "game/arm_shield.h"
#include "game/tentacle.h"
#include "input/action.h"
#include "physics/armcalc.h"
#include "physics/tentaclecalc.h"
#include "render/model.h"
#include <memory>

#ifdef _DEBUG
#include "core/debug_ostream.h"
#include <iostream>
#endif

// 調整用の値（今後追加する腕も、取付位置と待機方向はここを使う）
static constexpr float ARM_REST_PITCH = -XM_PI / 4.0f; // 待機の腕を正面から 45° 下げる
static constexpr float ARM_REST_SPREAD = XMConvertToRadians(25.0f); // 片側ずつ外へ開く角度
static constexpr float SHOULDER_OFFSET = 0.65f;
static constexpr float SHOULDER_FORWARD = -0.1f; // 肩を少し後ろへ引いて胸を張る
static constexpr float HAND_HEIGHT = 1.35f;
static constexpr XMFLOAT4 LEFT_COLOR{ 0.8f, 0.3f, 0.65f, 1.0f };
static constexpr XMFLOAT4 RIGHT_COLOR{ 0.35f, 0.8f, 0.6f, 1.0f };
static constexpr XMFLOAT4 HOLD_COLOR{ 1.0f, 0.7f, 0.2f, 1.0f };

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
};
static const char* const ARM_NAMES[ARM_TYPE_COUNT] = {
	"触腕", "グラップリング", "ドリル", "磁石", "デカ盾", "ハサミ"
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
	const ACTION_EQUIP_KEY key = ActionGetEquipKey(m_Links.Slot);
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
	return ArmCalcShoulder({ p.x, p.y, p.z }, GetPlayer().GetYaw(), hand, SHOULDER_OFFSET, HAND_HEIGHT, SHOULDER_FORWARD);
}

VEC3 Arm::GetRestDirection(float pitchOffset) const
{
	return ArmCalcRestDirection(GetPlayer().GetYaw(), GetHand(), ARM_REST_PITCH + pitchOffset, ARM_REST_SPREAD);
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
