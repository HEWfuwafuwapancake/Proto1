#pragma once

/// @file wavcalc.h
/// @brief WAV ファイルの中身から、音の形式と波形の場所を読む計算（Windows・DirectX 不使用）

#include <cstddef>
#include <cstdint>

/// @brief WAV ファイルから読み取った音の形式と波形の場所
struct WAV_INFO
{
	uint16_t Channels = 0; ///< チャンネル数（1：モノラル、2：ステレオ）
	uint32_t SampleRate = 0; ///< 1秒あたりのサンプル数（Hz）
	uint16_t BitsPerSample = 0; ///< 1サンプルのビット数（16 など）
	uint16_t BlockAlign = 0; ///< 全チャンネル分の1サンプルのバイト数
	size_t DataOffset = 0; ///< ファイルの先頭から波形（data チャンク）の中身までのバイト数
	size_t DataSize = 0; ///< 波形のバイト数
};

/// @brief WAV ファイルの中身を読み、PCM の形式と波形の場所を求める
/// @details RIFF のチャンクを順にたどり、fmt と data を探す（LIST などほかのチャンクは飛ばす）。
///          対応するのは非圧縮の PCM だけ（assets/sounds/se の音は全部 16bit PCM に変換してある）
/// @param data ファイルの中身
/// @param size ファイルのバイト数
/// @param info 読み取った形式を書く先
/// @return 読めたら true（RIFF/WAVE でない・PCM でない・fmt か data がない・波形がファイルからはみ出すときは false）
bool WavCalcParse(const uint8_t* data, size_t size, WAV_INFO& info);
