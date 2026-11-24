#include "game/stage.h"
#include "render/shape.h"
#include <math.h>


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
// 対戦マップの色（設計書 第12節の sRGB のベース色）
static constexpr XMFLOAT4 POST_FLOOR_COLOR = { 0.780f, 0.855f, 0.812f, 1.0f };		// #C7DACF 屋上の床
static constexpr XMFLOAT4 POST_ROOF_COLOR = { 0.467f, 0.651f, 0.612f, 1.0f };		// #77A69C 集配棟
static constexpr XMFLOAT4 POST_CENTER_COLOR = { 0.910f, 0.925f, 0.753f, 1.0f };		// #E8ECC0 仕分け台
static constexpr XMFLOAT4 POST_EDGE_COLOR = { 1.0f, 0.729f, 0.592f, 1.0f };			// #FFBA97 外周の帯
static constexpr XMFLOAT4 BELT_COLOR = { 0.36f, 0.40f, 0.44f, 1.0f };				// ベルトの帯
static constexpr XMFLOAT4 BELT_ARROW_COLOR = { 0.45f, 0.95f, 1.0f, 1.0f };			// ベルトの走る光の山形
static constexpr XMFLOAT4 MARKET_SHORE_COLOR = { 0.796f, 0.718f, 0.600f, 1.0f };	// #CBB799 岸
static constexpr XMFLOAT4 MARKET_LOW_COLOR = { 0.612f, 0.671f, 0.612f, 1.0f };		// #9CAB9C 低い荷道
static constexpr XMFLOAT4 MARKET_SHUTTER_COLOR = { 0.702f, 0.490f, 0.353f, 1.0f };	// #B37D5A 南の橋
static constexpr XMFLOAT4 MARKET_TRAM_COLOR = { 0.42f, 0.56f, 0.66f, 1.0f };		// 北の橋（市電の屋根）
static constexpr XMFLOAT4 MARKET_TRAM_WINDOW_COLOR = { 0.18f, 0.22f, 0.28f, 1.0f };	// 市電の窓枠
static constexpr XMFLOAT4 MARKET_BASE_COLOR = { 0.66f, 0.58f, 0.48f, 1.0f };		// 建物の基礎
static constexpr XMFLOAT4 MARKET_LINE_COLOR = { 0.97f, 0.97f, 0.95f, 1.0f };		// 地割れの縁の白線
static constexpr float BRIDGE_DRAW_THICKNESS = 0.5f;	// 片面の橋の板の厚さ（見た目だけ）
static constexpr float RAMP_SLAB_THICKNESS = 0.5f;		// 坂の上面の板の厚さ（見た目だけ）
static constexpr int RAMP_COLUMNS = 8;					// 坂の下を埋める段の数（見た目だけ）
static constexpr float MARK_LIFT = 0.02f;				// 床に描く印を浮かせる高さ（床と重ならないように）
static constexpr float POST_EDGE_WIDTH = 0.20f;			// 郵便局の外周の帯の幅
static constexpr float MARKET_LINE_WIDTH = 0.15f;		// 市場の縁の白線の幅
static constexpr float MARKET_LINE_DASH = 1.2f;			// 欠けた白線の1本の長さ（同じだけ空ける）
static constexpr float SQRT2 = 1.41421356f;


