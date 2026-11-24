#pragma once

#include "physics/vec3.h"

/// @file stagecalc.h
/// @brief ステージ（浮島と高台）と場外の計算
/// @details DirectX と Windows を使わないので、単体テスト（tests/Proto1Tests）からそのまま使える。
///          場外の線は下だけ（横や上に飛ばされても、落ちて下の線を越えたらアウト）

/// @brief ステージに置ける高台の最大数
static constexpr int STAGE_PLATFORM_MAX = 8;

/// @brief 下から通り抜けられる四角い高台
struct STAGE_PLATFORM
{
	float CenterX;		///< 高台の中心の X
	float CenterZ;		///< 高台の中心の Z
	float HalfWidth;	///< 高台の横幅（X）の半分
	float HalfDepth;	///< 高台の奥行き（Z）の半分
	float TopY;			///< 高台の上の面の高さ（ここに立つ）
	float Thickness;	///< 高台の描画上の厚さ
};

/// @brief 地形の区画の最大数
static constexpr int STAGE_REGION_MAX = 16;
/// @brief 開始・復活の候補（P1〜P4）の数
static constexpr int STAGE_SPAWN_MAX = 4;
/// @brief 腕カプセルの決まった出現地点の最大数
static constexpr int STAGE_SUPPLY_MAX = 16;
/// @brief Ｇ（補助触腕で高所へ上がる場所）の最大数
static constexpr int STAGE_LIFT_MAX = 4;
/// @brief 荷送りベルトの最大数
static constexpr int STAGE_BELT_MAX = 2;
/// @brief 地形を調べるときの体の半径（設計書の「キャラ半径0.5」）
static constexpr float STAGE_BODY_RADIUS = 0.5f;
/// @brief 地形を調べるときの体の高さ
static constexpr float STAGE_BODY_HEIGHT = 1.8f;

/// @brief StageCalcMoveBody の戻り値で、X の移動が壁で止まったことを表すビット
static constexpr int STAGE_STOP_X = 1;
/// @brief StageCalcMoveBody の戻り値で、Z の移動が壁で止まったことを表すビット
static constexpr int STAGE_STOP_Z = 2;

/// @brief 地形の区画の見た目の種類（描く色と、効果を付ける区画の目印）
enum STAGE_SURFACE
{
	STAGE_SURFACE_FLOOR,		///< 基準の床（郵便局の屋上・市場の路面）
	STAGE_SURFACE_CENTER,		///< 郵便局の仕分け台
	STAGE_SURFACE_ROOF,			///< 郵便局の集配棟の屋根
	STAGE_SURFACE_RAMP,			///< 坂
	STAGE_SURFACE_LOW_ROAD,		///< 市場の低い搬入橋と、掘り下げた荷道
	STAGE_SURFACE_TRAM,			///< 市場の北の高い橋（市電の屋根）
	STAGE_SURFACE_SHUTTER,		///< 市場の南の高い橋（シャッター。空き缶が鳴る）
	STAGE_SURFACE_BASE,			///< 市場の建物の基礎
};

/// @brief 地形の1区画（範囲の中なら上面の高さ H = BaseY + SlopeX × x + SlopeZ × z）
/// @details 区画は上から順に調べ、最初に入った区画の高さを使う。範囲は両端を含む
struct STAGE_REGION
{
	float MinX;				///< 範囲の X の最小
	float MaxX;				///< 範囲の X の最大
	float MinZ;				///< 範囲の Z の最小
	float MaxZ;				///< 範囲の Z の最大
	float DiagonalLimit;	///< |x| ＋ |z| がこれ以下の所だけを含む（四隅を切り落とす。0 以下なら使わない）
	float BaseY;			///< 上面の高さの x = 0・z = 0 での値
	float SlopeX;			///< 上面の高さが x 1 あたりに増える量（坂でなければ 0）
	float SlopeZ;			///< 上面の高さが z 1 あたりに増える量（坂でなければ 0）
	bool Solid;				///< 側面があるかを設定する（true：地盤の下面から上面までが壁、false：上面だけの片面の橋で、下から通り抜けられる）
	STAGE_SURFACE Surface;	///< 描く色と、効果を付ける区画の目印
};

/// @brief Ｇ（地上の始点の近くで押すと、補助触腕で高所の名目着地点へ移る場所）
struct STAGE_LIFT
{
	float StartX;	///< 地上の始点の X
	float StartZ;	///< 地上の始点の Z
	VEC3 Landing;	///< 名目着地点（足元。Y は高所の上面）
};

