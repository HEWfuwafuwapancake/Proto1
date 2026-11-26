#include "game/stage.h"
#include "core/dx11.h"
#include "core/gametime.h"
#include "core/shader.h"
#include "render/model.h"
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
static constexpr float DRAW_HEIGHT_EPSILON = 0.01f;	// 地形を描くときに高さをくらべる誤差（台帳の式の小数の誤差を吸収する。見た目だけ）
static constexpr float HOLE_FILL_THICKNESS = 0.5f;	// 床の穴に入る区画（荷道など）を、穴のいちばん低い所からさらに下へ描く厚さ（見た目だけ）
static constexpr float MARK_LIFT = 0.02f;				// 床に描く印を浮かせる高さ（床と重ならないように）
static constexpr float POST_EDGE_WIDTH = 0.20f;			// 郵便局の外周の帯の幅
static constexpr float MARKET_LINE_WIDTH = 0.15f;		// 市場の縁の白線の幅
static constexpr float MARKET_LINE_DASH = 1.2f;			// 欠けた白線の1本の長さ（同じだけ空ける）
static constexpr float SQRT2 = 1.41421356f;
// 空の色と、モデルで描くマップの景観（並びは STAGE_MAP と同じ。モデルは頂点の色にライトを焼き込んだ GLB）
static constexpr XMFLOAT4 DEFAULT_SKY_COLOR = { 0.4f, 0.6f, 0.9f, 1.0f };	// 郵便局・市場・浮島の空（今までの青）
/// @brief マップごとの見た目（空の色と景観のモデル）
struct MAP_LOOK
{
	XMFLOAT4 Sky;		// 空の色
	const char* Model;	// 景観のモデル（nullptr なら区画の箱で描く）
};
static constexpr MAP_LOOK MAP_LOOKS[STAGE_MAP_COUNT] = {
	{ DEFAULT_SKY_COLOR, nullptr },										// 郵便局
	{ DEFAULT_SKY_COLOR, nullptr },										// 市場
	{ { 0.30f, 0.13f, 0.43f, 1.0f }, "assets/models/maps/neon.glb" },		// ネオンビル群（紫の夕空）
	{ { 0.63f, 0.43f, 0.31f, 1.0f }, "assets/models/maps/coliseum.glb" },	// コロシアム（砂岩の色）
	{ { 0.57f, 0.83f, 0.90f, 1.0f }, "assets/models/maps/station.glb" },	// 廃駅（晴天の水色）
	{ DEFAULT_SKY_COLOR, nullptr },										// 浮島
};
static constexpr const char* COLUMN_MODEL = "assets/models/maps/coliseum_column.glb";	// 倒れる柱（根元が原点、+Y が柱の先）
static constexpr float COLUMN_DAMAGE_SHADE = 0.35f;	// 柱の耐久が尽きたときに暗くする割合（傷みを色で見せる）


STAGE Stage::MakeIsland(void)
{
	STAGE stage{};
	stage.CenterX = 0.0f;
	stage.CenterZ = 0.0f;
	stage.HalfWidth = STAGE_WIDTH * 0.5f;
	stage.HalfDepth = STAGE_DEPTH * 0.5f;
	stage.TopY = STAGE_TOP_Y;
	stage.Thickness = STAGE_THICKNESS;
	stage.OutY = OUT_Y;
	stage.PlatformCount = STAGE_PLATFORM_COUNT;
	for (int i = 0; i < stage.PlatformCount; ++i) {
		stage.Platforms[i] = STAGE_PLATFORMS[i];
	}
	return stage;
}

void Stage::Reset(void)
{
	m_Stage = MakeIsland();
	m_Map = STAGE_MAP_ISLAND;
	m_Scale = 1.0f;
	m_Pillars.Setup(m_Stage);
	SetClearColor(MAP_LOOKS[m_Map].Sky);
}

void Stage::SetMap(STAGE_MAP map, float scale)
{
	if (!MapCalcBuild(map, scale, m_Stage)) {
		SetSizeScale(scale);
		return;
	}
	m_Map = map;
	m_Scale = scale;
	// 作り直すたびに柱を全部立て直す（試合の始め・もう一度でも元に戻る）。空の色は次の画面を消すときから使う。
	m_Pillars.Setup(m_Stage);
	SetClearColor(MAP_LOOKS[m_Map].Sky);
}

