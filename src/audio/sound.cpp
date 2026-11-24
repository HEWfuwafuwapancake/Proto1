#include "audio/sound.h"

#include <Windows.h>
#include <xaudio2.h>
#include <cstdio>
#include <cstdint>
#include <vector>
#include "audio/wavcalc.h"
#include "core/debug_ostream.h"

#pragma comment (lib, "xaudio2.lib")

namespace
{
	/// 読み込んだ1つの音（ファイルの中身をそのまま持ち、波形はその中を指す）
	struct SOUND_CLIP
	{
		std::vector<uint8_t> File;
		WAVEFORMATEX Format{};
		size_t DataOffset = 0;
		size_t DataSize = 0;
	};

	/// 鳴らしている1つの音
	struct SOUND_VOICE
	{
		IXAudio2SourceVoice* Voice = nullptr;
		uint32_t Generation = 0; // 同じ枠を使い回しても、古い番号で止めないための数
		uint32_t StartOrder = 0; // 枠が足りないときに、いちばん古い音を探すための数
	};

	IXAudio2* s_XAudio = nullptr;
	IXAudio2MasteringVoice* s_Master = nullptr;
	std::vector<SOUND_CLIP> s_Clips;
	SOUND_VOICE s_Voices[SOUND_VOICE_COUNT];
	uint32_t s_PlayCount = 0;

	// 番号 = 世代 × 枠の数 + 枠。古い番号は世代が合わないので何もしない。
	int MakeHandle(int slot) { return static_cast<int>(s_Voices[slot].Generation % 0x10000u) * SOUND_VOICE_COUNT + slot; }
	int FindSlot(int handle)
	{
		if (handle < 0) return -1;
		const int slot = handle % SOUND_VOICE_COUNT;
		const uint32_t generation = static_cast<uint32_t>(handle / SOUND_VOICE_COUNT);
		if (s_Voices[slot].Voice == nullptr || s_Voices[slot].Generation % 0x10000u != generation) return -1;
		return slot;
	}

	void DestroySlot(SOUND_VOICE& slot)
	{
		if (slot.Voice == nullptr) return;
		slot.Voice->DestroyVoice(); // 止めてから消える（再生中でもよい）
		slot.Voice = nullptr;
	}

	bool IsFinished(const SOUND_VOICE& slot)
	{
		if (slot.Voice == nullptr) return true;
		XAUDIO2_VOICE_STATE state{};
		slot.Voice->GetState(&state, XAUDIO2_VOICE_NOSAMPLESPLAYED);
		return state.BuffersQueued == 0;
	}
}

bool SoundInitialize(void)
{
	if (FAILED(XAudio2Create(&s_XAudio, 0, XAUDIO2_DEFAULT_PROCESSOR))) {
		s_XAudio = nullptr;
		hal::dout << "音：XAudio2 を作れませんでした（音なしで続けます）" << std::endl;
		return false;
	}
	if (FAILED(s_XAudio->CreateMasteringVoice(&s_Master))) {
		s_XAudio->Release();
		s_XAudio = nullptr;
		s_Master = nullptr;
		hal::dout << "音：出力先の機器がありません（音なしで続けます）" << std::endl;
		return false;
	}
	return true;
}

void SoundFinalize(void)
{
	for (auto& slot : s_Voices) DestroySlot(slot);
	if (s_Master) { s_Master->DestroyVoice(); s_Master = nullptr; }
	if (s_XAudio) { s_XAudio->Release(); s_XAudio = nullptr; }
	s_Clips.clear();
}

void SoundUpdate(void)
{
	for (auto& slot : s_Voices) {
		if (slot.Voice && IsFinished(slot)) DestroySlot(slot);
	}
}

int SoundLoad(const char* path)
{
	FILE* file = nullptr;
	if (fopen_s(&file, path, "rb") != 0 || file == nullptr) {
		hal::dout << "音：開けません " << path << std::endl;
		return -1;
	}
	SOUND_CLIP clip;
	fseek(file, 0, SEEK_END);
	const long size = ftell(file);
	fseek(file, 0, SEEK_SET);
	if (size > 0) {
		clip.File.resize(static_cast<size_t>(size));
		if (fread(clip.File.data(), 1, clip.File.size(), file) != clip.File.size()) clip.File.clear();
	}
	fclose(file);
	WAV_INFO info;
	if (!WavCalcParse(clip.File.data(), clip.File.size(), info)) {
		hal::dout << "音：PCM の WAV として読めません " << path << std::endl;
		return -1;
	}
	clip.Format.wFormatTag = WAVE_FORMAT_PCM;
	clip.Format.nChannels = info.Channels;
	clip.Format.nSamplesPerSec = info.SampleRate;
	clip.Format.wBitsPerSample = info.BitsPerSample;
	clip.Format.nBlockAlign = info.BlockAlign;
	clip.Format.nAvgBytesPerSec = info.SampleRate * info.BlockAlign;
	clip.DataOffset = info.DataOffset;
	clip.DataSize = info.DataSize - info.DataSize % info.BlockAlign;
	s_Clips.push_back(std::move(clip));
	return static_cast<int>(s_Clips.size()) - 1;
}

int SoundPlay(int sound, float volume)
{
	if (s_XAudio == nullptr || sound < 0 || sound >= static_cast<int>(s_Clips.size())) return -1;
	// 空いている枠（鳴り終わった枠も空きとみなす）、なければいちばん古い音の枠を使う。
	int use = -1;
	for (int i = 0; i < SOUND_VOICE_COUNT; ++i) {
		if (IsFinished(s_Voices[i])) { use = i; break; }
		if (use < 0 || s_Voices[i].StartOrder < s_Voices[use].StartOrder) use = i;
	}
	SOUND_VOICE& slot = s_Voices[use];
	DestroySlot(slot);
	const SOUND_CLIP& clip = s_Clips[sound];
	if (FAILED(s_XAudio->CreateSourceVoice(&slot.Voice, &clip.Format))) {
		slot.Voice = nullptr;
		return -1;
	}
	XAUDIO2_BUFFER buffer{};
	buffer.AudioBytes = static_cast<UINT32>(clip.DataSize);
	buffer.pAudioData = clip.File.data() + clip.DataOffset;
	buffer.Flags = XAUDIO2_END_OF_STREAM;
	slot.Voice->SetVolume(volume);
	if (FAILED(slot.Voice->SubmitSourceBuffer(&buffer)) || FAILED(slot.Voice->Start(0))) {
		DestroySlot(slot);
		return -1;
	}
	++slot.Generation;
	slot.StartOrder = ++s_PlayCount;
	return MakeHandle(use);
}

void SoundStop(int voice)
{
	const int slot = FindSlot(voice);
	if (slot >= 0) DestroySlot(s_Voices[slot]);
}

bool SoundIsPlaying(int voice)
{
	const int slot = FindSlot(voice);
	return slot >= 0 && !IsFinished(s_Voices[slot]);
}

void SoundStopAll(void)
{
	for (auto& slot : s_Voices) DestroySlot(slot);
}

void SoundSetMasterVolume(float volume)
{
	if (s_Master) s_Master->SetVolume(volume);
}
