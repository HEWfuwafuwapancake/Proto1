#pragma once

/// @file sound.h
/// @brief XAudio2 で WAV の音を鳴らす部品（読み込んだ音を、同時にいくつでも重ねて鳴らせる）
/// @details 音声の機器がない PC では初期化に失敗するが、そのときはどの関数も何もしない（ゲームは止めない）。
///          ゲームから鳴らすときは、名前で選べる se.h の SePlay を使う

/// @brief 同時に鳴らせる音の数（超えたら、いちばん古い音を止めて鳴らす）
static constexpr int SOUND_VOICE_COUNT = 48;

/// @brief XAudio2 と出力先（マスターボイス）を作る（CoInitializeEx のあとに呼ぶ）
/// @return 作れたら true（失敗しても、ほかの関数は何もしないだけなので続けてよい）
bool SoundInitialize(void);

/// @brief 鳴っている音を全部止め、読み込んだ音と XAudio2 を解放する
void SoundFinalize(void);

/// @brief 鳴り終わった音の後片づけをする（毎フレーム呼ぶ）
void SoundUpdate(void);

/// @brief WAV ファイル（非圧縮の PCM）を読み込む
/// @param path ファイルのパス（実行するフォルダから。日本語のフォルダ名でもよい）
/// @return 音の番号（読めなかったら -1）
int SoundLoad(const char* path);

/// @brief 読み込んだ音を最初から1回鳴らす
/// @param sound SoundLoad で受け取った音の番号（-1 なら何もしない）
/// @param volume 音量（1.0 で元の大きさ）
/// @return 鳴らした音の番号（止めるときに使う。鳴らせなかったら -1）
int SoundPlay(int sound, float volume = 1.0f);

/// @brief 鳴らしている音を止める（鳴り終わった・-1 なら何もしない）
/// @param voice SoundPlay で受け取った番号
void SoundStop(int voice);

/// @brief 鳴らしている音がまだ鳴っているか
/// @param voice SoundPlay で受け取った番号
/// @return 鳴っていれば true
bool SoundIsPlaying(int voice);

/// @brief 鳴らしている音を全部止める（シーンを切り替えるときなど）
void SoundStopAll(void);

/// @brief 全体の音量を設定する
/// @param volume 音量（1.0 で元の大きさ、0 で無音）
void SoundSetMasterVolume(float volume);
