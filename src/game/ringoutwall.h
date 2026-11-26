#pragma once

#include <d3d11.h>
#include "physics/stagecalc.h"

/// @file ringoutwall.h
/// @brief 横の場外の壁に「DEAD LINE」の帯を描く（判定は physics/stagecalc.h の StageCalcIsSideOut）

/// @brief 帯の見た目の種類
enum RING_OUT_WALL_LOOK
{
	RING_OUT_WALL_LOOK_NORMAL,		///< ふだんの帯（緑の DEAD LINE）
	RING_OUT_WALL_LOOK_RING_OUT,	///< 場外になった所の帯（赤の RING OUT!。復活するまで）
	RING_OUT_WALL_LOOK_COUNT,		///< 見た目の数
};

/// @brief RING OUT! にできる場所の最大数（体の数）
static constexpr int RING_OUT_WALL_MARK_MAX = 4;

/// @brief 横の場外の壁の位置に、半透明の DEAD LINE の帯を一周並べて描く
/// @details 帯は開始位置のいちばん低い床からキャラ1体分上を下の端にし、縦には重ねない。
///          ステージの内側からだけ見える（外側からは消える）。
///          組の数は一周がちょうど埋まるように切り上げ、1組の幅を画像の左右の端を切って縮める（文字は拡大・縮小しない）。
///          場外になった人がいれば、壁のいちばん近い所の1組を、復活するまで赤の RING OUT! にする
class RingOutWall
{
public:
	/// @brief 帯の画像を読み、描くための頂点バッファを作る（ShapeInitialize のあとに呼ぶ）
	void Initialize(void);
	/// @brief 画像と頂点バッファを解放する
	void Finalize(void);
	/// @brief 帯を描く（ステージを描いた直後、体・腕より前に呼ぶ。深度を見るので地形・柱の奥なら隠れる。横の壁がないステージでは何もしない）
	/// @details 景観のモデルで描くマップでは、呼ぶ前に景観の深度を消して地形の区画の深度だけを書いておく（Stage::DrawTerrainDepth）
	/// @param stage ステージ
	/// @param outPositions 復活を待っている人の、場外になった場所（足元）の並び
	/// @param outCount outPositions の数（RING_OUT_WALL_MARK_MAX まで使う）
	void Draw(const STAGE& stage, const VEC3* outPositions, int outCount) const;

private:
	ID3D11ShaderResourceView* m_Textures[RING_OUT_WALL_LOOK_COUNT] = {};	///< 見た目ごとの帯1組の画像
	ID3D11Buffer* m_VertexBuffer = nullptr;			///< 帯の板の頂点（描くたびに書き換える）
};
