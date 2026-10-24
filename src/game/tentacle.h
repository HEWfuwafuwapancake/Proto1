#pragma once

/// @file tentacle.h
/// @brief 触腕（左右の腕で敵をつかんで、カメラの向きへ投げる）

/// @brief 左右の触腕を待機状態にする（ダミー初期化後）
void TentacleInitialize(void);
/// @brief つかんでいる敵をダメージなしで離し、左右の触腕を待機状態に戻す（時間切れ・やり直しのとき）
void TentacleReset(void);
/// @brief 保持している敵を離して終了する
void TentacleFinalize(void);
/// @brief カメラ更新後に、入力・命中・保持・投げを更新する
void TentacleUpdate(void);
/// @brief 球を並べて左右の触腕を描く
void TentacleDraw(void);

/// @brief どちらかの腕が敵をつかんでいるか返す（No.33 のロックオン停止に使う）
/// @return 左右いずれかの腕が保持中ならtrue、どちらも保持中でなければfalse
bool TentacleIsHolding(void);
