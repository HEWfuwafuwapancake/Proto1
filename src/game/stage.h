#pragma once

#include "physics/stagecalc.h"

/// @file stage.h
/// @brief ステージ（空に浮かぶ四角い浮島が1つ）。場外の線は下だけ
/// @details 浮島の大きさ・場外の下の線は stage.cpp の「調整用の値」にある。計算は physics/stagecalc.h

/// @brief 浮島の頂点バッファとインデックスバッファを作る
void StageInitialize(void);

/// @brief 浮島のバッファを解放する
void StageFinalize(void);

/// @brief ステージを更新する（毎フレーム呼ぶ）
/// @details 今は確かめ用に、Debug ビルドでカメラが場外に出た・戻ったことを出力ウィンドウに出すだけ（No.8 でプレイヤーに置き換える）
void StageUpdate(void);

/// @brief 浮島を描く
void StageDraw(void);

/// @brief 浮島と場外の線を取得する（着地や場外の判定に使う）
/// @return ステージ
const STAGE& StageGet(void);
