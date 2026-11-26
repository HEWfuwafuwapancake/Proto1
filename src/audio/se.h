#pragma once

/// @file se.h
/// @brief 効果音（assets/sounds/se）を名前で鳴らす部品
/// @details 使い方：
///          - メニューなど場所のない音を1回鳴らす：`SePlay(SE_MENU_DECIDE);`（候補が複数ある音は、鳴らすたびにランダムに1つ選ぶ）
///          - 戦闘の音を場所つきで鳴らす：`SePlayAt(SE_HAMMER_TAP, 位置, 攻撃した人, 当てられた人);`
///            （聞き手（この PC で遊ぶ人）からの距離で小さくなり、カメラの向きで左右が決まる。SeSetListeners で毎フレーム聞き手を渡す）
///          - 途中で止めたい音（溜め・振り回しなど）：メンバーに `SeVoice m_ChargeSe;` を持ち、
///            始めるときに `m_ChargeSe.PlayAt(...)`、動いたら `m_ChargeSe.MoveTo(位置)`、やめるときに `m_ChargeSe.Stop();`
///          音とファイルの対応・音量は se.cpp の「調整用の値」の表で変える。
///          鳴らすだけでゲームの計算は変えない（大きさ・左右は PC ごとの聞き手で変わってよい）

#include "audio/sespatialcalc.h"

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
	SE_KATANA_HIT2, ///< 刀のヒット音の2つ目（今は刀では使わず、槍が刺さったときに使う）
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

/// @brief 効果音を1回鳴らす（場所を使わず、真ん中で鳴らす。メニューの音など）
/// @param id 鳴らす音
/// @param volume 表の音量に掛ける倍率（1.0 で表のまま）
/// @return 鳴らした音の番号（SoundStop で止められる。鳴らせなかったら -1）
int SePlay(SE_ID id, float volume = 1.0f);

/// @brief 聞き手（この PC で遊ぶ人）を設定する（試合の間、毎フレーム渡す。試合の外では 0 人にする）
/// @param listeners 聞き手の並び（スロットの小さい順。count が 0 なら nullptr でよい）
/// @param count 聞き手の数（0〜SE_LISTENER_MAX）
void SeSetListeners(const SE_LISTENER* listeners, int count);

/// @brief スロットの人がこの PC で遊ぶ人（聞き手）か
/// @param slot スロット番号
/// @return 聞き手なら true（CPU・ダミー・ほかの PC の人は false）
bool SeIsListener(int slot);

/// @brief 戦闘の効果音を、出た場所に合わせた大きさと左右で1回鳴らす
/// @param id 鳴らす音
/// @param position 音の出た場所（振り・発射はその腕、着弾・爆発は当たった場所）
/// @param slotA 音に関わる人（攻撃した人など。いなければ -1）
/// @param slotB 音に関わるもう1人（当てられた人など。いなければ -1）
/// @param volume 表の音量に掛ける倍率
/// @return 鳴らした音の番号（鳴らせなかったら -1）
int SePlayAt(SE_ID id, const VEC3& position, int slotA, int slotB = -1, float volume = 1.0f);

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
	/// @brief 前の音を止めてから、場所つきで鳴らす（SePlayAt と同じ大きさと左右）
	/// @param id 鳴らす音
	/// @param position 音の出た場所
	/// @param slotA 音に関わる人（いなければ -1）
	/// @param volume 表の音量に掛ける倍率
	void PlayAt(SE_ID id, const VEC3& position, int slotA, float volume = 1.0f);
	/// @brief 鳴っていなければ場所つきで鳴らし、鳴っていれば場所だけ動かす（毎フレーム呼んでよい）
	/// @param id 鳴らす音
	/// @param position 音の出た場所
	/// @param slotA 音に関わる人（いなければ -1）
	/// @param volume 表の音量に掛ける倍率
	void KeepAt(SE_ID id, const VEC3& position, int slotA, float volume = 1.0f);
	/// @brief 鳴らしている音の場所を動かす（大きさと左右を今の聞き手で決め直す。場所なしで鳴らした音は変えない）
	/// @param position 音の出た場所
	void MoveTo(const VEC3& position);
	/// @brief 鳴らしている音を止める（鳴っていなければ何もしない）
	void Stop(void);
	/// @brief 鳴らしている音がまだ鳴っているか
	/// @return 鳴っていれば true
	bool IsPlaying(void) const;

private:
	int m_Voice = -1; // SoundPlay の番号（鳴らしていなければ -1）
	bool m_Spatial = false; // 場所つきで鳴らしたか
	float m_Volume = 1.0f; // 表の音量と倍率を掛けた大きさ（場所で決まる倍率を掛ける前）
	int m_SlotA = -1; // 音に関わる人
};
