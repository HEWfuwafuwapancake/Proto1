#include "audio/sound.h"

#include <Windows.h>
#include <xaudio2.h>
#include <cstdio>
#include <cstdint>
#include <vector>
#include "audio/wavcalc.h"
#include "audio/sespatialcalc.h"
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
		int Channels = 1; // 鳴らしている音のチャンネル数（左右を配るときに使う）
	};

	IXAudio2* s_XAudio = nullptr;
	IXAudio2MasteringVoice* s_Master = nullptr;
	std::vector<SOUND_CLIP> s_Clips;
	SOUND_VOICE s_Voices[SOUND_VOICE_COUNT];
	uint32_t s_PlayCount = 0;
	int s_OutputChannels = 2; // 出力先（スピーカー）のチャンネル数
	float s_MonoDefault = 0.7071f; // 左右を指定しないモノラルの音が、左のスピーカーへ配られる大きさ（XAudio2 の既定）

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

	// 左右を配る。モノラルの音はそのまま、ステレオの音は左右を混ぜてから配る（真ん中では今までと同じ大きさ）。
	// 出力先が1チャンネル・3チャンネル以上の素材は配らない（音量だけ変える）。
	void ApplyPan(const SOUND_VOICE& slot, float pan)
	{
		if (slot.Voice == nullptr || s_Master == nullptr || s_OutputChannels < 2 || slot.Channels > 2) return;
		float left = 0.0f, right = 0.0f;
		SeSpatialCalcPanGains(pan, left, right);
		const int source = slot.Channels;
		std::vector<float> matrix(static_cast<size_t>(source * s_OutputChannels), 0.0f);
		// 行列は「出力のチャンネル × 素材のチャンネル数 + 素材のチャンネル」の並び。
		const float scale = source == 1 ? s_MonoDefault * 1.41421356f : 0.70710678f;
		for (int c = 0; c < source; ++c) {
			matrix[0 * source + c] = left * scale;
			matrix[1 * source + c] = right * scale;
		}
		slot.Voice->SetOutputMatrix(nullptr, source, s_OutputChannels, matrix.data());
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
	XAUDIO2_VOICE_DETAILS details{};
	s_Master->GetVoiceDetails(&details);
	s_OutputChannels = static_cast<int>(details.InputChannels);
	// 左右を指定しないときのモノラルの配り方を調べ、左右を配るときも真ん中では同じ大きさにする。
	WAVEFORMATEX mono{};
	mono.wFormatTag = WAVE_FORMAT_PCM;
	mono.nChannels = 1;
	mono.nSamplesPerSec = 44100;
	mono.wBitsPerSample = 16;
	mono.nBlockAlign = 2;
	mono.nAvgBytesPerSec = 44100 * 2;
	IXAudio2SourceVoice* probe = nullptr;
	if (s_OutputChannels >= 2 && SUCCEEDED(s_XAudio->CreateSourceVoice(&probe, &mono))) {
		std::vector<float> matrix(static_cast<size_t>(s_OutputChannels), 0.0f);
		probe->GetOutputMatrix(nullptr, 1, s_OutputChannels, matrix.data());
		if (matrix[0] > 0.0f) s_MonoDefault = matrix[0];
		probe->DestroyVoice();
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

int SoundPlay(int sound, float volume, float pitch, bool panned, float pan)
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
	slot.Channels = clip.Format.nChannels;
	if (pitch > 0.0f && pitch != 1.0f) slot.Voice->SetFrequencyRatio(pitch);
	if (panned) ApplyPan(slot, pan);
	if (FAILED(slot.Voice->SubmitSourceBuffer(&buffer)) || FAILED(slot.Voice->Start(0))) {
		DestroySlot(slot);
		return -1;
	}
	++slot.Generation;
	slot.StartOrder = ++s_PlayCount;
	return MakeHandle(use);
}

void SoundSetMix(int voice, float volume, float pan)
{
	const int slot = FindSlot(voice);
	if (slot < 0) return;
	s_Voices[slot].Voice->SetVolume(volume);
	ApplyPan(s_Voices[slot], pan);
}

bool SoundShortenTail(int sound, float keepSeconds, float fadeSeconds)
{
	if (sound < 0 || sound >= static_cast<int>(s_Clips.size()) || keepSeconds <= 0.0f) return false;
	SOUND_CLIP& clip = s_Clips[sound];
	if (clip.Format.wBitsPerSample != 16) return false;
	const size_t frames = clip.DataSize / clip.Format.nBlockAlign;
	const size_t keep = static_cast<size_t>(keepSeconds * clip.Format.nSamplesPerSec);
	const size_t fade = static_cast<size_t>(fadeSeconds * clip.Format.nSamplesPerSec);
	int16_t* samples = reinterpret_cast<int16_t*>(clip.File.data() + clip.DataOffset);
	const size_t kept = WavCalcShortenTail(samples, frames, clip.Format.nChannels, keep, fade);
	clip.DataSize = kept * clip.Format.nBlockAlign;
	return true;
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
