#include "audio/se.h"

#include <random>
#include "audio/sound.h"

namespace
{
	/// 1つの効果音の候補のファイルと音量
	struct SE_ENTRY
	{
		SE_ID Id;
		float Volume;
		const char* Files[4]; // 候補（複数あれば鳴らすたびにランダム。空きは nullptr）
		float Pitch = 1.0f; // 再生の速さ（1.0 で元のまま。小さくすると低く・ゆっくり＝重く聞こえる）
		float TailSeconds = 0.0f; // 0 より大きければ、この長さで余韻を切る（ファイルは変えない）
	};
	static constexpr float TAIL_FADE_SECONDS = 0.25f; // 余韻を切るときに、だんだん小さくする長さ

	// 調整用の値
	// フォルダ名は長いので、まとめておく。
#define SE_DIR "assets/sounds/se/"
#define SE_MENU SE_DIR "選択画面/選択・カーソル移動/"
#define SE_CHARA SE_DIR "選択画面/キャラクター選択/"
#define SE_GIRL SE_DIR "戦闘中/メカガール/"
#define SE_ARM SE_DIR "戦闘中/腕/"
	// 「(1)」「(2)」のないものが2つある音（刀のヒット・刀の移動・レーザーの溜め・ミサイルのロックオン・爆発）は、
	// サウンド担当と話し合って決めるまで、1つ目（仮）を使う。入れ替えるときはここのファイル名を変える。
	// 触手を戻したときの音は、サウンド担当の README で「いらない」となっているので鳴らさない。
	// No.195 の試聴：呼び出しで音を入れ替えたもの（触手の伸ばす＝振り回したとき、振り回し＝振り回したときのヒット音、
	// ナックルの空振り＝刀空振り、槍の刺さり＝刀単押しヒット2）は、各腕の .cpp で選んでいる。
	// 使わなくなった音（メカガールの攻撃・被弾の声、ミサイルのロックオン、触手の伸ばしたとき）も、
	// ほかで使うかもしれないので表には残す。
	const SE_ENTRY SE_TABLE[] = {
		{ SE_MENU_CURSOR, 0.6f, { SE_MENU "カーソル移動7.wav" } },
		{ SE_MENU_DECIDE, 0.8f, { SE_MENU "決定.wav" } },
		{ SE_MENU_BACK, 0.8f, { SE_MENU "戻る.wav" } },
		{ SE_CHARACTER_SELECT, 1.0f, { SE_CHARA "キャラクターを選んでね.wav" } },
		{ SE_MEKAGIRL_DECIDE, 1.0f, { SE_CHARA "メカガール決定時.wav" } },

		{ SE_MEKAGIRL_ATTACK, 0.7f, { SE_GIRL "攻撃時 (1).wav", SE_GIRL "攻撃時 (2).wav", SE_GIRL "攻撃時 (3).wav", SE_GIRL "攻撃時 (4).wav" } },
		{ SE_MEKAGIRL_DAMAGE, 0.7f, { SE_GIRL "被弾 (1).wav", SE_GIRL "被弾 (2).wav", SE_GIRL "被弾 (3).wav" } },
		{ SE_MEKAGIRL_FALL, 0.9f, { SE_GIRL "落下時.wav" } },

		{ SE_ARM_BREAK, 1.0f, { SE_ARM "00_共通/腕の破壊音.wav" } },
		{ SE_ARM_HOLD, 0.6f, { SE_ARM "00_共通/長押し中(ループじゃない).wav" } },
		{ SE_ARM_GUARD, 0.9f, { SE_ARM "00_共通/防御.wav" }, 1.0f, 0.6f }, // 余韻を少し短く

		{ SE_TENTACLE_EXTEND, 0.7f, { SE_ARM "01_触手/伸ばしたとき.wav" } },
		{ SE_TENTACLE_SWING, 0.8f, { SE_ARM "01_触手/振り回したとき.wav" } },
		{ SE_TENTACLE_SWING_HIT, 0.9f, { SE_ARM "01_触手/振り回したときのヒット音.wav" } },
		{ SE_TENTACLE_SLAM, 1.0f, { SE_ARM "01_触手/敵を叩きつけたとき.wav" } },

		{ SE_HAMMER_TAP, 0.75f, { SE_ARM "02_ハンマー/ハンマー単押し.wav" } },
		{ SE_HAMMER_HOLD, 1.25f, { SE_ARM "02_ハンマー/ハンマー長押し.wav" } },

		{ SE_KNUCKLE_TAP, 0.7f, { SE_ARM "03_ナックル/ナックル単押し (1).wav", SE_ARM "03_ナックル/ナックル単押し (2).wav", SE_ARM "03_ナックル/ナックル単押し (3).wav" } },
		{ SE_KNUCKLE_MISS, 0.7f, { SE_ARM "03_ナックル/ナックル空振り.wav" } },
		{ SE_KNUCKLE_HOLD, 1.25f, { SE_ARM "03_ナックル/ナックル長押し.wav" } },

		{ SE_KATANA_HIT, 0.9f, { SE_ARM "04_刀/刀単押しヒット.wav" } }, // 単押しで当てたときは arm_katana.cpp で少し下げる
		{ SE_KATANA_HIT2, 0.9f, { SE_ARM "04_刀/刀単押しヒット2.wav" } },
		{ SE_KATANA_MISS, 0.7f, { SE_ARM "04_刀/刀空振り.wav" } },
		{ SE_KATANA_DASH, 0.9f, { SE_ARM "04_刀/刀長押し移動1.wav" } }, // 刀長押し移動2.wav は使わない（No.195）

		{ SE_LASER_SHOT, 0.8f, { SE_ARM "05_レーザー/レーザー単押し.wav" } },
		{ SE_LASER_CHARGE, 0.7f, { SE_ARM "05_レーザー/レーザー長押し溜め.wav" } }, // 溜め2は冒頭が発射に似るので保留
		{ SE_LASER_CANNON, 1.5f, { SE_ARM "05_レーザー/レーザー長押し発射.wav" } },

		{ SE_SPEAR_THROW, 0.8f, { SE_ARM "06_槍/槍投げ.wav" } },
		{ SE_SPEAR_STAB, 1.0f, { SE_ARM "06_槍/槍が刺さったとき.wav" } },
		{ SE_SPEAR_PULL, 0.8f, { SE_ARM "06_槍/槍ヒット時移動.wav" } },

		{ SE_FLAIL_SPIN, 0.7f, { SE_ARM "07_モーニングスター/モーニングスター振り回しているとき.wav" }, 0.85f }, // 低く・ゆっくりで重く
		{ SE_FLAIL_HIT, 1.0f, { SE_ARM "07_モーニングスター/モーニングスター攻撃時.wav" } },
		{ SE_FLAIL_CHAIN, 0.65f, { SE_ARM "07_モーニングスター/モーニングスター鎖.wav" }, 1.0f, 0.9f }, // 少し下げて余韻を短く

		{ SE_MISSILE_LOCK, 0.7f, { SE_ARM "08_ミサイル/ロックオン2.wav" } }, // No.195 でロックオンは鳴らさないことにした
		{ SE_MISSILE_FIRE, 0.8f, { SE_ARM "08_ミサイル/発射.wav" } },
		{ SE_MISSILE_EXPLODE, 1.0f, { SE_ARM "08_ミサイル/爆発1.wav" } }, // 候補：爆発2.wav（評価は良い。今は1だけ）
	};
#undef SE_DIR
#undef SE_MENU
#undef SE_CHARA
#undef SE_GIRL
#undef SE_ARM

