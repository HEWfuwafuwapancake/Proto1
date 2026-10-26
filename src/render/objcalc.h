#pragma once
#include "physics/vec3.h"
#include <string>
#include <vector>

/// @brief OBJ から読み込んだ左手系の頂点
struct OBJ_VERTEX
{
	VEC3 Position; ///< Z を反転した位置
	VEC3 Normal; ///< Z を反転した単位法線
};

/// @brief OBJ の文字列を左手系の三角形列へ変換する（DirectX 不使用）
/// @param source OBJ の全文。v・vn・f 以外の行は無視する
/// @param vertices 出力先。失敗時は空にする
/// @return 面があり、必要な数値とインデックスが正しければ true
bool ObjCalcParse(const std::string& source, std::vector<OBJ_VERTEX>& vertices);
/// @brief 三角形列を X について鏡写しにする（片手用のモデルから反対の手用を作る。DirectX 不使用）
/// @param vertices ObjCalcParse の結果。位置と法線の X を反転し、表が変わらないよう各三角形の並びを逆にする
void ObjCalcMirrorX(std::vector<OBJ_VERTEX>& vertices);
