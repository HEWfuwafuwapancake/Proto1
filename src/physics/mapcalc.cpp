#include <initializer_list>
#include <math.h>
#include "physics/mapcalc.h"

//------------------------------------------------------------------------------
//	調整用の値（台帳は標準倍率。区画は上から順に調べるので、上に重なる区画から並べる）
//------------------------------------------------------------------------------
static constexpr float OUT_Y = -15.0f;				// 場外の下の線
static constexpr float GROUND_BOTTOM_Y = -6.0f;		// 地盤の見える下面（側面の壁はここから上面まで）
// 象限の符号（P1〜P4・Ｇの順。北西・北東・南東・南西）
static constexpr float QUADRANTS[4][2] = { { -1.0f, 1.0f }, { 1.0f, 1.0f }, { 1.0f, -1.0f }, { -1.0f, -1.0f } };

// ---- ソラツギ中央郵便局（設計書 第08節）：中央台 → 屋根 → 坂 → 基準床の順 ----
// 並びは STAGE_REGION と同じ（範囲・四隅の切り落とし・高さの式 H = BaseY + SlopeX × x + SlopeZ × z・側面・目印）。
static constexpr STAGE_REGION POST_OFFICE_ROWS[] = {
	{ -8.0f, 8.0f, -8.0f, 8.0f, 14.0f, 1.2f, 0.0f, 0.0f, true, STAGE_SURFACE_CENTER },		// 仕分け台（八角形）
	{ -22.0f, -14.0f, -7.0f, 7.0f, 0.0f, 6.0f, 0.0f, 0.0f, true, STAGE_SURFACE_ROOF },		// 西の集配棟
	{ 14.0f, 22.0f, -7.0f, 7.0f, 0.0f, 6.0f, 0.0f, 0.0f, true, STAGE_SURFACE_ROOF },		// 東の集配棟
	{ -22.0f, -18.0f, 7.0f, 17.0f, 0.0f, 10.2f, 0.0f, -0.6f, true, STAGE_SURFACE_RAMP },	// 西の坂 H = 0.6 × (17 − z)
	{ 18.0f, 22.0f, -17.0f, -7.0f, 0.0f, 10.2f, 0.0f, 0.6f, true, STAGE_SURFACE_RAMP },		// 東の坂 H = 0.6 × (17 ＋ z)
	{ -26.0f, 26.0f, -26.0f, 26.0f, 46.0f, 0.0f, 0.0f, 0.0f, true, STAGE_SURFACE_FLOOR },	// 屋上（四隅を6ずつ切り落とす）
};

