#pragma once

#include "physics/collision.h"
#include "physics/randomcalc.h"
#include "physics/stagecalc.h"
#include <array>

/// @brief 足場に置かれた腕カプセルの状態
struct ARM_CAPSULE_ITEM
{
	VEC3 Position; ///< 足場の上面にある配置点
	int Type;      ///< 袋から引いた腕の種類の番号
	bool Active;   ///< 拾えるカプセルが入っているか
};

/// @brief 腕カプセルの出現と取得の判定を管理する
class ArmCapsuleCalc
{
public:
	/// @brief 同時に置けるカプセルの最大数
	static constexpr int CAPACITY = 16;
	/// @brief 袋に入れる腕の種類の数
	static constexpr int TYPE_COUNT = 6;
	/// @brief 最初の個数と出現間隔を設定する（個数は0〜最大数、秒数は正の値。配置は次のStartで戻す）
	void SetAmount(int initialCount, double spawnSeconds);

	/// @brief 種とステージを受け取り、試合開始時のカプセルを配置する
	void Start(const STAGE& stage, uint32_t seed);
	/// @brief 試合の経過秒数 dt を足し、出現間隔ごとに空き枠へカプセルを置く
	void Step(const STAGE& stage, float dt);
	/// @brief 体と重なる有効なカプセルのうち最も若い枠番号を取得する（なければ -1）
	int FindTouching(const CAPSULE& body) const;
	/// @brief 拾われたカプセルの枠を空ける
	void Remove(int index);
	/// @brief 配置の一覧を取得する（Active の枠だけが拾える）
	const std::array<ARM_CAPSULE_ITEM, CAPACITY>& GetItems() const { return m_Items; }

private:
	void Spawn(const STAGE& stage);
	VEC3 ChoosePosition(const STAGE& stage);
	float NearestDistanceSq(const VEC3& position) const;
	int DrawType();

	std::array<ARM_CAPSULE_ITEM, CAPACITY> m_Items{};
	RandomCalc m_Random;
	int m_Bag[TYPE_COUNT]{};
	int m_NextType = TYPE_COUNT;
	double m_SpawnElapsed = 0.0;
	int m_InitialCount = 8;
	double m_SpawnSeconds = 1.5;
};
