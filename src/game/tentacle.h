#pragma once

/// @brief 左右の触腕を待機状態にする（ダミー初期化後）
void TentacleInitialize(void);
/// @brief 保持している敵を離して終了する
void TentacleFinalize(void);
/// @brief カメラ更新後に、入力・命中・保持・投げを更新する
void TentacleUpdate(void);
/// @brief 球を並べて左右の触腕を描く
void TentacleDraw(void);
