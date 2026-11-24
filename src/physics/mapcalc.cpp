#include <math.h>
#include "physics/mapcalc.h"

//------------------------------------------------------------------------------
//	調整用の値（台帳は標準倍率。設計書 第08・09・11・12節）
//------------------------------------------------------------------------------
static constexpr float OUT_Y = -15.0f;				// 場外の下の線
static constexpr float GROUND_BOTTOM_Y = -6.0f;		// 地盤の見える下面（側面の壁はここから上面まで）
static constexpr float BELT_SPEED = 3.0f;			// 荷送りベルトの速さ（1秒に進む距離）
static constexpr float CAN_TILT_DEGREES = 15.0f;	// 空き缶の揺れのいちばん大きい傾き
static constexpr float CAN_SHAKE_PERIOD = 12.0f;	// 空き缶が1往復する更新数
static constexpr float PI = 3.14159265f;
// 補給区画の中心に足して、1区画に4つの地点を作るずれ（北西・北東・南東・南西の順）
static constexpr float SUPPLY_OFFSETS[4][2] = { { -1.0f, 1.0f }, { 1.0f, 1.0f }, { 1.0f, -1.0f }, { -1.0f, -1.0f } };
// 象限の符号（P1〜P4・Ｇ・補給区画の順。北西・北東・南東・南西）
static constexpr float QUADRANTS[4][2] = { { -1.0f, 1.0f }, { 1.0f, 1.0f }, { 1.0f, -1.0f }, { -1.0f, -1.0f } };

/// @brief 台帳の1行（標準倍率の範囲と高さの式）
struct MAP_ROW
{
	float MinX, MaxX, MinZ, MaxZ;	// 範囲
	float DiagonalLimit;			// |x|＋|z| の上限（0 なら使わない）
	float BaseY, SlopeX, SlopeZ;	// 上面の高さ H = BaseY + SlopeX × x + SlopeZ × z
	bool Solid;						// 側面があるか（false は上面だけの橋）
	STAGE_SURFACE Surface;			// 色と効果の目印
};

// ソラツギ中央郵便局：中央台 → 屋根 → 坂 → 基準床の順に調べる。
static constexpr MAP_ROW POST_OFFICE_ROWS[] = {
	{ -8.0f, 8.0f, -8.0f, 8.0f, 14.0f, 1.2f, 0.0f, 0.0f, true, STAGE_SURFACE_CENTER },		// 仕分け台（八角形）
	{ -22.0f, -14.0f, -7.0f, 7.0f, 0.0f, 6.0f, 0.0f, 0.0f, true, STAGE_SURFACE_ROOF },		// 西の集配棟
	{ 14.0f, 22.0f, -7.0f, 7.0f, 0.0f, 6.0f, 0.0f, 0.0f, true, STAGE_SURFACE_ROOF },		// 東の集配棟
	{ -22.0f, -18.0f, 7.0f, 17.0f, 0.0f, 10.2f, 0.0f, -0.6f, true, STAGE_SURFACE_RAMP },	// 西の坂 H = 0.6 × (17 − z)
	{ 18.0f, 22.0f, -17.0f, -7.0f, 0.0f, 10.2f, 0.0f, 0.6f, true, STAGE_SURFACE_RAMP },		// 東の坂 H = 0.6 × (17 ＋ z)
	{ -26.0f, 26.0f, -26.0f, 26.0f, 46.0f, 0.0f, 0.0f, 0.0f, true, STAGE_SURFACE_FLOOR },	// 屋上（四隅を6ずつ切り落とす）
};