void Stage::HitPillar(int index, int source, int attacker, float damage, const VEC3& direction)
{
	m_Pillars.Hit(index, source, attacker, damage, direction);
}

int Stage::StepPillars(const CAPSULE (&bodies)[FIGHTER_COUNT], const bool (&hittable)[FIGHTER_COUNT], PILLAR_IMPACT impacts[], int maxImpacts)
{
	return m_Pillars.Step(GameTimeGetDeltaTime(), bodies, hittable, m_Stage, impacts, maxImpacts);
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
	if (m_MapModels[m_Map] >= 0) {
		DrawModelMap(m_MapModels[m_Map]);
		return;
	}
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
	// 景観のモデルは起動時に1回だけ読む（マップを選び直しても読み直さない）。
	for (int i = 0; i < STAGE_MAP_COUNT; ++i) m_MapModels[i] = MAP_LOOKS[i].Model ? ModelLoad(MAP_LOOKS[i].Model) : -1;
	m_ColumnModel = ModelLoad(COLUMN_MODEL);
	Reset();
}

void Stage::DrawModelMap(int model) const
{
	// 景観は頂点の色にライトを焼き込んであるので、ライトを切って描く。マップの広さの倍率は XZ だけに掛ける（高さはそのまま）。
	static constexpr XMFLOAT4 WHITE = { 1.0f, 1.0f, 1.0f, 1.0f };
	const float s = m_Scale;
	Shader_SetLightEnable(false);
	ModelDraw(model, { 0.0f, 0.0f, 0.0f }, { s, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, s }, WHITE);
	for (int i = 0; i < m_Pillars.GetCount(); ++i) {
		// 柱は根元を中心に、倒れる向きへ傾ける（太さだけ広さの倍率にする）。傷むほど暗くする。
		const PILLAR_POSE pose = m_Pillars.GetPose(i);
		const float shade = 1.0f - pose.Damage * COLUMN_DAMAGE_SHADE;
		ModelDraw(m_ColumnModel, { pose.Base.X, pose.Base.Y, pose.Base.Z },
			{ pose.Side.X * s, pose.Side.Y * s, pose.Side.Z * s }, { pose.Axis.X, pose.Axis.Y, pose.Axis.Z },
			{ pose.Front.X * s, pose.Front.Y * s, pose.Front.Z * s }, { shade, shade, shade, 1.0f });
	}
	Shader_SetLightEnable(true);
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
	const int index = static_cast<int>(&region - m_Stage.Regions);
	const float solidBottom = GetSolidDrawBottom(index);

	if (region.SlopeX == 0.0f && region.SlopeZ == 0.0f) {
		// 平らな区画：実体なら下の床（なければ地盤の下面）から上面まで、片面の橋なら薄い板。
		const float top = region.BaseY;
		const float bottom = region.Solid ? solidBottom : top - BRIDGE_DRAW_THICKNESS;
		const float height = top - bottom;
		const float middle = (top + bottom) * 0.5f;
		if (height <= 0.0f) return;
		if (region.DiagonalLimit <= 0.0f) {
			// 先に調べる区画で、この上面より低い所がある区画（荷道・坂など）は、そこを除いて描く。
			// 除かないと、当たり判定のない見た目だけの床がその上にかぶさって隠してしまう。
			// 穴は、いちばん深い穴の底までにとどめ、その下は1つの箱で描く（下から見て継ぎ目が出ないように）。
			float holes[STAGE_REGION_MAX][4];
			int holeCount = 0;
			for (int i = 0; i < index; ++i) {
				const STAGE_REGION& other = m_Stage.Regions[i];
				if (!IsHole(other, region)) continue;
				holes[holeCount][0] = other.MinX; holes[holeCount][1] = other.MaxX;
				holes[holeCount][2] = other.MinZ; holes[holeCount][3] = other.MaxZ;
				++holeCount;
			}
			const float holeBottom = region.Solid && holeCount > 0 ? fminf(fmaxf(GetHoleBottom(index), bottom), top) : bottom;
			if (holeBottom > bottom) {
				ShapeDrawBox({ centerX, (holeBottom + bottom) * 0.5f, centerZ }, { width, holeBottom - bottom, depth }, color);
			}
			if (top <= holeBottom) return;
			float pieces[32][4];
			const int count = MapCalcSubtractRects(region.MinX, region.MaxX, region.MinZ, region.MaxZ, holes, holeCount, pieces, 32);
			for (int i = 0; i < count; ++i) {
				const float* r = pieces[i];
				ShapeDrawBox({ (r[0] + r[1]) * 0.5f, (top + holeBottom) * 0.5f, (r[2] + r[3]) * 0.5f }, { r[1] - r[0], top - holeBottom, r[3] - r[2] }, color);
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

	const float centerTop = StageCalcRegionHeight(region, centerX, centerZ);
	if (region.Solid) {
		// 実体の坂：上面が当たり判定の式どおりに斜めの箱1つで、下の床（なければ地盤の下面）から描く。
		ShapeDrawSlopedBox(region.MinX, region.MaxX, region.MinZ, region.MaxZ, solidBottom,
			centerTop, region.SlopeX, region.SlopeZ, color);
		return;
	}
	// 片面の坂：上面に沿った板だけ。
	const bool alongX = region.SlopeX != 0.0f;
	const auto point = [&](float a) {
		const float x = alongX ? a : centerX;
		const float z = alongX ? centerZ : a;
		return XMFLOAT3{ x, StageCalcRegionHeight(region, x, z) - RAMP_SLAB_THICKNESS * 0.5f, z };
	};
	ShapeDrawBoxAlong(point(alongX ? region.MinX : region.MinZ), point(alongX ? region.MaxX : region.MaxZ),
		alongX ? depth : width, RAMP_SLAB_THICKNESS, color);
}

float Stage::GetRegionLow(const STAGE_REGION& region)
{
	// 上面は平面なので、いちばん低い所は四隅のどれか。
	return fminf(fminf(StageCalcRegionHeight(region, region.MinX, region.MinZ), StageCalcRegionHeight(region, region.MaxX, region.MinZ)),
		fminf(StageCalcRegionHeight(region, region.MinX, region.MaxZ), StageCalcRegionHeight(region, region.MaxX, region.MaxZ)));
}

bool Stage::IsHole(const STAGE_REGION& region, const STAGE_REGION& floor)
{
	const bool overlaps = region.MinX < floor.MaxX && region.MaxX > floor.MinX && region.MinZ < floor.MaxZ && region.MaxZ > floor.MinZ;
	return overlaps && GetRegionLow(region) < floor.BaseY - DRAW_HEIGHT_EPSILON;
}

float Stage::GetHoleBottom(int floorIndex) const
{
	const STAGE_REGION& floor = m_Stage.Regions[floorIndex];
	float bottom = floor.BaseY;
	for (int i = 0; i < floorIndex; ++i) {
		const STAGE_REGION& other = m_Stage.Regions[i];
		if (IsHole(other, floor)) bottom = fminf(bottom, GetRegionLow(other) - HOLE_FILL_THICKNESS);
	}
	return bottom;
}

float Stage::GetSolidDrawBottom(int index) const
{
	// あとに調べる平らな実体の区画（床）が、この区画の下を全部覆っていれば、床の上面（この区画が床の穴なら穴の底）から描けばよい。
	// 床は地盤の下面まで伸びているので、上に乗る区画まで地盤の下面へ伸ばすと、下から見たときに面が重なってちらつく。
	const STAGE_REGION& region = m_Stage.Regions[index];
	const float corners[4][2] = { { region.MinX, region.MinZ }, { region.MaxX, region.MinZ }, { region.MinX, region.MaxZ }, { region.MaxX, region.MaxZ } };
	for (int i = index + 1; i < m_Stage.RegionCount; ++i) {
		const STAGE_REGION& floor = m_Stage.Regions[i];
		if (!floor.Solid || floor.SlopeX != 0.0f || floor.SlopeZ != 0.0f) continue;
		bool covers = region.MinX >= floor.MinX && region.MaxX <= floor.MaxX && region.MinZ >= floor.MinZ && region.MaxZ <= floor.MaxZ;
		for (const auto& c : corners) {
			if (floor.DiagonalLimit > 0.0f && fabsf(c[0]) + fabsf(c[1]) > floor.DiagonalLimit) covers = false;
		}
		if (!covers) continue;
		if (!IsHole(region, floor)) return floor.BaseY;
		// 穴の区画は、床の穴の底から描く（穴の底は穴の区画のいちばん低い所以下になる）。床に穴を開けない床（八角形）は使わない。
		if (floor.DiagonalLimit <= 0.0f) return GetHoleBottom(i);
	}
	return m_Stage.GroundBottomY;
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
