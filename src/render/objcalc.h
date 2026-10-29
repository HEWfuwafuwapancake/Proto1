#pragma once
#include "physics/vec3.h"
#include <string>
#include <vector>

/// @brief OBJ から読み込んだ左手系の頂点
struct OBJ_VERTEX
{
	VEC3 Position; ///< Z を反転した位置
	VEC3 Normal; ///< Z を反転した単位法線
	int Material = -1; ///< 材質の一覧での番号（usemtl がない・一覧にないときは -1）
};

/// @brief MTL から読み込んだ材質
struct OBJ_MATERIAL
{
	std::string Name; ///< newmtl の名前
	VEC3 Diffuse; ///< Kd の色（RGB を X・Y・Z に入れる。Kd がなければ白）
};

/// @brief MTL の文字列から材質の一覧を読む（DirectX 不使用）
/// @param source MTL の全文。newmtl・Kd 以外の行は無視する
/// @param materials 出力先。失敗時は空にする
/// @return 材質が1つ以上あり、Kd の数値が正しければ true
bool ObjCalcParseMtl(const std::string& source, std::vector<OBJ_MATERIAL>& materials);
/// @brief OBJ の文字列から mtllib のファイル名を取り出す（DirectX 不使用）
/// @param source OBJ の全文
/// @return 最初の mtllib のファイル名。なければ空
std::string ObjCalcGetMtlLib(const std::string& source);
/// @brief OBJ の文字列を左手系の三角形列へ変換する（材質なし。DirectX 不使用）
/// @param source OBJ の全文。v・vn・f 以外の行は無視する
/// @param vertices 出力先。失敗時は空にする。頂点の材質の番号はすべて -1
/// @return 面があり、必要な数値とインデックスが正しければ true
bool ObjCalcParse(const std::string& source, std::vector<OBJ_VERTEX>& vertices);
/// @brief OBJ の文字列を、材質の番号つきの左手系の三角形列へ変換する（DirectX 不使用）
/// @param source OBJ の全文。v・vn・f・usemtl 以外の行は無視する
/// @param materials ObjCalcParseMtl で読んだ材質の一覧。usemtl の名前をこの一覧の番号に変える
/// @param vertices 出力先。失敗時は空にする
/// @return 面があり、必要な数値とインデックスが正しければ true
bool ObjCalcParse(const std::string& source, const std::vector<OBJ_MATERIAL>& materials,
	std::vector<OBJ_VERTEX>& vertices);
/// @brief 三角形列を X について鏡写しにする（片手用のモデルから反対の手用を作る。DirectX 不使用）
/// @param vertices ObjCalcParse の結果。位置と法線の X を反転し、表が変わらないよう各三角形の並びを逆にする（材質の番号は変えない）
void ObjCalcMirrorX(std::vector<OBJ_VERTEX>& vertices);