// 汐跡市場の三本橋：橋と坂を置いてから、残りに岸の路面を張る。
static constexpr MAP_ROW MARKET_ROWS[] = {
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

/// @brief マップごとの、開始位置・Ｇ・補給区画などの値（標準倍率。符号は QUADRANTS で付ける）
struct MAP_POINTS
{
	float SpawnX, SpawnZ;					// P1 の開始位置の絶対値
	float LiftStartX, LiftStartZ;			// Ｇの始点の絶対値
	float LiftLandX, LiftLandY, LiftLandZ;	// Ｇの名目着地点の絶対値と高さ
	float LiftSideSign;						// 塞がれたとき横へずらす向き
	float SupplyX, SupplyY, SupplyZ;		// 補給区画の中心の絶対値と床の高さ
	float HalfWidth, HalfDepth;				// 全体を囲む半分の幅・奥行き
	float LowestY;							// いちばん低い床の高さ
};

static constexpr MAP_POINTS POST_OFFICE_POINTS{ 12.0f, 17.0f, 12.0f, 10.0f, 16.0f, 6.0f, 5.0f, 1.0f, 4.0f, 1.2f, 3.5f, 26.0f, 26.0f, 0.0f };
static constexpr MAP_POINTS MARKET_POINTS{ 17.0f, 9.0f, 8.0f, 7.0f, 3.0f, 3.0f, 12.0f, -1.0f, 7.0f, -3.0f, 2.0f, 30.0f, 24.0f, -3.0f };

STAGE_MAP MapCalcCycle(STAGE_MAP map, int direction)
{
	return static_cast<STAGE_MAP>((map + direction % STAGE_MAP_COUNT + STAGE_MAP_COUNT) % STAGE_MAP_COUNT);
}

// 台帳の行を倍率倍の区画にする（範囲を s 倍、傾きを 1/s 倍にして、台帳の x = 世界X ÷ s で引くのと同じにする）。
static STAGE_REGION ScaleRow(const MAP_ROW& row, float scale)
{
	return { row.MinX * scale, row.MaxX * scale, row.MinZ * scale, row.MaxZ * scale, row.DiagonalLimit * scale,
		row.BaseY, row.SlopeX / scale, row.SlopeZ / scale, row.Solid, row.Surface };
}

bool MapCalcBuild(STAGE_MAP map, float scale, STAGE& stage)
{
	if (map != STAGE_MAP_POST_OFFICE && map != STAGE_MAP_MARKET) return false;
	const bool post = map == STAGE_MAP_POST_OFFICE;
	const MAP_ROW* rows = post ? POST_OFFICE_ROWS : MARKET_ROWS;
	const int rowCount = post ? (int)(sizeof(POST_OFFICE_ROWS) / sizeof(POST_OFFICE_ROWS[0]))
		: (int)(sizeof(MARKET_ROWS) / sizeof(MARKET_ROWS[0]));
	const MAP_POINTS& points = post ? POST_OFFICE_POINTS : MARKET_POINTS;

	stage = STAGE{};
	stage.CenterX = 0.0f;
	stage.CenterZ = 0.0f;
	stage.HalfWidth = points.HalfWidth * scale;
	stage.HalfDepth = points.HalfDepth * scale;
	stage.TopY = points.LowestY;
	stage.Thickness = points.LowestY - GROUND_BOTTOM_Y;
	stage.OutY = OUT_Y;
	stage.HasTerrain = true;
	stage.GroundBottomY = GROUND_BOTTOM_Y;
	stage.RegionCount = rowCount;
	for (int i = 0; i < rowCount; ++i) stage.Regions[i] = ScaleRow(rows[i], scale);

	stage.SpawnCount = STAGE_SPAWN_MAX;
	stage.LiftCount = STAGE_LIFT_MAX;
	stage.LiftSideSign = points.LiftSideSign;
	for (int i = 0; i < 4; ++i) {
		const float sx = QUADRANTS[i][0], sz = QUADRANTS[i][1];
		stage.Spawns[i] = { sx * points.SpawnX * scale, 0.0f, sz * points.SpawnZ * scale };
		stage.Lifts[i] = { sx * points.LiftStartX * scale, sz * points.LiftStartZ * scale,
			{ sx * points.LiftLandX * scale, points.LiftLandY, sz * points.LiftLandZ * scale } };
		// 区画の中心を北西・北東・南東・南西の順に並べ、各中心にずれを4つ足す（3×3 の区画に4点ずつ、計16点）。
		for (int k = 0; k < 4; ++k) {
			stage.Supplies[i * 4 + k] = { (sx * points.SupplyX + SUPPLY_OFFSETS[k][0]) * scale, points.SupplyY,
				(sz * points.SupplyZ + SUPPLY_OFFSETS[k][1]) * scale };
		}
	}
	stage.SupplyCount = STAGE_SUPPLY_MAX;

	if (post) {
		// 南北の搬入口から仕分け台へ向かうベルト（X −3〜3、|Z| 8〜17）。
		stage.BeltCount = 2;
		stage.Belts[0] = { -3.0f * scale, 3.0f * scale, 8.0f * scale, 17.0f * scale };
		stage.Belts[1] = { -3.0f * scale, 3.0f * scale, -17.0f * scale, -8.0f * scale };
		stage.BeltSpeed = BELT_SPEED;
	}
	else {
		// 南の橋の両端の空き缶。
		stage.HasCans = true;
		stage.Cans[0] = { -3.5f * scale, 3.0f, -12.0f * scale };
		stage.Cans[1] = { 3.5f * scale, 3.0f, -12.0f * scale };
	}
	return true;
}

void CanBellCalc::Reset()
{
	for (bool& on : m_OnBridge) on = false;
	m_LastFrame = -1;
}

bool CanBellCalc::Step(int frame, const bool* onBridge, int count)
{
	bool entered = false;
	for (int i = 0; i < count && i < 8; ++i) {
		if (onBridge[i] && !m_OnBridge[i]) entered = true;
		m_OnBridge[i] = onBridge[i];
	}
	if (!entered || (m_LastFrame >= 0 && frame < m_LastFrame + INTERVAL_FRAMES)) return false;
	m_LastFrame = frame;
	return true;
}

float CanBellCalc::GetTilt(int frame) const
{
	const int k = frame - m_LastFrame;
	if (m_LastFrame < 0 || k < 0 || k >= SHAKE_FRAMES) return 0.0f;
	return CAN_TILT_DEGREES * PI / 180.0f * (1.0f - (float)k / SHAKE_FRAMES) * sinf(2.0f * PI * k / CAN_SHAKE_PERIOD);
}
