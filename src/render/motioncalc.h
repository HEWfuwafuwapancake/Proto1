#pragma once
#include "render/animcalc.h"

/// @file motioncalc.h
/// @brief キャラの見た目の動きの計算（脚の向き・上半身のねじれ・傾き・クリップの選び方・再生速度）
/// @details 描画だけに使い、ゲームの計算（当たり判定・向き）や通信には使わない。
///          DirectX を使わないので、単体テスト（tests/Proto1Tests）からそのまま使える

/// @brief 1フレーム分の体の状態
struct MOTION_INPUT
{
	float VelocityX = 0.0f; ///< 横の速度の X（1秒に進む距離）
	float VelocityY = 0.0f; ///< 上下の速度（上がプラス）
	float VelocityZ = 0.0f; ///< 横の速度の Z
	bool OnGround = true; ///< 浮島に立っているか（true：立っている、false：空中）
	float AimYaw = 0.0f; ///< 上半身を向ける左右の向き（ゲームの向き。ラジアン。0で+Z、プラスで右回り）
	bool Locomotion = true; ///< 走り・ジャンプの動きをするか（true：人の通常の動き、false：吹っ飛び中など。脚は狙いの向きにそろえ、空中なら落下だけにする）
};

/// @brief 見た目の動きの調整用の値
struct MOTION_PARAMS
{
	float FullSpeed = 6.0f; ///< 再生速度 1 になる横の速さ（1秒に進む距離）
	float RunThreshold = 0.25f; ///< これより速いと走りにする横の速さ
	float LegTurnSpeed = 9.4f; ///< 走っているとき脚が回る速さ（ラジアン／秒）
	float IdleTurnSpeed = 6.3f; ///< 止まっているとき脚が狙いの向きへ回る速さ（ラジアン／秒）
	float MaxTwist = 1.3f; ///< 上半身を脚からねじれる角度の上限（ラジアン）
	float StrafeAngle = 0.872665f; ///< 地面で横走りを始める、進む向きと狙いの差（約50度。ラジアン）
	float BackAngle = 1.92f; ///< 進む向きと狙いの差がこれを超えると後ろ走りにする（ラジアン）
	float BackHysteresis = 0.17f; ///< 後ろ走り・横走りをやめるときの余裕（ラジアン。境目で行ったり来たりしないように）
	float LandSeconds = 0.2f; ///< 着地の動きを見せる秒数
	float MinPlaySpeed = 0.5f; ///< 走りの再生速度の下限
	float LeanForward = 0.12f; ///< 全速で走ったときの前傾（ラジアン。後ろ走りは半分だけ後ろへ）
	float LeanTurn = 0.06f; ///< 脚が 1 ラジアン／秒で回るときの内側への傾き（ラジアン）
	float LeanSideMax = 0.17f; ///< 内側への傾きの上限（ラジアン）
	float LeanRate = 8.0f; ///< 傾きが目標へ近づく速さ（1秒あたりの割合）
};

/// @brief やられの全身クリップの指定（描画だけ）
struct MOTION_HIT_REACTION
{
	bool Active = false; ///< やられのクリップを使うか（true：使う、false：通常の動きを使う）
	SKINNED_CLIP Clip = SKINNED_CLIP::Idle; ///< 全身で再生するクリップ
	float ClipTime = -1.0f; ///< 再生位置の秒（負なら経過時間で進める）
};

/// @brief つかまれ・のけぞり・しびれ・吹っ飛びの順に、やられのクリップを選ぶ
/// @param held つかまれ中か（true：つかまれ中、false：つかまれていない）
/// @param stunned しびれ中か（true：しびれ中、false：しびれていない）
/// @param launched 吹っ飛び中か（true：吹っ飛び中、false：吹っ飛んでいない）
/// @param launchElapsed 吹っ飛ばされてからの秒数（復活時はのけぞりの秒数以上にする）
/// @param flinchSeconds のけぞりを見せる秒数
/// @return 使うか・クリップ・再生位置（ループするクリップは負）
MOTION_HIT_REACTION MotionCalcHitReaction(bool held, bool stunned, bool launched, float launchElapsed, float flinchSeconds);

/// @brief 1体分の見た目の動き（Fighter ごとに持つ）
class MotionCalc
{
public:
	/// @brief 脚と狙いを同じ向きにし、傾き・着地を消す
	/// @param yaw 向き（ラジアン）
	void Reset(float yaw);
	/// @brief 1フレーム分進める
	/// @param input 体の状態
	/// @param params 調整用の値
	/// @param dt 1フレームの秒数（0以下なら何も変えない）
	void Step(const MOTION_INPUT& input, const MOTION_PARAMS& params, float dt);
	/// @brief 脚（モデル全体）の左右の向きを取得する（ラジアン）
	float GetLegYaw() const { return m_LegYaw; }
	/// @brief 脚から見た上半身のねじれを取得する（ラジアン。プラスで右回り）
	float GetTwist() const { return m_Twist; }
	/// @brief 前傾を取得する（ラジアン。プラスで前へ傾く）
	float GetLeanPitch() const { return m_LeanPitch; }
	/// @brief 横の傾きを取得する（ラジアン。プラスで右へ傾く）
	float GetLeanRoll() const { return m_LeanRoll; }
	/// @brief 下半身（全身）で再生するクリップを取得する
	SKINNED_CLIP GetClip() const { return m_Clip; }
	/// @brief クリップの再生速度の倍率を取得する
	float GetPlaySpeed() const { return m_PlaySpeed; }
	/// @brief 後ろ走り中か取得する
	bool IsBackward() const { return m_Backward; }

private:
	float m_LegYaw = 0.0f;
	float m_Twist = 0.0f;
	float m_LeanPitch = 0.0f;
	float m_LeanRoll = 0.0f;
	float m_LandTimer = 0.0f;
	float m_PlaySpeed = 1.0f;
	bool m_Backward = false;
	bool m_Strafing = false;
	bool m_WasOnGround = true;
	SKINNED_CLIP m_Clip = SKINNED_CLIP::Idle;
};
