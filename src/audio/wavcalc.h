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

/// @brief 16bit の波形の余韻を短くする（残す長さの終わりの部分を少しずつ小さくして消す）
/// @details 素材の余韻が長すぎるときに、読み込んだ波形をこの関数で切る（ファイルは変えない）
/// @param samples 波形（チャンネルが交互に並んだ 16bit のサンプル。書き換える）
/// @param frames 波形のフレーム数（1フレーム = 全チャンネル分の1サンプル）
/// @param channels チャンネル数
/// @param keepFrames 残すフレーム数（frames 以上なら何もしない）
/// @param fadeFrames 残す部分の終わりで、0 まで小さくしていくフレーム数（keepFrames より長ければ keepFrames にそろえる）
/// @return 残したフレーム数
size_t WavCalcShortenTail(int16_t* samples, size_t frames, int channels, size_t keepFrames, size_t fadeFrames);
