#include "game/stage.h"
#include "render/shape.h"


//------------------------------------------------------------------------------
//	調整用の値
//------------------------------------------------------------------------------
static constexpr float STAGE_WIDTH = 30.0f;		// 浮島の横幅（X）
static constexpr float STAGE_DEPTH = 30.0f;		// 浮島の奥行き（Z）
static constexpr float STAGE_THICKNESS = 2.0f;	// 浮島の厚さ（見た目だけ）
static constexpr float STAGE_TOP_Y = 0.0f;		// 浮島の上の面の高さ
static constexpr float OUT_Y = -15.0f;			// 場外の下の線（これより下に落ちたら場外）
static constexpr XMFLOAT4 STAGE_COLOR = { 0.45f, 0.65f, 0.4f, 1.0f };	// 浮島の色（仮。くすんだ緑）
static constexpr STAGE_PLATFORM STAGE_PLATFORMS[] = {
	{ -8.0f,  8.0f, 3.0f, 3.0f, 4.0f, 0.6f },
	{  8.0f, -8.0f, 3.0f, 3.0f, 4.0f, 0.6f },
	{  8.0f,  8.0f, 3.0f, 3.0f, 6.5f, 0.6f },
	{ -8.0f, -8.0f, 3.0f, 3.0f, 6.5f, 0.6f },
};
static constexpr int STAGE_PLATFORM_COUNT = sizeof(STAGE_PLATFORMS) / sizeof(STAGE_PLATFORMS[0]);
static_assert(STAGE_PLATFORM_COUNT <= STAGE_PLATFORM_MAX, "Too many stage platforms");


void Stage::Reset(void)
{
	m_Stage.CenterX = 0.0f;
	m_Stage.CenterZ = 0.0f;
	m_Stage.HalfWidth = STAGE_WIDTH * 0.5f;
	m_Stage.HalfDepth = STAGE_DEPTH * 0.5f;
	m_Stage.TopY = STAGE_TOP_Y;
	m_Stage.Thickness = STAGE_THICKNESS;
	m_Stage.OutY = OUT_Y;
	m_Stage.PlatformCount = STAGE_PLATFORM_COUNT;
	for (int i = 0; i < m_Stage.PlatformCount; ++i) {
		m_Stage.Platforms[i] = STAGE_PLATFORMS[i];
	}
}

void Stage::Finalize(void)
{

}

void Stage::Update(void)
{

}

void Stage::Draw(void) const
{
	const auto drawPlatform = [](const STAGE_PLATFORM& platform) {
		// 上の面が TopY になるように、箱の中心を厚さの半分だけ下げる
		const XMFLOAT3 center = { platform.CenterX, platform.TopY - platform.Thickness * 0.5f, platform.CenterZ };
		const XMFLOAT3 size = { platform.HalfWidth * 2.0f, platform.Thickness, platform.HalfDepth * 2.0f };
		ShapeDrawBox(center, size, STAGE_COLOR);
	};
	drawPlatform({ m_Stage.CenterX, m_Stage.CenterZ, m_Stage.HalfWidth, m_Stage.HalfDepth,
		m_Stage.TopY, m_Stage.Thickness });
	for (int i = 0; i < m_Stage.PlatformCount; ++i) {
		drawPlatform(m_Stage.Platforms[i]);
	}
}

const STAGE& Stage::GetStage(void) const
{
	return m_Stage;
}

void Stage::Initialize(void)
{
	Reset();
}
