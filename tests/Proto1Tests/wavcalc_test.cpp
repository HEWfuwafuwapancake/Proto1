#include <gtest/gtest.h>
#include <cstring>
#include <vector>
#include "audio/wavcalc.h"

namespace
{
	void PutU16(std::vector<uint8_t>& out, uint16_t v) { out.push_back(v & 0xFF); out.push_back(v >> 8); }
	void PutU32(std::vector<uint8_t>& out, uint32_t v) { for (int i = 0; i < 4; ++i) out.push_back((v >> (i * 8)) & 0xFF); }
	void PutTag(std::vector<uint8_t>& out, const char* tag) { out.insert(out.end(), tag, tag + 4); }

	// 16bit ステレオ 44100Hz の WAV を作る（fmt と data の間に、飛ばすべき LIST チャンク（奇数の大きさ）を入れる）。
	std::vector<uint8_t> MakeWav(uint16_t format, uint32_t dataSize)
	{
		std::vector<uint8_t> out;
		PutTag(out, "RIFF");
		PutU32(out, 0); // 大きさは読まないので 0 のまま
		PutTag(out, "WAVE");
		PutTag(out, "fmt ");
		PutU32(out, 16);
		PutU16(out, format);
		PutU16(out, 2);
		PutU32(out, 44100);
		PutU32(out, 44100 * 4);
		PutU16(out, 4);
		PutU16(out, 16);
		PutTag(out, "LIST");
		PutU32(out, 3);
		out.insert(out.end(), { 'a', 'b', 'c', 0 }); // 3バイト＋詰め物1バイト
		PutTag(out, "data");
		PutU32(out, dataSize);
		out.insert(out.end(), dataSize, 0x11);
		return out;
	}
}

// PCM の WAV から形式と波形の場所を読み、間のほかのチャンクは飛ばす
TEST(WavCalcTest, ReadsPcmFormatAndSkipsOtherChunks)
{
	const std::vector<uint8_t> wav = MakeWav(1, 8);
	WAV_INFO info;
	ASSERT_TRUE(WavCalcParse(wav.data(), wav.size(), info));
	EXPECT_EQ(info.Channels, 2);
	EXPECT_EQ(info.SampleRate, 44100u);
	EXPECT_EQ(info.BitsPerSample, 16);
	EXPECT_EQ(info.BlockAlign, 4);
	EXPECT_EQ(info.DataSize, 8u);
	EXPECT_EQ(info.DataOffset, wav.size() - 8);
}

// 圧縮された形式（PCM 以外）は読まない（mp3 などを WAV に包んだものは鳴らせないので）
TEST(WavCalcTest, RejectsNonPcm)
{
	const std::vector<uint8_t> wav = MakeWav(0x55, 8);
	WAV_INFO info;
	EXPECT_FALSE(WavCalcParse(wav.data(), wav.size(), info));
}

// 余韻を短くすると、残す長さで切れて、終わりに向かって小さくなり最後は 0 になる
TEST(WavCalcTest, ShortenTailFadesToSilence)
{
	std::vector<int16_t> samples(20, 1000); // ステレオ 10 フレーム
	EXPECT_EQ(WavCalcShortenTail(samples.data(), 10, 2, 6, 4), 6u);
	EXPECT_EQ(samples[0], 1000); // フェードより前はそのまま
	EXPECT_EQ(samples[3], 1000);
	EXPECT_LT(samples[2 * 2], 1000); // フェードの途中は小さい
	EXPECT_GT(samples[2 * 2], 0);
	EXPECT_EQ(samples[5 * 2], 0); // 最後のフレームは無音
	EXPECT_EQ(samples[5 * 2 + 1], 0);
	EXPECT_EQ(WavCalcShortenTail(samples.data(), 10, 2, 12, 4), 10u); // 元より長くは伸ばさない
}
