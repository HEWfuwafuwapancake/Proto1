#pragma once

/// @file particlecalc.h
/// @brief パーティクル（火花・土ぼこり・光など、小さな粒）の動きの計算
/// @details DirectX と Windows を使わないので、単体テスト（tests/Proto1Tests）からそのまま使える。
///          見た目だけなので、試合の乱数・通信の確かめには関わらない。

#include "physics/vec3.h"

/// @brief 粒の色（0〜1。A は不透明さ）
struct PARTICLE_COLOR
{
	float R; ///< 赤
	float G; ///< 緑
	float B; ///< 青
	float A; ///< 不透明さ（0 で見えない）
};

/// @brief 粒の形
enum PARTICLE_SHAPE
{
	PARTICLE_SHAPE_SPHERE, ///< 球
	PARTICLE_SHAPE_BOX, ///< 箱（破片など）
};

/// @brief 1つの粒
struct PARTICLE
{
	VEC3 Position = {}; ///< 今の位置
	VEC3 Velocity = {}; ///< 今の速さ（1秒あたりの移動量）
	float Gravity = 0.0f; ///< 1秒ごとに下向きの速さが増える量
	float Drag = 0.0f; ///< 1秒あたりに速さを減らす割合（0 で減らない）
	float Age = 0.0f; ///< 出てからの秒数
	float Life = 0.0f; ///< 消えるまでの秒数（0 以下の粒は使っていない枠）
	float StartSize = 0.0f; ///< 出たときの大きさ（球は半径、箱は1辺）
	float EndSize = 0.0f; ///< 消えるときの大きさ
	PARTICLE_COLOR StartColor = {}; ///< 出たときの色
	PARTICLE_COLOR EndColor = {}; ///< 消えるときの色
	PARTICLE_SHAPE Shape = PARTICLE_SHAPE_SPHERE; ///< 形
};

/// @brief 粒を決まった数まで持ち、動かして寿命で消す計算
class ParticleCalc
{
public:
	/// @brief 同時に持てる粒の数（超えたら古い粒から消す）
	static constexpr int MAX_PARTICLES = 512;

	/// @brief 粒を全部消す
	void Reset();
	/// @brief 粒を1つ出す（いっぱいなら、いちばん古く出した粒と入れ替える）
	/// @param particle 出す粒（Life が 0 以下なら出さない。Age は 0 にする）
	void Spawn(const PARTICLE& particle);
	/// @brief 全部の粒を進め、寿命が来た粒を消す
	/// @param dt 進める秒数
	void Step(float dt);
	/// @brief 枠の数（MAX_PARTICLES）を取得する
	int GetCapacity() const { return MAX_PARTICLES; }
	/// @brief 生きている粒の数を取得する
	int GetAliveCount() const;
	/// @brief 枠の粒を取得する（IsAlive で生きているか確かめてから使う）
	/// @param index 枠の番号（0〜MAX_PARTICLES-1）
	const PARTICLE& Get(int index) const { return m_Particles[index]; }
	/// @brief 枠の粒が生きているか取得する
	/// @param index 枠の番号（0〜MAX_PARTICLES-1）
	bool IsAlive(int index) const { return m_Particles[index].Life > 0.0f; }

private:
	PARTICLE m_Particles[MAX_PARTICLES]; ///< 粒の枠
	int m_Next = 0; ///< 次に使う枠（いちばん古く出した粒の枠）
};

/// @brief 粒の寿命の進み具合（0：出たとき、1：消えるとき）から、今の大きさを求める
float ParticleCalcSize(const PARTICLE& particle);
/// @brief 粒の寿命の進み具合から、今の色を求める
PARTICLE_COLOR ParticleCalcColor(const PARTICLE& particle);