	static_assert(sizeof(SE_TABLE) / sizeof(SE_TABLE[0]) == SE_COUNT, "SE_TABLE に全部の SE_ID を1行ずつ書く");

	/// 読み込んだ効果音（候補ごとの音の番号と、表の音量）
	struct SE_LOADED
	{
		int Sounds[4] = { -1, -1, -1, -1 };
		int Count = 0;
		float Volume = 1.0f;
		float Pitch = 1.0f;
	};

	SE_LOADED s_Loaded[SE_COUNT];
	// 候補を選ぶ乱数。ゲームの乱数（オンラインで全員そろえる）とは別にする。
	std::minstd_rand s_Random(12345u);
	SE_LISTENER s_Listeners[SE_LISTENER_MAX];
	int s_ListenerCount = 0;

	// 候補を1つ選んで、決めた大きさ・左右で鳴らす。
	int PlayLoaded(SE_ID id, float volume, bool panned, float pan)
	{
		if (id < 0 || id >= SE_COUNT) return -1;
		const SE_LOADED& loaded = s_Loaded[id];
		if (loaded.Count == 0) return -1;
		const int pick = loaded.Count == 1 ? 0 : static_cast<int>(s_Random() % static_cast<unsigned>(loaded.Count));
		return SoundPlay(loaded.Sounds[pick], volume, loaded.Pitch, panned, pan);
	}
}

