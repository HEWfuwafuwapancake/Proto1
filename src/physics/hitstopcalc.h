#pragma once

/// @file hitstopcalc.h
/// @brief ヒットストップ（当たった瞬間に当てた人と当てられた人を数フレーム止める）の計算
/// @details DirectX と Windows を使わないので、単体テスト（tests/Proto1Tests）からそのまま使える。
///          試合の動きが変わるので、オンラインでは全員の PC で同じに数える（フレーム数で数える）。

/// @brief 当たりの強さから、止めるフレーム数を決める値
struct HITSTOP_PARAMS
{
	float MinDamage; ///< これより小さいダメージ%の当たりは止めない（振り回しの連続ヒットなど）
	float MediumDamage; ///< このダメージ%以上は「中」
	float HeavyDamage; ///< このダメージ%以上は「強」
	float FinishSpeed; ///< 吹っ飛ぶ速さがこれ以上なら「決め手」（場外になりやすい一撃）
	int LightFrames; ///< 「弱」の当たりで止めるフレーム数
	int MediumFrames; ///< 「中」の当たりで止めるフレーム数
	int HeavyFrames; ///< 「強」の当たりで止めるフレーム数
	int FinishFrames; ///< 「決め手」で止めるフレーム数
};

/// @brief 当たりのダメージ%と吹っ飛ぶ速さから、止めるフレーム数を求める
/// @param damage 当たりで足したダメージ%
/// @param launchSpeed 吹っ飛ぶ速さ（1秒あたりの移動量。吹っ飛ばない当たりは 0）
/// @param params 強さの区切りとフレーム数
/// @return 止めるフレーム数（止めないときは 0）
int HitstopCalcFrames(float damage, float launchSpeed, const HITSTOP_PARAMS& params);

/// @brief 1人分のヒットストップの残りを数える
class HitstopCalc
{
public:
	/// @brief 止まっていない状態へ戻す
	void Reset();
	/// @brief 止め始める（すでに止まっていれば、残りの長い方にする。足し算はしない）
	/// @param frames 止めるフレーム数（0 以下は何もしない）
	void Start(int frames);
	/// @brief 1フレーム進める
	/// @return このフレームを止めるなら true（残りを1減らす）
	bool Step();
	/// @brief 残りのフレーム数を取得する
	int GetFrames() const { return m_Frames; }

private:
	int m_Frames = 0; ///< 止める残りのフレーム数
};
