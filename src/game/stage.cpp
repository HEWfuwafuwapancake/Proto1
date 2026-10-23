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


static STAGE g_Stage = {};		// 浮島と場外の線


void StageInitialize(void)
{
	g_Stage.CenterX = 0.0f;
	g_Stage.CenterZ = 0.0f;
	g_Stage.HalfWidth = STAGE_WIDTH * 0.5f;
	g_Stage.HalfDepth = STAGE_DEPTH * 0.5f;
	g_Stage.TopY = STAGE_TOP_Y;
	g_Stage.Thickness = STAGE_THICKNESS;
	g_Stage.OutY = OUT_Y;
}

void StageFinalize(void)
{

}

void StageUpdate(void)
{

}

void StageDraw(void)
{
	// 上の面が TopY になるように、箱の中心を厚さの半分だけ下げる
	const XMFLOAT3 center = { g_Stage.CenterX, g_Stage.TopY - g_Stage.Thickness * 0.5f, g_Stage.CenterZ };
	const XMFLOAT3 size = { g_Stage.HalfWidth * 2.0f, g_Stage.Thickness, g_Stage.HalfDepth * 2.0f };
	ShapeDrawBox(center, size, STAGE_COLOR);
}

const STAGE& StageGet(void)
{
	return g_Stage;
}