void SeInitialize(void)
{
	for (const SE_ENTRY& entry : SE_TABLE) {
		SE_LOADED& loaded = s_Loaded[entry.Id];
		loaded = {};
		loaded.Volume = entry.Volume;
		loaded.Pitch = entry.Pitch;
		for (const char* file : entry.Files) {
			if (file == nullptr) continue;
			const int sound = SoundLoad(file);
			if (sound < 0) continue;
			if (entry.TailSeconds > 0.0f) SoundShortenTail(sound, entry.TailSeconds, TAIL_FADE_SECONDS);
			loaded.Sounds[loaded.Count++] = sound;
		}
	}
}

int SePlay(SE_ID id, float volume)
{
	if (id < 0 || id >= SE_COUNT) return -1;
	return PlayLoaded(id, s_Loaded[id].Volume * volume, false, 0.0f);
}

void SeSetListeners(const SE_LISTENER* listeners, int count)
{
	s_ListenerCount = 0;
	for (int i = 0; listeners != nullptr && i < count && i < SE_LISTENER_MAX; ++i) s_Listeners[s_ListenerCount++] = listeners[i];
}

bool SeIsListener(int slot)
{
	for (int i = 0; i < s_ListenerCount; ++i) {
		if (s_Listeners[i].Slot == slot) return true;
	}
	return false;
}

int SePlayAt(SE_ID id, const VEC3& position, int slotA, int slotB, float volume)
{
	if (id < 0 || id >= SE_COUNT) return -1;
	// 聞き手がいない（試合の外）ときは、場所を使わずに鳴らす。
	if (s_ListenerCount == 0) return SePlay(id, volume);
	const SE_MIX mix = SeSpatialCalcMix(s_Listeners, s_ListenerCount, position, slotA, slotB);
	return PlayLoaded(id, s_Loaded[id].Volume * volume * mix.Volume, true, mix.Pan);
}

void SeVoice::Play(SE_ID id, float volume)
{
	Stop();
	m_Spatial = false;
	m_Voice = SePlay(id, volume);
}

void SeVoice::Keep(SE_ID id, float volume)
{
	if (IsPlaying()) return;
	m_Spatial = false;
	m_Voice = SePlay(id, volume);
}

void SeVoice::PlayAt(SE_ID id, const VEC3& position, int slotA, float volume)
{
	Stop();
	if (id < 0 || id >= SE_COUNT) return;
	m_Spatial = true;
	m_Volume = s_Loaded[id].Volume * volume;
	m_SlotA = slotA;
	m_Voice = SePlayAt(id, position, slotA, -1, volume);
}

void SeVoice::KeepAt(SE_ID id, const VEC3& position, int slotA, float volume)
{
	if (IsPlaying()) MoveTo(position);
	else PlayAt(id, position, slotA, volume);
}

void SeVoice::MoveTo(const VEC3& position)
{
	if (!m_Spatial || s_ListenerCount == 0 || !IsPlaying()) return;
	const SE_MIX mix = SeSpatialCalcMix(s_Listeners, s_ListenerCount, position, m_SlotA, -1);
	SoundSetMix(m_Voice, m_Volume * mix.Volume, mix.Pan);
}

void SeVoice::Stop(void)
{
	SoundStop(m_Voice);
	m_Voice = -1;
}

bool SeVoice::IsPlaying(void) const
{
	return SoundIsPlaying(m_Voice);
}
