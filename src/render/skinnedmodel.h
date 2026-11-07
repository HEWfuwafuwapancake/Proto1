#pragma once
#include "core/dx11.h"
#include "render/animcalc.h"

/// @brief メカのクリップ番号
enum class SKINNED_CLIP
{
	Idle, ///< 待機
	Run, ///< 走り
	Left, ///< 左腕を前に出す
	Right, ///< 右腕を前に出す
	Count ///< クリップの数
};

/// @brief Fighter ごとに持つ描画専用のアニメ状態
struct SKINNED_POSE
{
	SKINNED_CLIP Clip = SKINNED_CLIP::Idle; ///< 再生中のクリップ
	float Time = 0.0f; ///< クリップ内の秒
	float BlendElapsed = 0.0f; ///< 切り替えからの秒
	std::vector<ANIM_TRANSFORM> Local; ///< 現在の各ノードの姿勢
	std::vector<ANIM_TRANSFORM> BlendFrom; ///< 切り替え直前に描いていた姿勢
	std::vector<ANIM_MATRIX> Global; ///< 各ノードのモデル空間の行列
};

/// @brief 待機の FBX からモデルを作り、列挙の順に4つのクリップを読む。成功したら true
bool SkinnedModelLoad(const std::array<const char*, static_cast<size_t>(SKINNED_CLIP::Count)>& paths);

/// @brief 共有しているモデルとクリップを解放する
void SkinnedModelFinalize();

/// @brief 待機の先頭の姿勢に戻す
void SkinnedModelReset(SKINNED_POSE& pose);

/// @brief 指定されたクリップへ切り替え、ゲームの経過秒で姿勢を進める
void SkinnedModelUpdate(SKINNED_POSE& pose, SKINNED_CLIP clip, float dt, float blendSeconds);

/// @brief 姿勢とワールド行列を使い、全体を指定色で描く
void SkinnedModelDraw(const SKINNED_POSE& pose, const XMMATRIX& world, const XMFLOAT4& color);