// ---- 汐跡市場の三本橋（設計書 第09節）：橋と坂を置いてから、残りに岸の路面を張る ----
static constexpr STAGE_REGION MARKET_ROWS[] = {
	{ -10.0f, -4.0f, -4.0f, 4.0f, 0.0f, -3.0f, 0.0f, 0.0f, true, STAGE_SURFACE_LOW_ROAD },	// 西の掘り下げた荷道
	{ 4.0f, 10.0f, -4.0f, 4.0f, 0.0f, -3.0f, 0.0f, 0.0f, true, STAGE_SURFACE_LOW_ROAD },	// 東の掘り下げた荷道
	{ -4.0f, 4.0f, -4.0f, 4.0f, 0.0f, -3.0f, 0.0f, 0.0f, false, STAGE_SURFACE_LOW_ROAD },	// 地割れの上の低い搬入橋
	{ -16.0f, -10.0f, -4.0f, 4.0f, 0.0f, -8.0f, -0.5f, 0.0f, true, STAGE_SURFACE_RAMP },	// 西の搬入橋の坂 H = (a − 16) ÷ 2
	{ 10.0f, 16.0f, -4.0f, 4.0f, 0.0f, -8.0f, 0.5f, 0.0f, true, STAGE_SURFACE_RAMP },		// 東の搬入橋の坂
	{ -4.0f, 4.0f, 10.0f, 14.0f, 0.0f, 3.0f, 0.0f, 0.0f, false, STAGE_SURFACE_TRAM },		// 北の高い橋（市電の屋根）
	{ -4.0f, 4.0f, -14.0f, -10.0f, 0.0f, 3.0f, 0.0f, 0.0f, false, STAGE_SURFACE_SHUTTER },	// 南の高い橋（シャッター）
	{ -12.0f, -4.0f, 10.0f, 14.0f, 0.0f, 4.5f, 0.375f, 0.0f, true, STAGE_SURFACE_RAMP },	// 高い橋の坂 H = 3 × (12 − a) ÷ 8
	{ 4.0f, 12.0f, 10.0f, 14.0f, 0.0f, 4.5f, -0.375f, 0.0f, true, STAGE_SURFACE_RAMP },
	{ -12.0f, -4.0f, -14.0f, -10.0f, 0.0f, 4.5f, 0.375f, 0.0f, true, STAGE_SURFACE_RAMP },
	{ 4.0f, 12.0f, -14.0f, -10.0f, 0.0f, 4.5f, -0.375f, 0.0f, true, STAGE_SURFACE_RAMP },
	{ -27.0f, -21.0f, 4.0f, 10.0f, 0.0f, 1.2f, 0.0f, 0.0f, true, STAGE_SURFACE_BASE },		// 氷蔵庫跡の基礎
	{ 21.0f, 27.0f, -10.0f, -4.0f, 0.0f, 1.2f, 0.0f, 0.0f, true, STAGE_SURFACE_BASE },		// 切符売場跡の基礎
	{ -30.0f, -4.0f, -24.0f, 24.0f, 0.0f, 0.0f, 0.0f, 0.0f, true, STAGE_SURFACE_FLOOR },	// 西岸の路面
	{ 4.0f, 30.0f, -24.0f, 24.0f, 0.0f, 0.0f, 0.0f, 0.0f, true, STAGE_SURFACE_FLOOR },		// 東岸の路面
};

/// @brief 郵便局・市場の、開始位置・Ｇなどの値（標準倍率。符号は QUADRANTS で付ける）
struct MAP_POINTS
{
	float SpawnX, SpawnZ;					// P1 の開始位置の絶対値
	float LiftStartX, LiftStartZ;			// Ｇの始点の絶対値
	float LiftLandX, LiftLandY, LiftLandZ;	// Ｇの名目着地点の絶対値と高さ
	float LiftSideSign;						// 塞がれたとき横へずらす向き
	float HalfWidth, HalfDepth;				// 全体を囲む半分の幅・奥行き
	float LowestY;							// いちばん低い床の高さ
};

static constexpr MAP_POINTS POST_OFFICE_POINTS{ 12.0f, 17.0f, 12.0f, 10.0f, 16.0f, 6.0f, 5.0f, 1.0f, 26.0f, 26.0f, 0.0f };
static constexpr MAP_POINTS MARKET_POINTS{ 17.0f, 9.0f, 8.0f, 7.0f, 3.0f, 3.0f, 12.0f, -1.0f, 30.0f, 24.0f, -3.0f };

