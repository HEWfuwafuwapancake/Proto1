#include "game/armcapsule.h"
#include "core/gametime.h"
#include "game/arm.h"
#include "game/fightermanager.h"
#include "game/stage.h"
#include "render/shape.h"

// 調整用の値（描画だけ）
static constexpr float CAPSULE_RADIUS = 0.38f;
static constexpr float CAPSULE_HEIGHT = 1.1f;
static constexpr float CAPSULE_FLOAT_HEIGHT = 0.15f;
static constexpr float CAPSULE_BOB_HEIGHT = 0.1f;
static constexpr float CAPSULE_BOB_SPEED = 3.0f;
// 触手・ハンマー・ナックル・C4・ガンランス・刀の順。
static constexpr XMFLOAT4 CAPSULE_COLORS[] = {
	{ 0.82f, 0.25f, 0.72f, 1.0f }, { 1.0f, 0.72f, 0.15f, 1.0f },
	{ 0.95f, 0.26f, 0.18f, 1.0f }, { 0.20f, 0.90f, 0.42f, 1.0f },
	{ 0.20f, 0.55f, 1.0f, 1.0f }, { 0.45f, 0.95f, 1.0f, 1.0f },
};
static_assert(ArmCapsuleCalc::TYPE_COUNT == ARM_TYPE_NONE, "袋には素手以外の腕を入れる");
static_assert(sizeof(CAPSULE_COLORS) / sizeof(CAPSULE_COLORS[0]) == ArmCapsuleCalc::TYPE_COUNT, "腕ごとに色を設定する");

void ArmCapsule::Initialize(const Stage& stage)
{
	m_Stage = &stage;
}

void ArmCapsule::Reset(uint32_t seed)
{
	m_Calc.Start(m_Stage->GetStage(), seed);
}

void ArmCapsule::Finalize()
{
	m_Calc = {};
	m_Stage = nullptr;
}

void ArmCapsule::Update(const FighterManager& fighters, ArmManager* arms)
{
	m_Calc.Step(m_Stage->GetStage(), GameTimeGetDeltaTime());
	for (int slot = 0; slot < FIGHTER_COUNT; ++slot) {
		const Fighter& fighter = fighters.GetFighter(slot);
		if (!fighter.IsHuman() || !fighter.CanUseArms()) continue;
		const int index = m_Calc.FindTouching(fighter.GetBody());
		if (index >= 0 && arms[slot].TryEquipPickup(static_cast<ARM_TYPE>(m_Calc.GetItems()[index].Type))) {
			m_Calc.Remove(index);
		}
	}
}

void ArmCapsule::Draw() const
{
	const float height = CAPSULE_FLOAT_HEIGHT + CAPSULE_BOB_HEIGHT * sinf(GameTimeGetElapsed() * CAPSULE_BOB_SPEED);
	for (const auto& item : m_Calc.GetItems()) {
		if (!item.Active) continue;
		const VEC3& p = item.Position;
		ShapeDrawCapsule({ p.X, p.Y + height, p.Z }, CAPSULE_RADIUS, CAPSULE_HEIGHT, CAPSULE_COLORS[item.Type]);
	}
}