/// @brief 荷送りベルト（接地して歩いている人を Z の中央向きに運ぶ範囲）
struct STAGE_BELT
{
	float MinX;	///< 範囲の X の最小
	float MaxX;	///< 範囲の X の最大
	float MinZ;	///< 範囲の Z の最小
	float MaxZ;	///< 範囲の Z の最大
};

/// @brief 浮島、高台、場外の線（対戦マップでは地形の区画・開始位置・Ｇ・補給・ベルトも持つ）
struct STAGE
{
	float CenterX;		///< 浮島の中心の X（地形のマップでは全体の中心）
	float CenterZ;		///< 浮島の中心の Z（地形のマップでは全体の中心）
	float HalfWidth;	///< 浮島の横幅（X）の半分（地形のマップでは全体を囲む半分の幅）
	float HalfDepth;	///< 浮島の奥行き（Z）の半分（地形のマップでは全体を囲む半分の奥行き）
	float TopY;			///< 浮島の上の面の高さ（ここに立つ。地形のマップではいちばん低い床の高さ）
	float Thickness;	///< 浮島の厚さ（見た目だけ。上の面から下へこの分だけある）
	float OutY;			///< 場外の下の線の高さ（これより下に落ちたら場外）
	STAGE_PLATFORM Platforms[STAGE_PLATFORM_MAX]{};	///< 高台の並び
	int PlatformCount = 0;	///< 高台の個数
	bool HasTerrain = false;	///< 地形の区画を使うかを設定する（true：浮島と高台の代わりに Regions で判定する、false：浮島と高台）
	STAGE_REGION Regions[STAGE_REGION_MAX]{};	///< 地形の区画（上から順に調べる）
	int RegionCount = 0;	///< 地形の区画の数
	float GroundBottomY = -6.0f;	///< 地盤の下面の高さ（側面の壁はここから上面まで）
	VEC3 Spawns[STAGE_SPAWN_MAX]{};	///< 開始・復活の候補（P1〜P4 の足元）
	int SpawnCount = 0;	///< 開始・復活の候補の数（0 なら浮島の決まった配置を使う）
	VEC3 Supplies[STAGE_SUPPLY_MAX]{};	///< 腕カプセルの決まった出現地点（抽選の順）
	int SupplyCount = 0;	///< 決まった出現地点の数（0 なら浮島・高台の上に乱数で置く）
	STAGE_LIFT Lifts[STAGE_LIFT_MAX]{};	///< Ｇの並び（P1〜P4 と同じ象限の順）
	int LiftCount = 0;	///< Ｇの数
	float LiftSideSign = 1.0f;	///< Ｇの着地点が塞がれたとき横へずらす向き（名目着地点の X の符号に掛ける）
	STAGE_BELT Belts[STAGE_BELT_MAX]{};	///< 荷送りベルト
	int BeltCount = 0;	///< 荷送りベルトの数
	float BeltSpeed = 0.0f;	///< ベルトが運ぶ速さ（1秒に進む距離）
	VEC3 Cans[2]{};	///< 南の橋の両端の空き缶の位置（足元）
	bool HasCans = false;	///< 南の橋の空き缶があるかを設定する（true：STAGE_SURFACE_SHUTTER に乗ると鳴る、false：なし）
};

/// @brief 地形の区画の、その場所の上面の高さを求める（範囲は見ない）
/// @param region 区画
/// @param x 調べる場所の X
/// @param z 調べる場所の Z
/// @return 上面の高さ
float StageCalcRegionHeight(const STAGE_REGION& region, float x, float z);

/// @brief その場所の床になる地形の区画を探す（上から順に調べ、最初に入った区画）
/// @param stage ステージ（地形のないステージでは見つからない）
/// @param x 調べる場所の X
/// @param z 調べる場所の Z
/// @return 区画の番号（床がなければ -1）
int StageCalcFindRegion(const STAGE& stage, float x, float z);

/// @brief その場所の床の上面の高さを求める（地形のマップは区画、それ以外は浮島と高台のいちばん高い面）
/// @param stage ステージ
/// @param x 調べる場所の X
/// @param z 調べる場所の Z
/// @param height 床があったときに上面の高さを書き込む先
/// @return 床があれば true
bool StageCalcGroundHeight(const STAGE& stage, float x, float z, float& height);

/// @brief 地面に立っている体が水平に動いたあと、足元の床へ合わせる（坂を上り下りする）
/// @details 地形のないステージは、今の高さに足場があるかだけを調べる（高さは変えない）。
///          地形のマップは、足元の床が今の高さから段差の上限の中にあれば、その高さに合わせる
/// @param stage ステージ
/// @param x 移動後の足元の X
/// @param y 移動前の足元の高さ
/// @param z 移動後の足元の Z
/// @param groundY 立ち続けられるときに足元の高さを書き込む先
/// @return 立ち続けられれば true（床なし・大きな下り段差なら false で、そこから落ちる）
bool StageCalcFollowGround(const STAGE& stage, float x, float y, float z, float& groundY);

