#pragma once

#include "physics/particlecalc.h"
#include "physics/randomcalc.h"

/// @brief 出すエフェクトの型（出し方は effect.cpp の「調整用の値」の表で決める）
enum EFFECT_ID
{
	EFFECT_HIT_SPARK_LIGHT, ///< 弱い当たりの白い火花（小）
	EFFECT_HIT_SPARK_MEDIUM, ///< 中くらいの当たりの火花
	EFFECT_HIT_SPARK_HEAVY, ///< 強い当たりの火花（大）
	EFFECT_HIT_RING, ///< 当たった所から広がる白い輪
	EFFECT_HIT_RING_LARGE, ///< 強い当たりの大きな輪
	EFFECT_FINISH_STREAK, ///< 決め手で、吹っ飛ぶ向きへ伸びる光の筋
	EFFECT_GUARD_FLASH, ///< ガードで防いだときに広がって消える灰色の光
	EFFECT_PARRY_FLASH, ///< パリィしたときに広がって消える黄色い光
	EFFECT_ARM_DEBRIS, ///< 腕が壊れたときに飛び散って落ちる破片（箱）
	EFFECT_FLASH_WHITE, ///< 一瞬の白い光（腕が壊れたときなど）
	EFFECT_ARM_SMOKE, ///< 耐久が残りわずかな腕から出る細い煙
	EFFECT_SPARK_TINY, ///< ときどき出る小さな火花（耐久わずか・% が高い）
	EFFECT_PILLAR_WALL, ///< 横の壁（DEAD LINE）を越えた所に立つ光の柱（色はその人）
	EFFECT_PILLAR_FALL, ///< 下へ落ちて場外になった所から上へ伸びる光の柱（色はその人）
	EFFECT_PILLAR_RESPAWN, ///< 復活した所へ上から降りる光の柱（向きは下。色はその人）
	EFFECT_FEET_RING, ///< 足元に広がる輪（復活など）
	EFFECT_SPARKLE, ///< 体から散る小さな光の粒（無敵が切れた）
	EFFECT_SMOKE_GRAY, ///< 体から上がる灰色の煙（% が高い）
	EFFECT_SMOKE_BLACK, ///< 体から上がる黒い煙（% がとても高い）
	EFFECT_PARRY_SPARK, ///< パリィした所から大きく放射する黄色い火花
	EFFECT_DUST_RING, ///< 足元から横へ広がる土ぼこりの輪（地上のジャンプ・軽い着地）
	EFFECT_DUST_RING_LARGE, ///< 大きな土ぼこりの輪（重い着地・叩きつけ・衝撃）
	EFFECT_AIR_RING, ///< 空気を蹴った白い輪（空中ジャンプ・空中ダッシュ。向きは輪の面の向き）
	EFFECT_DUST_KICK, ///< 足元から向きへ飛ぶ土ぼこり（地上のダッシュの始まりなど）
	EFFECT_COUNT, ///< 型の数
};

/// @brief 画面に重ねる白い光の強さ（HUD が描く。見た目だけ）
struct SCREEN_FLASH
{
	float Edge = 0.0f; ///< 画面のふちを一瞬白く光らせる強さ（0〜1。強い当たり）
	float Full = 0.0f; ///< 画面全体を一瞬明るくする強さ（0〜1。パリィ）
};

/// @brief 火花・輪・光などのパーティクルを出して動かし、描く（見た目だけ。試合の乱数・通信の確かめには関わらない）
class EffectManager
{
public:
	/// @brief 粒を全部消し、専用の乱数を最初に戻す
	void Initialize();
	/// @brief 粒を全部消す（試合の始まり・やり直し）
	void Reset();
	/// @brief 何もしない（持っている資源はない）
	void Finalize();
	/// @brief 粒を1フレーム進める
	void Update();
	/// @brief 生きている粒を描く（3D描画の中、不透明な物のあとに呼ぶ）
	void Draw() const;
	/// @brief エフェクトを出す
	/// @param id 出し方の型
	/// @param position 出す位置
	/// @param direction 向き（火花の飛ぶ向き・輪の面の向き・筋や柱の向き。長さは問わない。0 なら上向き）
	/// @param tint 粒の色を変えるとき、その色（RGB だけ使い、不透明さは表のまま。nullptr なら表の色）
	void Spawn(EFFECT_ID id, const VEC3& position, const VEC3& direction, const PARTICLE_COLOR* tint = nullptr);

private:
	VEC3 RandomInCone(const VEC3& axis, float halfAngle);
	ParticleCalc m_Particles; ///< 出ている粒
	RandomCalc m_Random; ///< 粒のばらつき専用の乱数（試合の乱数とは別）
};
