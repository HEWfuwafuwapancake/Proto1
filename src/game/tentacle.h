#pragma once

/// @file tentacle.h
/// @brief 触腕（左右の腕で敵をつかんで、カメラの向きへ投げる）

/// @brief 左右の触腕を待機状態にする（ダミー初期化後）
void TentacleInitialize(void);
/// @brief つかんでいる敵をダメージなしで離し、片手の触腕を待機状態に戻す
/// @param hand 左 0・右 1
void TentacleResetHand(int hand);
/// @brief 保持している敵を離して終了する
void TentacleFinalize(void);
/// @brief カメラ更新後に、片手の入力・命中・保持・投げを更新する
/// @param hand 左 0・右 1
/// @param trigger 押した瞬間かを設定する（true：押した瞬間、false：それ以外）
void TentacleUpdateHand(int hand, bool trigger);
/// @brief 片手の触腕を描く（OBJ をつなぐ。読み込み失敗時は球）
/// @param hand 左 0・右 1
void TentacleDrawHand(int hand);

/// @brief 片手の腕が出ているか返す（キャラを画面の正面へ向け続けるため）
/// @param hand 左 0・右 1
/// @return 伸長・保持・収縮中なら true、待機中なら false
bool TentacleIsActiveHand(int hand);

/// @brief 片手の腕が敵をつかんでいるか返す（No.33 のロックオン停止に使う）
/// @param hand 左 0・右 1
/// @return 保持中なら true、それ以外は false
bool TentacleIsHoldingHand(int hand);