// ---- ネオンビル群（仕様書 01。戦闘範囲 X ±28・Z ±24。道路の上面 Y = 0） ----
/// @brief 商業ビル1棟（中心・幅・奥行き・高さと、屋上の設備があるか）
struct NEON_SHOP
{
	float X, Z, Width, Depth, Height;	// 中心の X・Z、幅（X）・奥行き（Z）・高さ
	bool RoofUnit;						// 屋上に設備の箱があるか（VINYL・LUX・STAR・KILO）
};
static constexpr NEON_SHOP NEON_SHOPS[] = {
	{ -24.0f, -17.2f, 6.8f, 8.2f, 9.6f, false },	// SODA
	{ -17.0f, -18.4f, 6.3f, 9.0f, 13.2f, true },	// VINYL
	{ -9.2f, -18.1f, 8.3f, 9.2f, 16.5f, false },	// NOVA
	{ 9.0f, -17.6f, 7.7f, 8.9f, 10.5f, false },		// BEAT
	{ 16.8f, -18.5f, 6.3f, 9.3f, 15.2f, true },		// LUX
	{ 24.0f, -17.2f, 6.8f, 8.2f, 8.8f, false },		// VOLT
	{ -24.0f, 17.8f, 6.8f, 8.5f, 12.5f, true },		// STAR
	{ -16.8f, 17.4f, 6.6f, 8.4f, 8.2f, false },		// ZAP
	{ -9.0f, 18.2f, 7.8f, 9.0f, 14.3f, false },		// WAVE
	{ 9.4f, 17.7f, 8.3f, 8.9f, 11.7f, false },		// ORBIT
	{ 17.1f, 18.3f, 6.0f, 8.5f, 14.0f, false },		// PIXEL
	{ 24.0f, 17.3f, 6.8f, 8.5f, 9.0f, true },		// KILO
};
static constexpr float ROOF_UNIT_WIDTH_RATE = 0.68f;	// 屋上の設備の幅（ビルの幅に対して）
static constexpr float ROOF_UNIT_DEPTH_RATE = 0.70f;	// 屋上の設備の奥行き（ビルの奥行きに対して）
static constexpr float ROOF_UNIT_HEIGHT = 1.64f;		// 屋上の設備の高さ
/// @brief 車1台（中心と、車室をずらす向き）
struct NEON_CAR
{
	float X, Z;		// 車体の中心
	float Facing;	// 前の向き（+1 で +X。車室は後ろへ少しずれる）
};
static constexpr NEON_CAR NEON_CARS[] = { { -14.0f, -6.5f, 1.0f }, { 14.0f, 6.5f, -1.0f } };
static constexpr float CAR_WIDTH = 4.65f, CAR_DEPTH = 1.75f, CAR_TOP = 0.93f;		// 車体
static constexpr float CABIN_WIDTH = 2.26f, CABIN_DEPTH = 1.50f, CABIN_TOP = 1.425f;	// 車室
static constexpr float CABIN_SHIFT = 0.15f;			// 車室を車体の中心から後ろへずらす距離
static constexpr float NEON_PADS[][2] = { { -15.0f, 9.0f }, { 15.0f, -9.0f } };	// ジャンプ台の中心
static constexpr float PAD_SIZE = 2.0f, PAD_TOP = 0.2f;
static constexpr float SIDEWALK_X = 16.65f, SIDEWALK_Z = 16.9f;		// 歩道4枚の中心の絶対値
static constexpr float SIDEWALK_WIDTH = 22.7f, SIDEWALK_DEPTH = 14.2f, SIDEWALK_TOP = 0.18f;
static constexpr float NEON_HALF_WIDTH = 28.0f, NEON_HALF_DEPTH = 24.0f;
static constexpr VEC3 NEON_SPAWNS[STAGE_SPAWN_MAX] = { { -21.0f, 0.0f, 0.0f }, { 21.0f, 0.0f, 0.0f }, { -21.0f, 0.0f, 4.0f }, { 21.0f, 0.0f, -4.0f } };

// ---- コロシアム（仕様書 02。半径 21 の円い石畳。上面 Y = 0） ----
static constexpr float COLISEUM_RADIUS = 21.0f;
static constexpr float COLUMN_WIDTH = 2.04f;		// 柱の当たりの一辺
static constexpr float COLUMN_HEIGHT = 7.5f;		// 柱の高さ（倒れたあとの長さ）
static constexpr float COLISEUM_COLUMNS[][2] = { { 0.0f, 0.0f }, { -8.3f, -9.3f }, { 8.3f, -9.3f }, { -8.3f, 9.3f }, { 8.3f, 9.3f } };
static constexpr float COLISEUM_RUINS[][2] = { { -4.7f, -17.1f }, { 4.7f, 17.1f } };	// 低い石壁の中心
static constexpr float RUIN_WIDTH = 3.8f, RUIN_DEPTH = 1.2f, RUIN_TOP = 1.16f, RUIN_YAW = -0.25f;
static constexpr VEC3 COLISEUM_SPAWNS[STAGE_SPAWN_MAX] = { { -16.0f, 0.0f, 0.0f }, { 16.0f, 0.0f, 0.0f }, { -12.0f, 0.0f, 9.0f }, { 12.0f, 0.0f, -9.0f } };

