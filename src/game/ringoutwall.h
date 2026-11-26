#pragma once

#include <d3d11.h>
#include "physics/stagecalc.h"

/// @file ringoutwall.h
/// @brief 横の場外の壁に「DEAD LINE」の帯を描く（判定は physics/stagecalc.h の StageCalcIsSideOut）

/// @brief 横の場外の壁の位置に、半透明の DEAD LINE の帯を一周並べて描く
/// @details 帯は開始位置のいちばん低い床からキャラ1体分上を下の端にし、縦には重ねない。
///          ステージの内側からだけ見える（外側からは消える）。一周の端数の組は切る
class RingOutWall
{
public:
	/// @brief 帯の画像を読み、描くための頂点バッファを作る（ShapeInitialize のあとに呼ぶ）
	void Initialize(void);
	/// @brief 画像と頂点バッファを解放する
	void Finalize(void);
	/// @brief 帯を描く（ステージを描いた直後、体・腕より前に呼ぶ。深度を見ずに重ねるので景観の奥でも見える。横の壁がないステージでは何もしない）
	/// @param stage ステージ
	void Draw(const STAGE& stage) const;

private:
	ID3D11ShaderResourceView* m_Texture = nullptr;	///< DEAD LINE の帯の画像（1組）
	ID3D11Buffer* m_VertexBuffer = nullptr;			///< 帯の板の頂点（描くたびに書き換える）
};
