#pragma once

#include "physics/scenecalc.h"
#include "physics/collision.h"
#include "render/skinnedmodel.h"

/// @brief キャラごとの表示とモデルの調整値
struct CHARACTER_DATA
{
	const char* Name; ///< 選択画面とロビーに出す表示名
	std::array<const char*, static_cast<size_t>(SKINNED_CLIP::Count)> Paths; ///< 待機・走り・左腕・右腕の FBX
	float ModelHeight; ///< FBX の元の高さ
	float ModelFootY; ///< FBX の元の足元の高さ
	float ModelYaw; ///< FBX の正面をゲームの +Z に合わせる角度（ラジアン）
	VEC3 Shoulder; ///< 描画用の肩位置（X は左右の幅、Y は足元からの高さ、Z は前方）

	/// @brief キャラの調整値を取得する
	static const CHARACTER_DATA& Get(CHARACTER_TYPE character);
};