// ---- 廃駅（仕様書 03。線路 Y = 0、ホーム Y = 1.5。戦闘範囲 X ±28・Z ±24） ----
static constexpr float STATION_HALF_WIDTH = 28.0f, STATION_HALF_DEPTH = 24.0f;
static constexpr float PLATFORM_TOP = 1.5f;			// ホームの上面
static constexpr float PLATFORM_EDGE = 8.0f;		// ホームの線路側のふち（|Z|）
static constexpr float ACCESS_X[] = { -25.0f, -1.8f, 24.4f };	// ホームと線路をつなぐ坂の中心の X
static constexpr float ACCESS_WIDTH = 3.3f, ACCESS_LENGTH = 4.4f;
static constexpr float TRAIN_X[] = { -18.5f, -8.3f };	// 廃電車の中心の X
static constexpr float TRAIN_Z = -3.6f;
static constexpr float TRAIN_LENGTH = 9.62f, TRAIN_WIDTH = 3.05f, TRAIN_TOP = 3.22f;	// 車体
static constexpr float TRAIN_ROOF_INSET = 0.35f, TRAIN_ROOF_WIDTH = 2.7f, TRAIN_ROOF_TOP = 3.5f;	// 屋根
/// @brief 屋根付き待合所（中心と幅）
struct STATION_SHELTER
{
	float X, Z, Width;	// 中心の X・Z と、屋根の全体の幅
};
static constexpr STATION_SHELTER SHELTERS[] = { { -10.0f, -21.1f, 16.0f }, { 1.5f, 21.1f, 13.0f } };
static constexpr float SHELTER_POST_INSET = 1.1f;		// 柱を屋根の端から内側へ置く距離
static constexpr float SHELTER_POST_WIDTH = 0.24f, SHELTER_POST_DEPTH = 0.26f, SHELTER_POST_TOP = 5.22f;
static constexpr float SHELTER_ROOF_DEPTH = 3.7f, SHELTER_ROOF_TOP = 5.52f;
static constexpr float SHELTER_LEFT_RATE = 0.54f, SHELTER_RIGHT_RATE = 0.27f;	// 屋根の左右の板の幅（真ん中は抜けている）
static constexpr float BENCH_WIDTH = 3.65f, BENCH_DEPTH = 0.64f, BENCH_TOP = 2.16f;
static constexpr float STATION_TREES[][2] = { { -25.0f, -21.0f }, { 8.0f, -22.7f }, { 25.6f, -14.0f }, { -26.0f, 12.8f }, { -12.4f, 22.4f }, { 25.0f, 22.2f } };
static constexpr float TRUNK_SIZE = 0.30f, TRUNK_HEIGHT = 2.92f;
/// @brief 瓦礫1つ（中心・幅・高さ・奥行き。ホームの上に置く）
struct STATION_RUBBLE
{
	float X, Z, Width, Height, Depth;	// 中心の X・Z、幅（X）・高さ・奥行き（Z）
};
static constexpr STATION_RUBBLE RUBBLE[] = {
	{ -14.6f, -11.2f, 3.8f, 1.12f, 0.82f }, { 7.2f, -18.8f, 2.5f, 1.3f, 0.85f },
	{ -3.4f, 12.4f, 3.6f, 1.25f, 0.9f }, { 25.3f, 9.7f, 2.3f, 0.85f, 1.1f },
};
static constexpr float VIADUCT_X = 10.0f;			// 倒壊ビルの中心線の中心の X（Z は 0）
static constexpr float VIADUCT_YAW = 0.435f;		// 倒壊ビルの中心線の向き
static constexpr float VIADUCT_WIDTH = 5.8f;
static constexpr float VIADUCT_HALF_LENGTH = 18.0f;
static constexpr float VIADUCT_PEAK_AT = 3.8f;		// 中心線に沿って、いちばん高い所の位置
static constexpr float VIADUCT_PEAK_TOP = 6.4f;		// いちばん高い所の上面
static constexpr VEC3 STATION_SPAWNS[STAGE_SPAWN_MAX] = {
	{ -22.0f, PLATFORM_TOP, -16.0f }, { 22.0f, PLATFORM_TOP, 16.0f }, { 22.0f, PLATFORM_TOP, -16.0f }, { -22.0f, PLATFORM_TOP, 16.0f },
};


STAGE_MAP MapCalcCycle(STAGE_MAP map, int direction)
{
	return static_cast<STAGE_MAP>((map + direction % STAGE_MAP_COUNT + STAGE_MAP_COUNT) % STAGE_MAP_COUNT);
}

