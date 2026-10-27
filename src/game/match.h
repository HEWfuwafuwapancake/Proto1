#pragma once

#include "physics/matchcalc.h"

class DummyManager;
class ArmManager;

/// @file match.h
/// @brief 試合の流れ（制限時間 90 秒・ダミーを場外へぶっ飛ばすと +1・時間切れ・やり直し）
/// @details 時間切れのあとは、プレイヤー・ダミー・触腕が止まる（カメラは回せる）。
///          OPTIONS / R で、いつでも最初からやり直せる。制限時間は match.cpp の「調整用の値」、計算は physics/matchcalc.h

/// @brief 試合の時間・得点・時間切れを管理する
class Match
{
public:

	/// @brief 試合を始める（ダミーの初期化のあとに呼ぶ）
	void Initialize(const DummyManager& dummies, ArmManager& arms);

	/// @brief 状態を最初に戻す（参照はそのまま）
	void Reset(void);

	/// @brief 試合の終了処理（今は何もしない）
	void Finalize(void);

	/// @brief 場外の回数を数えて時間を進める
	/// @details 毎フレーム、DummyManager::Update のあと（ArmManager::Update のあと）に呼ぶ。やり直しは Game が受け持つ
	void Update(void);

	/// @brief 試合中か取得する（時間切れのあとは、プレイヤー・ダミー・触腕を動かさないために使う）
	/// @return 試合中なら true
	bool IsPlaying(void) const;

	/// @brief 画面に出す残り秒数を取得する（No.15 の表示用）
	/// @return 残り秒数（切り上げ。時間切れで 0）
	int GetRemainingSeconds(void) const;

	/// @brief 得点を取得する（No.15 の表示用）
	/// @return ダミーを場外へぶっ飛ばした回数
	int GetScore(void) const;

	/// @brief 試合の状態を取得する（No.15 の表示用）
	/// @return 試合中か、時間切れか
	MATCH_STATE GetState(void) const;

private:
	MatchCalc m_Match = {};
	const DummyManager* m_Dummies = nullptr;
	ArmManager* m_Arms = nullptr;
};