/// @brief 体を水平に動かし、地形の側面（足が上縁より低い段差）で止めて、残りを壁に沿わせる
/// @details 地形のないステージはそのまま動かす。今いる場所がすでに壁の中なら、抜け出せるようにそのまま動かす
/// @param stage ステージ
/// @param x 移動前の足元の X
/// @param y 足元の高さ
/// @param z 移動前の足元の Z
/// @param dx X の移動量
/// @param dz Z の移動量
/// @param onGround 立っているかを設定する（true：坂を上れる段差まで越える、false：空中で、少しの段差だけ越える）
/// @param outX 移動後の X を書き込む先
/// @param outZ 移動後の Z を書き込む先
/// @return 壁で止まった軸（STAGE_STOP_X・STAGE_STOP_Z の組み合わせ。止まらなければ 0）
int StageCalcMoveBody(const STAGE& stage, float x, float y, float z, float dx, float dz, bool onGround,
	float& outX, float& outZ);

/// @brief その場所の真下（または真上）に浮島があるか調べる
/// @details 高さは見ない。あれば上の面（TopY）に着地できる。端ちょうどは「ある」。地形のマップでは、その場所に床の区画があるか
/// @param stage ステージ
/// @param x 調べる場所の X
/// @param z 調べる場所の Z
/// @return 浮島の範囲の中なら true
bool StageCalcIsOnPlatform(const STAGE& stage, float x, float z);

/// @brief 指定の高さに浮島または高台の上面があるか調べる
/// @param stage ステージ
/// @param x 足元の X
/// @param y 足元の高さ
/// @param z 足元の Z
/// @return 足元に上面があれば true
bool StageCalcHasSupport(const STAGE& stage, float x, float y, float z);

/// @brief 上から通過した浮島と高台の上面のうち、最も高い上面を探す
/// @param stage ステージ
/// @param x 移動後の X
/// @param z 移動後の Z
/// @param previousY 移動前の高さ
/// @param y 移動後の高さ
/// @param landingY 上面が見つかったときに書き込む高さ
/// @return 上面が見つかったら true（同じ高さでの接触を含む）
bool StageCalcFindLanding(const STAGE& stage, float x, float z, float previousY, float y, float& landingY);

/// @brief 触手がつかむ足場のふち
struct STAGE_GRIP
{
	VEC3 Point;				///< ふちの点（上の面の高さ）
	STAGE_PLATFORM Surface;	///< そのふちを持つ足場の形（浮島なら浮島の形）
};

/// @brief 浮島の形を高台と同じ形にして取得する
/// @param stage ステージ
/// @return 浮島の中心・半幅・半奥行き・上の面の高さ・厚さ
STAGE_PLATFORM StageCalcIslandSurface(const STAGE& stage);

/// @brief 正面の足場のふちを探す（水平か見下ろしなら水平の扇でいちばん近いふち、見上げていれば向きにいちばん近いふち）
/// @details 立っている高台は選ばない。浮島は、浮島の上に立っていないとき（場外・浮島より下）だけ選ぶ
/// @param stage ステージ
/// @param origin 距離と向きを測る点（肩）
/// @param feet 足元の位置（立っている足場を除くのに使う）
/// @param forward 探す中心の向き（Y が正なら上下も含めて比べる。0 以下なら上下は使わない）
/// @param maxDistance 上下も含めて、これより遠いふちは選ばない
/// @param halfAngle 中心の向きからの開き（ラジアン）
/// @param grip 見つかったときに書き込むふち
/// @return 見つかったら true
bool StageCalcFindGrip(const STAGE& stage, const VEC3& origin, const VEC3& feet, const VEC3& forward,
	float maxDistance, float halfAngle, STAGE_GRIP& grip);

/// @brief 2点を結ぶ線が、浮島か高台の上の面を上下に通り抜けるか調べる（爆発が足場の向こうへ届かないようにする）
/// @param stage ステージ
/// @param from 始まりの点（爆心など）
/// @param to 終わりの点（相手の体の中心など）
/// @return 上の面の高さをまたぎ、またぐ所がその足場の範囲の中なら true（ちょうど面の高さの点はまたがない）
bool StageCalcIsBlocked(const STAGE& stage, const VEC3& from, const VEC3& to);

/// @brief 場外の下の線より下に落ちたか調べる
/// @param stage ステージ
/// @param y 調べる高さ
/// @return 下の線（OutY）より下なら true（ちょうどの高さはまだ場外でない）
bool StageCalcIsOut(const STAGE& stage, float y);
