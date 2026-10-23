#pragma once

/// @file gametime.h
/// @brief ゲームの時間（1フレームの秒数・経過時間・時間の進む速さ）
/// @details 1フレームの秒数は 1/60 秒で固定する（メインループがちょうど1秒に60回なので、
///          固定のほうが計算が安定して、同じ操作で同じ結果になる）。
///          DirectX と Windows を使わないので、単体テスト（tests/Proto1Tests）からそのまま使える

/// @brief 時間を最初の状態にする（経過時間 0・速さ 1）
void GameTimeInitialize(void);

/// @brief 時間を1フレーム分進める（毎フレーム、Update の最初に1回だけ呼ぶ）
void GameTimeUpdate(void);

/// @brief ゲームの計算に使う1フレームの秒数を取得する
/// @return 1/60 秒 × 時間の進む速さ（速さが 0 なら 0）
float GameTimeGetDeltaTime(void);

/// @brief ゲームが始まってからの秒数を取得する
/// @details 時間の進む速さが掛かった秒数（速さ 0 の間は増えない）
/// @return 経過時間（秒）
float GameTimeGetElapsed(void);

/// @brief 時間の進む速さを設定する（ヒットストップ・スローモーションに使う）
/// @param scale 速さ（0：止まる、1：ふつう、0.5：半分の速さ）。負の値は 0 にする
void GameTimeSetScale(float scale);

/// @brief 時間の進む速さを取得する
/// @return 速さ（0：止まっている、1：ふつう）
float GameTimeGetScale(void);
