#pragma once

#include "game/arm.h"
#include "audio/se.h"
#include "physics/lasercalc.h"

/// @file arm_laser.h
/// @brief 片手のレーザーの単発・溜めた砲と描画

/// @brief 単押しですぐ当たる単発のレーザー銃、3秒溜めて離すと全員を貫通するレーザー砲を撃つ腕
/// @details ロックオン（ハード・ソフト）中は狙っている相手へ必ず当たる。フリーカメラは画面の真ん中のレティクルの先へ撃つ。
///          溜めている間はカメラが肩越しに寄り、レティクルの周りに溜めのゲージが出る（camera.cpp・hud.cpp）
class LaserArm : public Arm
{
public:
	/// @brief モデルを読み、待機状態にする
	/// @param hand 左 0・右 1
	/// @param links 腕が使うゲームの部品への参照
	LaserArm(int hand, const ARM_LINKS& links);
	/// @brief 溜めと発射を進め、撃ったフレームに当たりを調べる
	/// @param input このフレームの腕の入力（単押しで単発、長押しで溜め、離して発射）
	void Update(const ARM_INPUT& input) override;
	/// @brief 腕・砲身・溜めの光・光線を描画する
	void Draw() override;
	/// @brief 溜めをやめ、光線を消して待機へ戻す
	void Reset() override;
	/// @brief 溜めている間と撃ったあとの休みの間なら true を返す
	bool IsActive() const override;
	/// @brief 敵をつかまないので、常に false を返す
	bool IsHolding() const override;
	/// @brief 撃ったあとの休みの間は、次の単押しを覚えておく
	bool CanTakeTap() const override;
	/// @brief 撃ったあとの休みの間は、長押しを待たせる
	bool CanTakeHold() const override;
	/// @brief 溜めている間は歩く速さを下げる
	float GetMoveScale() const override;
	/// @brief 構えと撃った反動の上半身の攻撃クリップを取得する
	ARM_BODY_CLIP GetBodyClip() const override;
	/// @brief 溜めている間と撃ったあとの休みの間は、狙う向きへ体を向けるので true を返す
	bool IsAiming() const override;
	/// @brief 溜めの割合を取得する（溜めていなければ -1）
	float GetAimCharge() const override;

private:
	VEC3 GetAimPoint(int* target) const;
	void Fire(LASER_FIRE fire);
	LaserCalc m_Calc;
	int m_ArmModel = -1;
	int m_BarrelModel = -1;
	VEC3 m_Direction = { 0.0f, 0.0f, 1.0f }; // 肩から狙う点への向き（上下も含む。描画だけでなく撃つ向き）
	VEC3 m_BeamEnd = {}; // 最後に撃った光線の終わり（描画だけ）
	float m_BeamSeconds = 0.0f; // 光線を描く残り秒数（描画だけ）
	bool m_BeamCannon = false; // 最後に撃った光線が砲なら true（描画だけ）
	float m_Straighten = 0.0f; // 描画だけのまっすぐ度（0：待機、1：狙う向き）
	float m_GlowTime = 0.0f; // 溜めの光を点滅させる時間（描画だけ）
	SeVoice m_ChargeSe; // 溜めている間の音（撃つ・やめるときに止める）
	bool m_WasFull = false; // 前のフレームで溜めきっていたか（満ちた瞬間に光らせる）
	VEC3 GetMuzzle() const; // 砲口のおよその位置（エフェクトを出す所）
	bool m_ChargeSePlaying = false; // 溜めの音を鳴らし始めたか（溜めている間に何度も鳴らさない）
};