// 中心・大きさ・上面の高さから、平らな箱の区画を作る（回すときは中心のまわりに回す）。
static STAGE_REGION BoxRegion(float x, float z, float width, float depth, float top, STAGE_SURFACE surface,
	bool solid = true, float yaw = 0.0f)
{
	STAGE_REGION region{ x - width * 0.5f, x + width * 0.5f, z - depth * 0.5f, z + depth * 0.5f, 0.0f, top, 0.0f, 0.0f, solid, surface };
	region.Yaw = yaw;
	return region;
}

static void AddRegion(STAGE& stage, const STAGE_REGION& region)
{
	if (stage.RegionCount < STAGE_REGION_MAX) stage.Regions[stage.RegionCount++] = region;
}

// 地形のマップの共通の値を入れる（区画・開始位置などは、このあと標準倍率で足す）。
static void BeginTerrain(STAGE& stage, float halfWidth, float halfDepth, float lowestY)
{
	stage = STAGE{};
	stage.HalfWidth = halfWidth;
	stage.HalfDepth = halfDepth;
	stage.TopY = lowestY;
	stage.Thickness = lowestY - GROUND_BOTTOM_Y;
	stage.OutY = OUT_Y;
	stage.HasTerrain = true;
	stage.GroundBottomY = GROUND_BOTTOM_Y;
}

static void SetSpawns(STAGE& stage, const VEC3 (&spawns)[STAGE_SPAWN_MAX])
{
	stage.SpawnCount = STAGE_SPAWN_MAX;
	for (int i = 0; i < STAGE_SPAWN_MAX; ++i) stage.Spawns[i] = spawns[i];
}

// 郵便局・市場：台帳の区画と、象限で対称な開始位置・Ｇ。郵便局にはダッシュ板もある。
template <int ROW_COUNT>
static void BuildLedger(const STAGE_REGION (&rows)[ROW_COUNT], const MAP_POINTS& points, STAGE& stage)
{
	BeginTerrain(stage, points.HalfWidth, points.HalfDepth, points.LowestY);
	for (const STAGE_REGION& row : rows) AddRegion(stage, row);
	stage.SpawnCount = STAGE_SPAWN_MAX;
	stage.LiftCount = STAGE_LIFT_MAX;
	stage.LiftSideSign = points.LiftSideSign;
	for (int i = 0; i < 4; ++i) {
		const float sx = QUADRANTS[i][0], sz = QUADRANTS[i][1];
		stage.Spawns[i] = { sx * points.SpawnX, 0.0f, sz * points.SpawnZ };
		stage.Lifts[i] = { sx * points.LiftStartX, sz * points.LiftStartZ, { sx * points.LiftLandX, points.LiftLandY, sz * points.LiftLandZ } };
	}
}

static void BuildPostOffice(STAGE& stage)
{
	BuildLedger(POST_OFFICE_ROWS, POST_OFFICE_POINTS, stage);
	// 南北の搬入口から仕分け台へ向かうダッシュ板（X −3〜3、|Z| 8〜17）。
	stage.BeltCount = 2;
	stage.Belts[0] = { -3.0f, 3.0f, 8.0f, 17.0f };
	stage.Belts[1] = { -3.0f, 3.0f, -17.0f, -8.0f };
}

