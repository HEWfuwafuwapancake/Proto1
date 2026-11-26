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
/// @param pitch 再生の速さ（1.0 で元のまま。小さくすると低く・ゆっくりになる）
/// @param panned 左右を指定するか（true：pan で左右を配る、false：素材のまま鳴らす）
/// @param pan 左右（-1：左、0：真ん中、+1：右。panned が true のときだけ使う）
/// @return 鳴らした音の番号（止めるときに使う。鳴らせなかったら -1）
int SoundPlay(int sound, float volume = 1.0f, float pitch = 1.0f, bool panned = false, float pan = 0.0f);

/// @brief 鳴らしている音の大きさと左右を変える（溜め・回転など、鳴っている間に動く音に使う）
/// @param voice SoundPlay で受け取った番号（鳴り終わっていれば何もしない）
/// @param volume 音量
/// @param pan 左右（-1：左、0：真ん中、+1：右）
void SoundSetMix(int voice, float volume, float pan);

/// @brief 読み込んだ音の余韻を短くする（読み込んだ波形を切るだけで、ファイルは変えない）
/// @param sound SoundLoad で受け取った音の番号
/// @param keepSeconds 残す長さ（秒。元より長ければ何もしない）
/// @param fadeSeconds 残す部分の終わりで、だんだん小さくして消す長さ（秒）
/// @return 短くできたら true（16bit でない・番号が違うときは false）
bool SoundShortenTail(int sound, float keepSeconds, float fadeSeconds);

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
