#pragma once

/// @file se.h
/// @brief 効果音（assets/sounds/se）を名前で鳴らす部品
/// @details 使い方：
///          - 1回鳴らすだけ：`SePlay(SE_HAMMER_TAP);`（候補が複数ある音は、鳴らすたびにランダムに1つ選ぶ）
///          - 途中で止めたい音（溜め・振り回しなど）：メンバーに `SeVoice m_ChargeSe;` を持ち、
///            始めるときに `m_ChargeSe.Play(SE_LASER_CHARGE);`、やめるときに `m_ChargeSe.Stop();`
///          音とファイルの対応・音量は se.cpp の「調整用の値」の表で変える。
///          鳴らすだけでゲームの計算は変えないので、オンラインでも全員の PC で同じ場面で鳴る

/// @brief 鳴らす効果音の名前
enum SE_ID
{
	// 選択画面
	SE_MENU_CURSOR, ///< カーソル・選んでいる値が動いた
	SE_MENU_DECIDE, ///< 決定して次の画面へ進んだ・準備OKにした
	SE_MENU_BACK, ///< 戻った・準備OKを取り消した
	SE_CHARACTER_SELECT, ///< キャラ選択の画面に入った（「キャラクターを選んでね」）
	SE_MEKAGIRL_DECIDE, ///< メカガールで準備OKにした

	// メカガールの声
	SE_MEKAGIRL_ATTACK, ///< 腕を使い始めた（4つからランダム）
	SE_MEKAGIRL_DAMAGE, ///< ダメージを受けた（3つからランダム）
	SE_MEKAGIRL_FALL, ///< 場外へ落ちた

	// 腕の共通
	SE_ARM_BREAK, ///< 腕が壊れた（耐久が尽きた）
	SE_ARM_HOLD, ///< 長押しでためはじめた
	SE_ARM_GUARD, ///< ガードで攻撃を防いだ

	// 触手
	SE_TENTACLE_EXTEND, ///< 伸ばした
	SE_TENTACLE_SWING, ///< つかんだ相手を振り回した
	SE_TENTACLE_SWING_HIT, ///< 振り回した相手がほかの相手に当たった
	SE_TENTACLE_SLAM, ///< つかんだ相手を叩きつけた

	// ハンマー
	SE_HAMMER_TAP, ///< 単押しで叩いた
	SE_HAMMER_HOLD, ///< 長押しで衝撃波を出した

	// ナックル
	SE_KNUCKLE_TAP, ///< 単押しのパンチが当たった（3つからランダム）
	SE_KNUCKLE_MISS, ///< 単押しのパンチが空振りした
	SE_KNUCKLE_HOLD, ///< 長押しの突進を出した

	// 刀
	SE_KATANA_HIT, ///< 単押しの居合が当たった
	SE_KATANA_MISS, ///< 単押しの居合が空振りした
	SE_KATANA_DASH, ///< 長押しで移動した

	// レーザー
	SE_LASER_SHOT, ///< 単押しで撃った
	SE_LASER_CHARGE, ///< 長押しで溜めている（途中で止める）
	SE_LASER_CANNON, ///< 長押しで撃った

	// 槍
	SE_SPEAR_THROW, ///< 投げた・突いた
	SE_SPEAR_STAB, ///< 刺さった（当たった）
	SE_SPEAR_PULL, ///< 当たったあとに引き寄せる・移動する

	// モーニングスター
	SE_FLAIL_SPIN, ///< 振り回している（途中で止める）
	SE_FLAIL_HIT, ///< 当たった
	SE_FLAIL_CHAIN, ///< 鎖を伸ばして投げた

	// ミサイル
	SE_MISSILE_LOCK, ///< ロックオンした
	SE_MISSILE_FIRE, ///< 撃った
	SE_MISSILE_EXPLODE, ///< 爆発した

	SE_COUNT ///< 効果音の数（鳴らす名前ではない）
};

/// @brief 効果音のファイルを全部読み込む（SoundInitialize のあとに1回呼ぶ。読めなかった音は鳴らないだけ）
void SeInitialize(void);

/// @brief 効果音を1回鳴らす（候補が複数ある音はランダムに1つ選ぶ）
/// @param id 鳴らす音
/// @param volume 表の音量に掛ける倍率（1.0 で表のまま）
/// @return 鳴らした音の番号（SoundStop で止められる。鳴らせなかったら -1）
int SePlay(SE_ID id, float volume = 1.0f);

/// @brief 途中で止めたい効果音を1つ受け持つ（溜め・振り回しなど。腕のメンバーに持つ）
class SeVoice
{
public:
	/// @brief 前の音を止めてから、効果音を鳴らす
	/// @param id 鳴らす音
	/// @param volume 表の音量に掛ける倍率
	void Play(SE_ID id, float volume = 1.0f);
	/// @brief 鳴っていなければ鳴らす（毎フレーム呼んでも重ならない。鳴り終わったらもう一度鳴らす）
	/// @param id 鳴らす音
	/// @param volume 表の音量に掛ける倍率
	void Keep(SE_ID id, float volume = 1.0f);
	/// @brief 鳴らしている音を止める（鳴っていなければ何もしない）
	void Stop(void);
	/// @brief 鳴らしている音がまだ鳴っているか
	/// @return 鳴っていれば true
	bool IsPlaying(void) const;

private:
	int m_Voice = -1; // SoundPlay の番号（鳴らしていなければ -1）
};