static void BuildNeon(STAGE& stage)
{
	BeginTerrain(stage, NEON_HALF_WIDTH, NEON_HALF_DEPTH, 0.0f);
	for (const auto& pad : NEON_PADS) AddRegion(stage, BoxRegion(pad[0], pad[1], PAD_SIZE, PAD_SIZE, PAD_TOP, STAGE_SURFACE_PAD));
	for (const NEON_CAR& car : NEON_CARS) {
		AddRegion(stage, BoxRegion(car.X - CABIN_SHIFT * car.Facing, car.Z, CABIN_WIDTH, CABIN_DEPTH, CABIN_TOP, STAGE_SURFACE_BLOCK));
		AddRegion(stage, BoxRegion(car.X, car.Z, CAR_WIDTH, CAR_DEPTH, CAR_TOP, STAGE_SURFACE_BLOCK));
	}
	for (const NEON_SHOP& shop : NEON_SHOPS) {
		if (!shop.RoofUnit) continue;
		AddRegion(stage, BoxRegion(shop.X, shop.Z, shop.Width * ROOF_UNIT_WIDTH_RATE, shop.Depth * ROOF_UNIT_DEPTH_RATE,
			shop.Height + ROOF_UNIT_HEIGHT, STAGE_SURFACE_BLOCK));
	}
	for (const NEON_SHOP& shop : NEON_SHOPS) AddRegion(stage, BoxRegion(shop.X, shop.Z, shop.Width, shop.Depth, shop.Height, STAGE_SURFACE_BLOCK));
	for (const auto& quadrant : QUADRANTS) {
		AddRegion(stage, BoxRegion(quadrant[0] * SIDEWALK_X, quadrant[1] * SIDEWALK_Z, SIDEWALK_WIDTH, SIDEWALK_DEPTH, SIDEWALK_TOP, STAGE_SURFACE_FLOOR));
	}
	AddRegion(stage, BoxRegion(0.0f, 0.0f, NEON_HALF_WIDTH * 2.0f, NEON_HALF_DEPTH * 2.0f, 0.0f, STAGE_SURFACE_ROAD));
	SetSpawns(stage, NEON_SPAWNS);
}

static void BuildColiseum(STAGE& stage)
{
	BeginTerrain(stage, COLISEUM_RADIUS, COLISEUM_RADIUS, 0.0f);
	stage.RoundOutline = true;	// 横の壁は円い床に沿わせる
	// 柱は倒れると長い箱になり、石壁や床の上に重なるので先に調べる。
	for (const auto& column : COLISEUM_COLUMNS) {
		AddRegion(stage, BoxRegion(column[0], column[1], COLUMN_WIDTH, COLUMN_WIDTH, COLUMN_HEIGHT, STAGE_SURFACE_PILLAR));
	}
	for (const auto& ruin : COLISEUM_RUINS) {
		AddRegion(stage, BoxRegion(ruin[0], ruin[1], RUIN_WIDTH, RUIN_DEPTH, RUIN_TOP, STAGE_SURFACE_BLOCK, true, RUIN_YAW));
	}
	STAGE_REGION floor = BoxRegion(0.0f, 0.0f, COLISEUM_RADIUS * 2.0f, COLISEUM_RADIUS * 2.0f, 0.0f, STAGE_SURFACE_FLOOR);
	floor.Radius = COLISEUM_RADIUS;
	AddRegion(stage, floor);
	SetSpawns(stage, COLISEUM_SPAWNS);
}

// 倒壊ビルの中心線に沿った位置での上面の高さ（両端がホームの高さ、VIADUCT_PEAK_AT がいちばん高い）。
static float ViaductTop(float along)
{
	const float rate = along < VIADUCT_PEAK_AT
		? (along + VIADUCT_HALF_LENGTH) / (VIADUCT_PEAK_AT + VIADUCT_HALF_LENGTH)
		: (VIADUCT_HALF_LENGTH - along) / (VIADUCT_HALF_LENGTH - VIADUCT_PEAK_AT);
	return PLATFORM_TOP + (VIADUCT_PEAK_TOP - PLATFORM_TOP) * rate;
}

// 倒壊ビルの1区間（中心線に沿った位置 start〜end）を、中心線の向きに上り下りする片面の坂にする。
static STAGE_REGION ViaductRegion(float start, float end)
{
	const float s = sinf(VIADUCT_YAW), c = cosf(VIADUCT_YAW);
	const float middle = (start + end) * 0.5f;
	const float grade = (ViaductTop(end) - ViaductTop(start)) / (end - start);
	STAGE_REGION region = BoxRegion(VIADUCT_X + middle * s, middle * c, VIADUCT_WIDTH, end - start, 0.0f, STAGE_SURFACE_DECK, false, VIADUCT_YAW);
	// 中心線に沿った位置は along = (x − VIADUCT_X) sin ＋ z cos。高さ = 始まりの高さ ＋ grade × (along − start) を x・z の式にする。
	region.BaseY = ViaductTop(start) - grade * (start + VIADUCT_X * s);
	region.SlopeX = grade * s;
	region.SlopeZ = grade * c;
	return region;
}

