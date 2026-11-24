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
	};

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
		{ SE_ARM_GUARD, 0.9f, { SE_ARM "00_共通/防御.wav" } },

		{ SE_TENTACLE_EXTEND, 0.7f, { SE_ARM "01_触手/伸ばしたとき.wav" } },
		{ SE_TENTACLE_SWING, 0.8f, { SE_ARM "01_触手/振り回したとき.wav" } },
		{ SE_TENTACLE_SWING_HIT, 0.9f, { SE_ARM "01_触手/振り回したときのヒット音.wav" } },
		{ SE_TENTACLE_SLAM, 1.0f, { SE_ARM "01_触手/敵を叩きつけたとき.wav" } },

		{ SE_HAMMER_TAP, 0.9f, { SE_ARM "02_ハンマー/ハンマー単押し.wav" } },
		{ SE_HAMMER_HOLD, 1.0f, { SE_ARM "02_ハンマー/ハンマー長押し.wav" } },

		{ SE_KNUCKLE_TAP, 0.9f, { SE_ARM "03_ナックル/ナックル単押し (1).wav", SE_ARM "03_ナックル/ナックル単押し (2).wav", SE_ARM "03_ナックル/ナックル単押し (3).wav" } },
		{ SE_KNUCKLE_MISS, 0.7f, { SE_ARM "03_ナックル/ナックル空振り.wav" } },
		{ SE_KNUCKLE_HOLD, 1.0f, { SE_ARM "03_ナックル/ナックル長押し.wav" } },

		{ SE_KATANA_HIT, 0.9f, { SE_ARM "04_刀/刀単押しヒット.wav" } }, // 候補：刀単押しヒット2.wav
		{ SE_KATANA_MISS, 0.7f, { SE_ARM "04_刀/刀空振り.wav" } },
		{ SE_KATANA_DASH, 0.9f, { SE_ARM "04_刀/刀長押し移動1.wav" } }, // 候補：刀長押し移動2.wav

		{ SE_LASER_SHOT, 0.8f, { SE_ARM "05_レーザー/レーザー単押し.wav" } },
		{ SE_LASER_CHARGE, 0.7f, { SE_ARM "05_レーザー/レーザー長押し溜め.wav" } }, // 候補：レーザー長押し溜め2.wav
		{ SE_LASER_CANNON, 1.0f, { SE_ARM "05_レーザー/レーザー長押し発射.wav" } },

		{ SE_SPEAR_THROW, 0.8f, { SE_ARM "06_槍/槍投げ.wav" } },
		{ SE_SPEAR_STAB, 1.0f, { SE_ARM "06_槍/槍が刺さったとき.wav" } },
		{ SE_SPEAR_PULL, 0.8f, { SE_ARM "06_槍/槍ヒット時移動.wav" } },

		{ SE_FLAIL_SPIN, 0.6f, { SE_ARM "07_モーニングスター/モーニングスター振り回しているとき.wav" } },
		{ SE_FLAIL_HIT, 1.0f, { SE_ARM "07_モーニングスター/モーニングスター攻撃時.wav" } },
		{ SE_FLAIL_CHAIN, 0.8f, { SE_ARM "07_モーニングスター/モーニングスター鎖.wav" } },

		{ SE_MISSILE_LOCK, 0.7f, { SE_ARM "08_ミサイル/ロックオン2.wav" } }, // 候補：ロックオン1.wav（9秒と長い）
		{ SE_MISSILE_FIRE, 0.8f, { SE_ARM "08_ミサイル/発射.wav" } },
		{ SE_MISSILE_EXPLODE, 1.0f, { SE_ARM "08_ミサイル/爆発1.wav" } }, // 候補：爆発2.wav
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
	};

	SE_LOADED s_Loaded[SE_COUNT];
	// 候補を選ぶ乱数。ゲームの乱数（オンラインで全員そろえる）とは別にする。
	std::minstd_rand s_Random(12345u);
}

void SeInitialize(void)
{
	for (const SE_ENTRY& entry : SE_TABLE) {
		SE_LOADED& loaded = s_Loaded[entry.Id];
		loaded = {};
		loaded.Volume = entry.Volume;
		for (const char* file : entry.Files) {
			if (file == nullptr) continue;
			const int sound = SoundLoad(file);
			if (sound >= 0) loaded.Sounds[loaded.Count++] = sound;
		}
	}
}

int SePlay(SE_ID id, float volume)
{
	if (id < 0 || id >= SE_COUNT) return -1;
	const SE_LOADED& loaded = s_Loaded[id];
	if (loaded.Count == 0) return -1;
	const int pick = loaded.Count == 1 ? 0 : static_cast<int>(s_Random() % static_cast<unsigned>(loaded.Count));
	return SoundPlay(loaded.Sounds[pick], loaded.Volume * volume);
}

void SeVoice::Play(SE_ID id, float volume)
{
	Stop();
	m_Voice = SePlay(id, volume);
}

void SeVoice::Keep(SE_ID id, float volume)
{
	if (!IsPlaying()) m_Voice = SePlay(id, volume);
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