void Stage::Reset(void)
{
	m_Stage = STAGE{};
	m_Map = STAGE_MAP_ISLAND;
	m_Scale = 1.0f;
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

void Stage::SetMap(STAGE_MAP map, float scale)
{
	if (!MapCalcBuild(map, scale, m_Stage)) {
		SetSizeScale(scale);
		return;
	}
	m_Map = map;
	m_Scale = scale;
}

void Stage::SetSizeScale(float scale)
{
	Reset();
	m_Scale = scale;
	m_Stage.HalfWidth *= scale;
	m_Stage.HalfDepth *= scale;
	for (int i = 0; i < m_Stage.PlatformCount; ++i) {
		m_Stage.Platforms[i].CenterX *= scale;
		m_Stage.Platforms[i].CenterZ *= scale;
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
	if (m_Stage.HasTerrain) {
		DrawTerrain();
		return;
	}
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

void Stage::DrawTerrain(void) const
{
	for (int i = 0; i < m_Stage.RegionCount; ++i) DrawRegion(m_Stage.Regions[i]);
	DrawMarks();
}

void Stage::DrawRegion(const STAGE_REGION& region) const
{
	const bool post = m_Map == STAGE_MAP_POST_OFFICE;
	XMFLOAT4 color = post ? POST_FLOOR_COLOR : MARKET_SHORE_COLOR;
	switch (region.Surface) {
	case STAGE_SURFACE_CENTER: color = POST_CENTER_COLOR; break;
	case STAGE_SURFACE_ROOF: color = POST_ROOF_COLOR; break;
	case STAGE_SURFACE_RAMP: color = post ? POST_ROOF_COLOR : MARKET_SHORE_COLOR; break;
	case STAGE_SURFACE_LOW_ROAD: color = MARKET_LOW_COLOR; break;
	case STAGE_SURFACE_TRAM: color = MARKET_TRAM_COLOR; break;
	case STAGE_SURFACE_SHUTTER: color = MARKET_SHUTTER_COLOR; break;
	case STAGE_SURFACE_BASE: color = MARKET_BASE_COLOR; break;
	default: break;
	}
	const float centerX = (region.MinX + region.MaxX) * 0.5f;
	const float centerZ = (region.MinZ + region.MaxZ) * 0.5f;
	const float width = region.MaxX - region.MinX;
	const float depth = region.MaxZ - region.MinZ;

	if (region.SlopeX == 0.0f && region.SlopeZ == 0.0f) {
		// 平らな区画：実体なら地盤の下面から上面まで、片面の橋なら薄い板。
		const float top = region.BaseY;
		const float bottom = region.Solid ? m_Stage.GroundBottomY : top - BRIDGE_DRAW_THICKNESS;
		const float height = top - bottom;
		const float middle = (top + bottom) * 0.5f;
		if (region.DiagonalLimit <= 0.0f) {
			ShapeDrawBox({ centerX, middle, centerZ }, { width, height, depth }, color);
			return;
		}
		// 四隅を切り落とした八角形：十字の2つの箱と、切り口に沿った4本の箱で描く（原点が中心の区画だけ）。
		const float hx = region.MaxX, hz = region.MaxZ;
		const float cut = hx + hz - region.DiagonalLimit;	// 各辺から切り落とす長さ
		ShapeDrawBox({ 0.0f, middle, 0.0f }, { hx * 2.0f, height, (hz - cut) * 2.0f }, color);
		ShapeDrawBox({ 0.0f, middle, 0.0f }, { (hx - cut) * 2.0f, height, hz * 2.0f }, color);
		for (int corner = 0; corner < 4; ++corner) {
			const float sx = (corner & 1) ? 1.0f : -1.0f;
			const float sz = (corner & 2) ? 1.0f : -1.0f;
			const float shift = cut * 0.25f; // 切り口から内側へ、帯の幅の半分だけずらす
			ShapeDrawBoxAlong({ sx * ((hx - cut) - shift), middle, sz * (hz - shift) },
				{ sx * (hx - shift), middle, sz * ((hz - cut) - shift) }, cut / SQRT2, height, color);
		}
		return;
	}

	// 坂：上面に沿った板と、その下を埋める段（実体の坂だけ）。
	const bool alongX = region.SlopeX != 0.0f;
	const float lowA = alongX ? region.MinX : region.MinZ;
	const float highA = alongX ? region.MaxX : region.MaxZ;
	const auto point = [&](float a) {
		const float x = alongX ? a : centerX;
		const float z = alongX ? centerZ : a;
		return XMFLOAT3{ x, StageCalcRegionHeight(region, x, z), z };
	};
	XMFLOAT3 from = point(lowA), to = point(highA);
	from.y -= RAMP_SLAB_THICKNESS * 0.5f;
	to.y -= RAMP_SLAB_THICKNESS * 0.5f;
	ShapeDrawBoxAlong(from, to, alongX ? depth : width, RAMP_SLAB_THICKNESS, color);
	if (!region.Solid) return;
	for (int i = 0; i < RAMP_COLUMNS; ++i) {
		const float a0 = lowA + (highA - lowA) * i / RAMP_COLUMNS;
		const float a1 = lowA + (highA - lowA) * (i + 1) / RAMP_COLUMNS;
		const float top = fminf(point(a0).y, point(a1).y) - RAMP_SLAB_THICKNESS * 0.5f;
		const float bottom = m_Stage.GroundBottomY;
		if (top <= bottom) continue;
		const float mid = (a0 + a1) * 0.5f;
		const XMFLOAT3 center{ alongX ? mid : centerX, (top + bottom) * 0.5f, alongX ? centerZ : mid };
		const XMFLOAT3 size{ alongX ? a1 - a0 : width, top - bottom, alongX ? depth : a1 - a0 };
		ShapeDrawBox(center, size, color);
	}
}

void Stage::DrawMarks(void) const
{
	const float s = m_Scale;
	const auto line = [](float x0, float z0, float x1, float z1, float y, float lineWidth, const XMFLOAT4& color) {
		ShapeDrawBoxAlong({ x0, y, z0 }, { x1, y, z1 }, lineWidth, MARK_LIFT, color);
	};
	if (m_Map == STAGE_MAP_POST_OFFICE) {
		// 外周の橙の帯（屋上の八角形のふちの内側に沿わせる）。
		const STAGE_REGION& roof = m_Stage.Regions[m_Stage.RegionCount - 1];
		const float h = roof.MaxX - POST_EDGE_WIDTH * 0.5f;
		const float c = roof.DiagonalLimit - POST_EDGE_WIDTH * 0.5f * SQRT2;
		const float corners[8][2] = { { -(c - h), h }, { -h, c - h }, { -h, -(c - h) }, { -(c - h), -h },
			{ c - h, -h }, { h, -(c - h) }, { h, c - h }, { c - h, h } };
		for (int i = 0; i < 8; ++i) {
			const float* a = corners[i];
			const float* b = corners[(i + 1) % 8];
			line(a[0], a[1], b[0], b[1], roof.BaseY + MARK_LIFT, POST_EDGE_WIDTH, POST_EDGE_COLOR);
		}
		// 荷送りベルト：中央を向く山形を3本ずつ。
		for (int i = 0; i < m_Stage.BeltCount; ++i) {
			const STAGE_BELT& belt = m_Stage.Belts[i];
			const float beltX = (belt.MinX + belt.MaxX) * 0.5f;
			const float beltZ = (belt.MinZ + belt.MaxZ) * 0.5f;
			ShapeDrawBox({ beltX, MARK_LIFT * 0.5f, beltZ }, { belt.MaxX - belt.MinX, MARK_LIFT, belt.MaxZ - belt.MinZ }, BELT_COLOR);
			const float toward = beltZ > 0.0f ? -1.0f : 1.0f; // 運ぶ向き
			const float halfX = (belt.MaxX - belt.MinX) * 0.35f;
			for (int k = 0; k < 3; ++k) {
				const float z = belt.MinZ + (belt.MaxZ - belt.MinZ) * (k + 0.5f) / 3.0f;
				const float tipZ = z + toward * halfX * 0.8f;
				line(beltX - halfX, z, beltX, tipZ, MARK_LIFT * 2.0f, 0.3f * s, BELT_ARROW_COLOR);
				line(beltX + halfX, z, beltX, tipZ, MARK_LIFT * 2.0f, 0.3f * s, BELT_ARROW_COLOR);
			}
		}
		return;
	}
	if (m_Map != STAGE_MAP_MARKET) return;
	// 地割れに面した岸の縁と、橋の落ちる側に、欠けた白線を引く。
	const auto dashed = [&](float x0, float z0, float x1, float z1, float y) {
		const float dx = x1 - x0, dz = z1 - z0;
		const float length = sqrtf(dx * dx + dz * dz);
		const int count = (int)(length / (MARKET_LINE_DASH * 2.0f * s));
		for (int i = 0; i < count; ++i) {
			const float t0 = (i * 2.0f * MARKET_LINE_DASH * s) / length;
			const float t1 = ((i * 2.0f + 1.0f) * MARKET_LINE_DASH * s) / length;
			line(x0 + dx * t0, z0 + dz * t0, x0 + dx * t1, z0 + dz * t1, y + MARK_LIFT, MARKET_LINE_WIDTH, MARKET_LINE_COLOR);
		}
	};
	const float inner = 4.0f * s + MARKET_LINE_WIDTH * 0.5f;
	const float sides[2] = { -1.0f, 1.0f };
	for (float side : sides) {
		const float x = side * inner;
		dashed(x, 4.0f * s, x, 10.0f * s, 0.0f);
		dashed(x, 14.0f * s, x, 24.0f * s, 0.0f);
		dashed(x, -10.0f * s, x, -4.0f * s, 0.0f);
		dashed(x, -24.0f * s, x, -14.0f * s, 0.0f);
		// 橋の長い辺（地割れの上の部分）。
		const float half = MARKET_LINE_WIDTH * 0.5f;
		dashed(-4.0f * s, side * (4.0f * s - half), 4.0f * s, side * (4.0f * s - half), -3.0f);
		dashed(-4.0f * s, side * (10.0f * s + half), 4.0f * s, side * (10.0f * s + half), 3.0f);
		dashed(-4.0f * s, side * (14.0f * s - half), 4.0f * s, side * (14.0f * s - half), 3.0f);
	}
	// 北の橋は市電の屋根：屋根の下に窓枠の帯を見せる（当たり判定はない）。
	ShapeDrawBox({ 0.0f, 3.0f - BRIDGE_DRAW_THICKNESS - 0.45f, 12.0f * s }, { 8.0f * s - 0.2f, 0.9f, 4.0f * s - 0.4f }, MARKET_TRAM_WINDOW_COLOR);
	// 南の橋は波板を継いだ板：上面に低い筋を入れる（当たり判定はない）。
	for (int k = 0; k < 6; ++k) {
		const float x = -4.0f * s + 8.0f * s * (k + 0.5f) / 6.0f;
		ShapeDrawBox({ x, 3.0f + MARK_LIFT, -12.0f * s }, { 0.12f, MARK_LIFT * 2.0f, 4.0f * s - 0.2f }, MARKET_BASE_COLOR);
	}
}