static void BuildStation(STAGE& stage)
{
	BeginTerrain(stage, STATION_HALF_WIDTH, STATION_HALF_DEPTH, 0.0f);
	// 片面の足場（倒壊ビル・待合所の屋根）は、下をくぐれるように、下の区画より先に調べる。
	AddRegion(stage, ViaductRegion(-VIADUCT_HALF_LENGTH, VIADUCT_PEAK_AT));
	AddRegion(stage, ViaductRegion(VIADUCT_PEAK_AT, VIADUCT_HALF_LENGTH));
	for (const STATION_SHELTER& shelter : SHELTERS) {
		const float left = shelter.Width * SHELTER_LEFT_RATE, right = shelter.Width * SHELTER_RIGHT_RATE;
		AddRegion(stage, BoxRegion(shelter.X - (shelter.Width - left) * 0.5f, shelter.Z, left, SHELTER_ROOF_DEPTH, SHELTER_ROOF_TOP, STAGE_SURFACE_DECK, false));
		AddRegion(stage, BoxRegion(shelter.X + (shelter.Width - right) * 0.5f, shelter.Z, right, SHELTER_ROOF_DEPTH, SHELTER_ROOF_TOP, STAGE_SURFACE_DECK, false));
	}
	for (float x : TRAIN_X) {
		AddRegion(stage, BoxRegion(x, TRAIN_Z, TRAIN_LENGTH - TRAIN_ROOF_INSET, TRAIN_ROOF_WIDTH, TRAIN_ROOF_TOP, STAGE_SURFACE_BLOCK));
		AddRegion(stage, BoxRegion(x, TRAIN_Z, TRAIN_LENGTH, TRAIN_WIDTH, TRAIN_TOP, STAGE_SURFACE_BLOCK));
	}
	for (const STATION_SHELTER& shelter : SHELTERS) {
		for (float side : { -1.0f, 1.0f }) {
			const float x = shelter.X + side * (shelter.Width * 0.5f - SHELTER_POST_INSET);
			AddRegion(stage, BoxRegion(x, shelter.Z, SHELTER_POST_WIDTH, SHELTER_POST_DEPTH, SHELTER_POST_TOP, STAGE_SURFACE_BLOCK));
		}
		AddRegion(stage, BoxRegion(shelter.X, shelter.Z, BENCH_WIDTH, BENCH_DEPTH, BENCH_TOP, STAGE_SURFACE_BLOCK));
	}
	for (const auto& tree : STATION_TREES) AddRegion(stage, BoxRegion(tree[0], tree[1], TRUNK_SIZE, TRUNK_SIZE, PLATFORM_TOP + TRUNK_HEIGHT, STAGE_SURFACE_BLOCK));
	for (const STATION_RUBBLE& rubble : RUBBLE) {
		AddRegion(stage, BoxRegion(rubble.X, rubble.Z, rubble.Width, rubble.Depth, PLATFORM_TOP + rubble.Height, STAGE_SURFACE_BLOCK));
	}
	// ホームと線路をつなぐ坂（線路側の端が高さ 0、ホーム側の端がホームの高さ）。
	const float grade = PLATFORM_TOP / ACCESS_LENGTH;
	const float rampZ = PLATFORM_EDGE - ACCESS_LENGTH * 0.5f;
	for (float sign : { -1.0f, 1.0f }) {
		for (float x : ACCESS_X) {
			STAGE_REGION ramp = BoxRegion(x, sign * rampZ, ACCESS_WIDTH, ACCESS_LENGTH, 0.0f, STAGE_SURFACE_RAMP);
			ramp.BaseY = -(PLATFORM_EDGE - ACCESS_LENGTH) * grade;	// H = grade × (|z| − 線路側の端の |z|)
			ramp.SlopeZ = sign * grade;
			AddRegion(stage, ramp);
		}
	}
	const float platformDepth = STATION_HALF_DEPTH - PLATFORM_EDGE;
	for (float sign : { -1.0f, 1.0f }) {
		AddRegion(stage, BoxRegion(0.0f, sign * (PLATFORM_EDGE + platformDepth * 0.5f), STATION_HALF_WIDTH * 2.0f, platformDepth, PLATFORM_TOP, STAGE_SURFACE_FLOOR));
	}
	AddRegion(stage, BoxRegion(0.0f, 0.0f, STATION_HALF_WIDTH * 2.0f, STATION_HALF_DEPTH * 2.0f, 0.0f, STAGE_SURFACE_ROAD));
	SetSpawns(stage, STATION_SPAWNS);
}

