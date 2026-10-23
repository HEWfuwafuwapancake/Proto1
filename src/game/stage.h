#pragma once

#include "physics/stagecalc.h"

/// @file stage.h
/// @brief ステージ（空に浮かぶ四角い浮島が1つ）。場外の線は下だけ
/// @details 浮島の大きさ・場外の下の線は stage.cpp の「調整用の値」にある。計算は physics/stagecalc.h

/// @brief 浮島と場外の線を「調整用の値」から決める（ShapeInitialize のあと、PlayerInitialize の前に呼ぶ）
void StageInitialize(void);

/// @brief ステージの終了処理（今は何もしない）
void StageFinalize(void);

/// @brief ステージを更新する（毎フレーム呼ぶ。今は何もしない。崩れる・縮むなどは入れないと決まっている）
void StageUpdate(void);

/// @brief 浮島を描く（render/shape.h の箱を使う）
void StageDraw(void);

/// @brief 浮島と場外の線を取得する（着地や場外の判定に使う）
/// @return ステージ
const STAGE& StageGet(void);
