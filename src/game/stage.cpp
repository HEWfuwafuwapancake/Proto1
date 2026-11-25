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
// 対戦マップの色（メニューと同じピンク・黄緑・紫を軸にした、ポップな色。2026-11-24 に管理者の依頼で設計書の色から変えた）
static constexpr XMFLOAT4 POST_FLOOR_COLOR = { 0.88f, 0.84f, 1.00f, 1.0f };		// 屋上の床（薄いラベンダー）
static constexpr XMFLOAT4 POST_ROOF_COLOR = { 1.00f, 0.36f, 0.70f, 1.0f };		// 集配棟（ピンク）
static constexpr XMFLOAT4 POST_RAMP_COLOR = { 0.62f, 0.40f, 1.00f, 1.0f };		// 保守用の坂（紫）
static constexpr XMFLOAT4 POST_CENTER_COLOR = { 0.78f, 0.96f, 0.20f, 1.0f };		// 仕分け台（黄緑）
static constexpr XMFLOAT4 POST_EDGE_COLOR = { 1.00f, 0.80f, 0.15f, 1.0f };		// 外周の帯（黄色）
static constexpr XMFLOAT4 BELT_COLOR = { 0.30f, 0.14f, 0.55f, 1.0f };				// ダッシュ板の帯（濃い紫）
static constexpr XMFLOAT4 BELT_ARROW_COLOR = { 0.35f, 1.00f, 1.00f, 1.0f };		// ダッシュ板の山形（水色）
static constexpr XMFLOAT4 MARKET_SHORE_COLOR = { 1.00f, 0.86f, 0.45f, 1.0f };	// 岸（ひまわり色）
static constexpr XMFLOAT4 MARKET_RAMP_COLOR = { 1.00f, 0.66f, 0.35f, 1.0f };		// 坂（オレンジ）
static constexpr XMFLOAT4 MARKET_LOW_COLOR = { 0.25f, 0.82f, 0.76f, 1.0f };		// 低い荷道と搬入橋（ミント）
static constexpr XMFLOAT4 MARKET_SHUTTER_COLOR = { 1.00f, 0.42f, 0.30f, 1.0f };	// 南の橋（朱色）
static constexpr XMFLOAT4 MARKET_TRAM_COLOR = { 0.30f, 0.62f, 1.00f, 1.0f };		// 北の橋（市電の屋根。空色）
static constexpr XMFLOAT4 MARKET_TRAM_WINDOW_COLOR = { 0.20f, 0.12f, 0.40f, 1.0f };	// 市電の窓枠（濃い紫）
static constexpr XMFLOAT4 MARKET_BASE_COLOR = { 1.00f, 0.45f, 0.62f, 1.0f };		// 建物の基礎（ピンク）
static constexpr XMFLOAT4 MARKET_LINE_COLOR = { 1.00f, 1.00f, 1.00f, 1.0f };		// 地割れの縁の白線
static constexpr float BRIDGE_DRAW_THICKNESS = 0.5f;	// 片面の橋の板の厚さ（見た目だけ）
static constexpr float RAMP_SLAB_THICKNESS = 0.5f;		// 坂の上面の板の厚さ（見た目だけ）
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
	case STAGE_SURFACE_RAMP: color = post ? POST_RAMP_COLOR : MARKET_RAMP_COLOR; break;
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
			// 先に調べる区画で、この上面より低い所がある区画（荷道・坂など）は、そこを除いて描く。
			// 除かないと、当たり判定のない見た目だけの床がその上にかぶさって隠してしまう。
			float holes[STAGE_REGION_MAX][4];
			int holeCount = 0;
			const int index = static_cast<int>(&region - m_Stage.Regions);
			for (int i = 0; i < index; ++i) {
				const STAGE_REGION& other = m_Stage.Regions[i];
				const float low = fminf(StageCalcRegionHeight(other, other.MinX, other.MinZ), StageCalcRegionHeight(other, other.MaxX, other.MaxZ));
				if (low >= top) continue;
				holes[holeCount][0] = other.MinX; holes[holeCount][1] = other.MaxX;
				holes[holeCount][2] = other.MinZ; holes[holeCount][3] = other.MaxZ;
				++holeCount;
			}
			float pieces[32][4];
			const int count = MapCalcSubtractRects(region.MinX, region.MaxX, region.MinZ, region.MaxZ, holes, holeCount, pieces, 32);
			for (int i = 0; i < count; ++i) {
				const float* r = pieces[i];
				ShapeDrawBox({ (r[0] + r[1]) * 0.5f, middle, (r[2] + r[3]) * 0.5f }, { r[1] - r[0], height, r[3] - r[2] }, color);
			}
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

	// 坂：上面に沿った板と、その下を埋める「くさび＋箱」（実体の坂だけ）。
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
	// くさびの斜めの面は板の真ん中（上面より板の厚さの半分だけ下）に置き、板の中に隠す。
	// その下は、低い側の端の高さから地盤の下面までを箱で埋める。
	const float rise = to.y - from.y;	// 範囲の大きい側の端が、小さい側の端よりどれだけ高いか
	const float base = fminf(from.y, to.y);
	const float length = highA - lowA;
	const float across = alongX ? depth : width;
	ShapeDrawWedge({ centerX, base + fabsf(rise) * 0.5f, centerZ }, length, fabsf(rise), across,
		alongX ? rise : 0.0f, alongX ? 0.0f : rise, color);
	const float bottom = m_Stage.GroundBottomY;
	if (base > bottom) {
		ShapeDrawBox({ centerX, (base + bottom) * 0.5f, centerZ }, { width, base - bottom, depth }, color);
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