// 標準倍率で作ったステージの XZ を倍率倍にする（坂の傾きは 1/倍率 にして、台帳を x = 世界X ÷ s で引くのと同じにする。高さはそのまま）。
static void ScaleStage(STAGE& stage, float scale)
{
	stage.HalfWidth *= scale;
	stage.HalfDepth *= scale;
	for (int i = 0; i < stage.RegionCount; ++i) {
		STAGE_REGION& region = stage.Regions[i];
		region.MinX *= scale;
		region.MaxX *= scale;
		region.MinZ *= scale;
		region.MaxZ *= scale;
		region.DiagonalLimit *= scale;
		region.Radius *= scale;
		region.SlopeX /= scale;
		region.SlopeZ /= scale;
	}
	for (int i = 0; i < stage.SpawnCount; ++i) {
		stage.Spawns[i].X *= scale;
		stage.Spawns[i].Z *= scale;
	}
	for (int i = 0; i < stage.LiftCount; ++i) {
		STAGE_LIFT& lift = stage.Lifts[i];
		lift.StartX *= scale;
		lift.StartZ *= scale;
		lift.Landing.X *= scale;
		lift.Landing.Z *= scale;
	}
	for (int i = 0; i < stage.BeltCount; ++i) {
		STAGE_BELT& belt = stage.Belts[i];
		belt = { belt.MinX * scale, belt.MaxX * scale, belt.MinZ * scale, belt.MaxZ * scale };
	}
}

bool MapCalcBuild(STAGE_MAP map, float scale, STAGE& stage)
{
	switch (map) {
	case STAGE_MAP_POST_OFFICE: BuildPostOffice(stage); break;
	case STAGE_MAP_MARKET: BuildLedger(MARKET_ROWS, MARKET_POINTS, stage); break;
	case STAGE_MAP_NEON: BuildNeon(stage); break;
	case STAGE_MAP_COLISEUM: BuildColiseum(stage); break;
	case STAGE_MAP_STATION: BuildStation(stage); break;
	default: return false;
	}
	ScaleStage(stage, scale);
	return true;
}

int MapCalcSubtractRects(float minX, float maxX, float minZ, float maxZ, const float (*holes)[4], int holeCount,
	float (*pieces)[4], int maxPieces)
{
	static constexpr int WORK_MAX = 64;
	float current[WORK_MAX][4] = { { minX, maxX, minZ, maxZ } };
	int count = 1;
	for (int h = 0; h < holeCount; ++h) {
		const float* hole = holes[h];
		float next[WORK_MAX][4];
		int nextCount = 0;
		const auto add = [&](float x0, float x1, float z0, float z1) {
			if (x1 - x0 <= 0.0f || z1 - z0 <= 0.0f || nextCount >= WORK_MAX) return;
			next[nextCount][0] = x0; next[nextCount][1] = x1; next[nextCount][2] = z0; next[nextCount][3] = z1;
			++nextCount;
		};
		for (int i = 0; i < count; ++i) {
			const float* r = current[i];
			// 重ならなければそのまま。重なれば、穴の左右・前後の4つに分ける。
			if (hole[0] >= r[1] || hole[1] <= r[0] || hole[2] >= r[3] || hole[3] <= r[2]) { add(r[0], r[1], r[2], r[3]); continue; }
			const float x0 = fmaxf(r[0], hole[0]), x1 = fminf(r[1], hole[1]);
			add(r[0], x0, r[2], r[3]);
			add(x1, r[1], r[2], r[3]);
			add(x0, x1, r[2], fmaxf(r[2], hole[2]));
			add(x0, x1, fminf(r[3], hole[3]), r[3]);
		}
		for (int i = 0; i < nextCount; ++i) for (int k = 0; k < 4; ++k) current[i][k] = next[i][k];
		count = nextCount;
	}
	const int written = count < maxPieces ? count : maxPieces;
	for (int i = 0; i < written; ++i) for (int k = 0; k < 4; ++k) pieces[i][k] = current[i][k];
	return written;
}
