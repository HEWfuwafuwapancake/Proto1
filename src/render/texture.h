#pragma once
#include "core/dx11.h"
#include <cstddef>

/// @brief 画像ファイル（PNG・JPEG・BMP など、Windows 標準の WIC で読めるもの）からテクスチャを作る
/// @param path 作業フォルダからの相対パス
/// @return テクスチャ（ミップマップつき）。読めないときは nullptr。使い終わったら Release する
ID3D11ShaderResourceView* TextureLoadFromFile(const char* path);
/// @brief メモリ上の画像ファイルの中身（PNG・JPEG など）からテクスチャを作る
/// @param data 画像ファイルの先頭
/// @param size 画像ファイルのバイト数
/// @return テクスチャ（ミップマップつき）。読めないときは nullptr。使い終わったら Release する
ID3D11ShaderResourceView* TextureLoadFromMemory(const void* data, size_t size);
/// @brief 1画素4バイト（B・G・R・A の順）の画素の並びからテクスチャを作る
/// @param pixels 左上から1行ずつ並んだ画素
/// @param width 幅（画素）
/// @param height 高さ（画素）
/// @return テクスチャ（ミップマップつき）。作れないときは nullptr。使い終わったら Release する
ID3D11ShaderResourceView* TextureCreateBGRA(const void* pixels, UINT width, UINT height);
