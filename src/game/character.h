#pragma once

#include "physics/scenecalc.h"
#include "physics/collision.h"
#include "render/skinnedmodel.h"

/// @brief キャラごとの表示とモデルの調整値
struct CHARACTER_DATA
{
	const char* Name; ///< 選択画面とロビーに出す表示名
	std::array<const char*, static_cast<size_t>(SKINNED_CLIP::Count)> Paths; ///< 待機・走り・左腕・右腕・後ろ走り・ジャンプ・落下・着地の FBX（後ろの4つは nullptr なら代わりのクリップ）
	SKINNED_BONES_DESC Bones; ///< 腕のクリップを重ねる上半身の根元と、ねじる背骨の名前
	float ModelHeight; ///< FBX の元の高さ
	float ModelFootY; ///< FBX の元の足元の高さ
	float ModelYaw; ///< FBX の正面をゲームの +Z に合わせる角度（ラジアン）
	VEC3 Shoulder; ///< 描画用の肩位置（X は左右の幅、Y は足元からの高さ、Z は前方）。腕の骨がないキャラだけ使う
	std::array<std::array<const char*, 4>, 2> ArmChains; ///< 左手・右手の腕の骨（肩・肘・手首・手先）。腕の付け根・待機の向き・長さをアニメに合わせる（nullptr なら Shoulder の固定位置）
	float ArmScale; ///< 腕のモデルを描く大きさの倍率（メカを 1 とする。触腕の太さ・節の大きさ・取付のずらし）
	float ArmLift; ///< 腕の骨に付けるとき、腕の付け根を肩の骨より上へ出す高さ（m。肩の部分を上に突き出すため）

	/// @brief キャラの調整値を取得する
	static const CHARACTER_DATA& Get(CHARACTER_TYPE character);
};
