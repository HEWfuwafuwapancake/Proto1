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
	EFFECT_COUNT, ///< 型の数
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
	/// @param direction 向き（火花の飛ぶ向き・輪の面の向き・筋の向き。長さは問わない。0 なら上向き）
	void Spawn(EFFECT_ID id, const VEC3& position, const VEC3& direction);

private:
	VEC3 RandomInCone(const VEC3& axis, float halfAngle);
	ParticleCalc m_Particles; ///< 出ている粒
	RandomCalc m_Random; ///< 粒のばらつき専用の乱数（試合の乱数とは別）
};
