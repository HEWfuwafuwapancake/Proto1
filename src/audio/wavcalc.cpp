#include "audio/wavcalc.h"

#include <cstring>

namespace
{
	uint16_t ReadU16(const uint8_t* p) { return static_cast<uint16_t>(p[0] | (p[1] << 8)); }
	uint32_t ReadU32(const uint8_t* p)
	{
		return static_cast<uint32_t>(p[0]) | (static_cast<uint32_t>(p[1]) << 8)
			| (static_cast<uint32_t>(p[2]) << 16) | (static_cast<uint32_t>(p[3]) << 24);
	}
	constexpr uint16_t FORMAT_PCM = 1; // WAVE_FORMAT_PCM
}

bool WavCalcParse(const uint8_t* data, size_t size, WAV_INFO& info)
{
	info = {};
	if (data == nullptr || size < 12) return false;
	if (memcmp(data, "RIFF", 4) != 0 || memcmp(data + 8, "WAVE", 4) != 0) return false;
	bool hasFormat = false;
	bool hasData = false;
	size_t pos = 12;
	while (pos + 8 <= size && !(hasFormat && hasData)) {
		const uint8_t* chunk = data + pos;
		const size_t chunkSize = ReadU32(chunk + 4);
		const size_t body = pos + 8;
		if (memcmp(chunk, "fmt ", 4) == 0) {
			if (chunkSize < 16 || body + 16 > size) return false;
			if (ReadU16(data + body) != FORMAT_PCM) return false;
			info.Channels = ReadU16(data + body + 2);
			info.SampleRate = ReadU32(data + body + 4);
			info.BlockAlign = ReadU16(data + body + 12);
			info.BitsPerSample = ReadU16(data + body + 14);
			hasFormat = true;
		}
		else if (memcmp(chunk, "data", 4) == 0) {
			if (body + chunkSize > size) return false;
			info.DataOffset = body;
			info.DataSize = chunkSize;
			hasData = true;
		}
		// チャンクの大きさが奇数なら、次のチャンクは1バイトの詰め物のあとから始まる。
		pos = body + chunkSize + (chunkSize & 1);
	}
	return hasFormat && hasData && info.Channels > 0 && info.BlockAlign > 0;
}
