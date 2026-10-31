#pragma once

#include "physics/matchcalc.h"

class FighterManager;
class ArmManager;

/// @file match.h
/// @brief 試合の流れ（制限時間90秒・最後に攻撃して場外へ落とすと+1・時間切れ・やり直し）
/// @details 時間切れのあとは、プレイヤー・ダミー・触腕が止まる（カメラは回せる）。
///          OPTIONS / R で、いつでも最初からやり直せる。制限時間は match.cpp の「調整用の値」、計算は physics/matchcalc.h

/// @brief 試合の時間・得点・時間切れを管理する
class Match
{
public:

	/// @brief 試合を始める（全員の体の初期化のあとに呼ぶ）
	/// @param arms スロットごとの腕（FIGHTER_COUNT 個の配列。時間切れで人のスロットの腕を離す）
	void Initialize(const FighterManager& fighters, ArmManager* arms);

	/// @brief 状態を最初に戻す（参照はそのまま）
	void Reset(void);

	/// @brief 試合の終了処理（今は何もしない）
	void Finalize(void);

	/// @brief 全員のKO数の増分を取り込み、時間を進める
	/// @details 毎フレーム、FighterManager::Update のあと（ArmManager::Update のあと）に呼ぶ。やり直しは Game が受け持つ
	void Update(void);

	/// @brief 試合中か取得する（時間切れのあとは、プレイヤー・ダミー・触腕を動かさないために使う）
	/// @return 試合中なら true
	bool IsPlaying(void) const;

	/// @brief 画面に出す残り秒数を取得する（No.15 の表示用）
	/// @return 残り秒数（切り上げ。時間切れで 0）
	int GetRemainingSeconds(void) const;

	/// @brief 得点を取得する（No.15 の表示用）
	/// @param slot 人のスロット番号（0〜3）
	int GetScore(int slot) const;

	/// @brief 試合の状態を取得する（No.15 の表示用）
	/// @return 試合中か、時間切れか
	MATCH_STATE GetState(void) const;

private:
	int GetTotalScore() const;
	MatchCalc m_Match = {};
	const FighterManager* m_Fighters = nullptr;
	ArmManager* m_Arms = nullptr;
};
