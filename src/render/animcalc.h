#pragma once
#include <array>
#include <vector>

/// @brief キャラのクリップ番号（FBX の並びと同じ）
enum class SKINNED_CLIP
{
	Idle, ///< 待機
	Run, ///< 走り
	Left, ///< 左腕を前に出す
	Right, ///< 右腕を前に出す
	RunBack, ///< 後ろ走り（ファイルがなければ走りを逆に再生する）
	Jump, ///< ジャンプの上昇（ループしない。ファイルがなければ待機）
	Fall, ///< 落下（ファイルがなければ待機）
	Land, ///< 着地（ループしない。ファイルがなければ待機）
	Count ///< クリップの数
};

/// @brief アニメで使う3成分
struct ANIM_VEC3
{
	float X, Y, Z; ///< 各軸の成分
};

/// @brief 単位クォータニオン
struct ANIM_QUAT
{
	float X, Y, Z; ///< ベクトル部の各軸の成分
	float W; ///< スカラー部
};

/// @brief ノードの親に対する姿勢
struct ANIM_TRANSFORM
{
	ANIM_VEC3 Position{}; ///< 移動
	ANIM_QUAT Rotation{ 0, 0, 0, 1 }; ///< 回転
	ANIM_VEC3 Scale{ 1, 1, 1 }; ///< 拡大率
};

/// @brief キー1つの時刻と値
template<class T>
struct ANIM_KEY
{
	float Time; ///< 秒単位の時刻
	T Value; ///< キーの値
};

/// @brief ノード1つ分の時刻順のキー
struct ANIM_TRACK
{
	std::vector<ANIM_KEY<ANIM_VEC3>> Positions; ///< 移動キー
	std::vector<ANIM_KEY<ANIM_QUAT>> Rotations; ///< 回転キー
	std::vector<ANIM_KEY<ANIM_VEC3>> Scales; ///< 拡大率キー
};

/// @brief 行ベクトル用の4行4列行列
using ANIM_MATRIX = std::array<std::array<float, 4>, 4>;

/// @brief 先頭と同じ末尾キーの時刻で折り返し、ループ内の秒を求める
float AnimCalcLoopTime(float seconds, float lastFrameSeconds);

/// @brief キーの間を補間する。キーがない成分は基準の姿勢を使う
ANIM_TRANSFORM AnimCalcSample(const ANIM_TRACK& track, float time, const ANIM_TRANSFORM& bindPose);

/// @brief 切り替え開始からの秒数を 0〜1 の補間の重みに変換する
float AnimCalcBlendWeight(float elapsed, float duration);

/// @brief 移動・拡大率を線形補間し、回転を球面線形補間する
ANIM_TRANSFORM AnimCalcBlend(const ANIM_TRANSFORM& from, const ANIM_TRANSFORM& to, float weight);

/// @brief 拡大・回転・移動の順でノードの変換行列を作る
ANIM_MATRIX AnimCalcMatrix(const ANIM_TRANSFORM& pose);

/// @brief ローカル変換に親の変換を掛け、モデル空間の行列にする
ANIM_MATRIX AnimCalcCompose(const ANIM_MATRIX& local, const ANIM_MATRIX& parent);

/// @brief モデル空間の行列を、その原点を中心にモデルの上下の軸（Y）まわりで回す（上半身のねじり）
/// @param global ノードのモデル空間の行列
/// @param angle 回す角度（ラジアン。XMMatrixRotationY と同じ向き）
/// @return 回したあとのモデル空間の行列（原点の位置は変わらない）
ANIM_MATRIX AnimCalcRotateY(const ANIM_MATRIX& global, float angle);
