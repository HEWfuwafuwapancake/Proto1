#pragma once

#include "physics/stagecalc.h"


/// @file stage.h
/// @brief ステージ（浮島と高台）。場外の線は下だけ
/// @details 浮島の大きさ・高台の配置・場外の下の線は stage.cpp の「調整用の値」にある。計算は physics/stagecalc.h

/// @brief 浮島・高台・場外の線を管理する
class Stage
{
public:

	/// @brief 浮島・高台・場外の線を「調整用の値」から決める（ShapeInitialize のあと、FighterManager::Initialize の前に呼ぶ）
	void Initialize(void);

	/// @brief 状態を最初に戻す（参照はそのまま）
	void Reset(void);
	/// @brief 浮島の幅・奥行きと高台の配置を元の値から指定倍率にする（高さと高台の大きさはそのまま）
	void SetSizeScale(float scale);

	/// @brief ステージの終了処理（今は何もしない）
	void Finalize(void);

	/// @brief ステージを更新する（毎フレーム呼ぶ。今は何もしない。崩れる・縮むなどは入れないと決まっている）
	void Update(void);

	/// @brief 浮島と高台を描く（render/shape.h の箱を使う）
	void Draw(void) const;

	/// @brief 浮島・高台・場外の線を取得する（着地や場外の判定に使う）
	/// @return ステージ
	const STAGE& GetStage(void) const;

private:
	STAGE m_Stage = {};		// 浮島・高台・場外の線
};
