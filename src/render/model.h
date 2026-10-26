#pragma once
#include "core/dx11.h"

/// @brief モデル管理を初期化する（以前のバッファがあれば解放する）
void ModelInitialize(void);
/// @brief 読み込んだすべてのモデルのバッファを解放する
void ModelFinalize(void);
/// @brief 作業フォルダからの相対パスで OBJ を読み込み、頂点バッファを作る
/// @param path OBJ のパス
/// @param mirrorX X について鏡写しにするかを設定する（true：鏡写しにする（片手用のモデルから反対の手用を作る）、false：そのまま）
/// @return モデル番号。ファイル・OBJ・バッファ作成の失敗時は -1
int ModelLoad(const char* path, bool mirrorX = false);
/// @brief モデルを原点と大きさ込みの直交する3軸で配置し、指定色で描く
/// @param model ModelLoad が返した番号（無効な番号は描かない）
/// @param origin モデルの原点を置くワールド座標
/// @param xAxis モデルの X 軸（長さが倍率）
/// @param yAxis モデルの Y 軸（長さが倍率）
/// @param zAxis モデルの Z 軸（長さが倍率）
/// @param color 色（RGBA）
void ModelDraw(int model, const XMFLOAT3& origin, const XMFLOAT3& xAxis,
	const XMFLOAT3& yAxis, const XMFLOAT3& zAxis, const XMFLOAT4& color);
