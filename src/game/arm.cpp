#include "game/arm.h"
#include "game/player.h"
#include "game/arm_grapple.h"
#include "game/arm_drill.h"
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
static std::unique_ptr<Arm> CreateArm(int hand)
{
	return std::make_unique<T>(hand);
}

// 新しい腕は Arm を派生し、この1行を CreateArm<腕のクラス> にする。
// nullptr の種類は読み込まず、数字キー・巡回のどちらでも選べない。
using ArmFactory = std::unique_ptr<Arm> (*)(int hand);
static const ArmFactory ARM_FACTORIES[ARM_TYPE_COUNT] = {
	CreateArm<TentacleArm>, // 触腕
	CreateArm<GrappleArm>, // グラップリング（No.36）
	CreateArm<DrillArm>, // ドリル（No.37）
	nullptr, // 磁石（No.38）
	nullptr, // デカ盾（No.39）
	nullptr, // ハサミ（No.40）
};
static const char* const ARM_NAMES[ARM_TYPE_COUNT] = {
	"触腕", "グラップリング", "ドリル", "磁石", "デカ盾", "ハサミ"
};
static ARM_TYPE g_Types[2] = { ARM_TYPE_TENTACLE, ARM_TYPE_TENTACLE };
// 左右それぞれ全種類を起動時に作る。装備切り替えでは作り直さない。
static std::unique_ptr<Arm> g_Arms[2][ARM_TYPE_COUNT];

static void ChangeType(int hand, int next)
{
	if (next < 0 || next >= ARM_TYPE_COUNT || ARM_FACTORIES[next] == nullptr || next == g_Types[hand]) return;
	g_Arms[hand][g_Types[hand]]->Reset();
	g_Types[hand] = (ARM_TYPE)next;
#ifdef _DEBUG
	hal::dout << "腕：" << (hand == 0 ? "左" : "右") << " → " << ArmGetName(g_Types[hand]) << std::endl;
#endif
}

void ArmInitialize(void)
{
	g_Types[0] = g_Types[1] = ARM_TYPE_TENTACLE;
	for (int type = 0; type < ARM_TYPE_COUNT; ++type) {
		if (ARM_FACTORIES[type] == nullptr) continue;
		for (int hand = 0; hand < 2; ++hand) g_Arms[hand][type] = ARM_FACTORIES[type](hand);
	}
	// 引き寄せは両手で同時に行わず、後から刺さった手へ所有権を渡す。
	GrappleArm& left = static_cast<GrappleArm&>(*g_Arms[0][ARM_TYPE_GRAPPLE]);
	GrappleArm& right = static_cast<GrappleArm&>(*g_Arms[1][ARM_TYPE_GRAPPLE]);
	left.ConnectOtherArm(right);
}

void ArmFinalize(void)
{
	for (int type = 0; type < ARM_TYPE_COUNT; ++type) {
		for (int hand = 0; hand < 2; ++hand) g_Arms[hand][type].reset();
	}
}

void ArmReset(void)
{
	for (int hand = 0; hand < 2; ++hand) g_Arms[hand][g_Types[hand]]->Reset();
}

void ArmUpdate(bool playing)
{
	bool available[ARM_TYPE_COUNT];
	for (int type = 0; type < ARM_TYPE_COUNT; ++type) available[type] = ARM_FACTORIES[type] != nullptr;
	const ACTION_EQUIP_KEY key = ActionGetEquipKey();
	for (int hand = 0; hand < 2; ++hand) {
		const ACTION prev = hand == 0 ? ACTION_EQUIP_LEFT_PREV : ACTION_EQUIP_RIGHT_PREV;
		const ACTION next = hand == 0 ? ACTION_EQUIP_LEFT_NEXT : ACTION_EQUIP_RIGHT_NEXT;
		// 前後を同時に押したときは巡回しない。数字キーがあれば直接選択を優先する。
		const int direction = (ActionIsTrigger(next) ? 1 : 0) - (ActionIsTrigger(prev) ? 1 : 0);
		if (key.Hand == hand) ChangeType(hand, ArmCalcSelect(key.Number, available, ARM_TYPE_COUNT));
		else if (direction != 0) ChangeType(hand, ArmCalcCycle(g_Types[hand], direction, available, ARM_TYPE_COUNT));
	}
	if (!playing) return;
	for (int hand = 0; hand < 2; ++hand) {
		g_Arms[hand][g_Types[hand]]->Update(ActionIsTrigger(hand == 0 ? ACTION_ARM_LEFT : ACTION_ARM_RIGHT));
	}
}

void ArmDraw(void)
{
	for (int hand = 0; hand < 2; ++hand) g_Arms[hand][g_Types[hand]]->Draw();
}

bool ArmIsActive(void)
{
	for (int hand = 0; hand < 2; ++hand) {
		if (g_Arms[hand][g_Types[hand]]->IsActive()) return true;
	}
	return false;
}

bool ArmIsHolding(void)
{
	for (int hand = 0; hand < 2; ++hand) {
		if (g_Arms[hand][g_Types[hand]]->IsHolding()) return true;
	}
	return false;
}

ARM_TYPE ArmGetType(int hand)
{
	return hand >= 0 && hand < 2 ? g_Types[hand] : ARM_TYPE_COUNT;
}

const char* ArmGetName(ARM_TYPE type)
{
	return type >= 0 && type < ARM_TYPE_COUNT ? ARM_NAMES[type] : "不明";
}

VEC3 ArmGetShoulder(int hand)
{
	const XMFLOAT3 p = PlayerGetPosition();
	return ArmCalcShoulder({ p.x, p.y, p.z }, PlayerGetYaw(), hand, SHOULDER_OFFSET, HAND_HEIGHT, SHOULDER_FORWARD);
}

VEC3 ArmGetRestDirection(int hand, float pitchOffset)
{
	return ArmCalcRestDirection(PlayerGetYaw(), hand, ARM_REST_PITCH + pitchOffset, ARM_REST_SPREAD);
}

XMFLOAT4 ArmGetColor(int hand, bool active)
{
	return active ? HOLD_COLOR : (hand == 0 ? LEFT_COLOR : RIGHT_COLOR);
}

void ArmDrawPiece(int model, VEC3 start, VEC3 end, float sourceLength, float thickness, const XMFLOAT4& color, float roll)
{
	const TENTACLE_PIECE_AXES axes = ArmCalcRollAxes(TentacleCalcPieceAxes(start, end, { 0, -1, 0 }), roll);
	const VEC3 x = Vec3Scale(axes.X, thickness);
	const VEC3 y = Vec3Scale(axes.Y, Vec3Length(Vec3Sub(end, start)) / sourceLength);
	const VEC3 z = Vec3Scale(axes.Z, thickness);
	ModelDraw(model, { start.X, start.Y, start.Z }, { x.X, x.Y, x.Z }, { y.X, y.Y, y.Z }, { z.X, z.Y, z.Z }, color);
}

int ArmLoadModel(const char* path, int hand)
{
	return ModelLoad(path, hand == 1);
}
