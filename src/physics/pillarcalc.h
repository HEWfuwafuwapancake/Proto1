#pragma once

#include <cstdint>
#include "physics/collision.h"
#include "physics/stagecalc.h"
#include "physics/versuscalc.h"

/// @file pillarcalc.h
/// @brief 攻撃で倒れる柱（コロシアム。仕様書 docs/plan/09_追加マップ3面/02_コロシアム.txt の「柱の倒壊」）
/// @details 柱はステージの区画（STAGE_SURFACE_PILLAR）で、倒れ方に合わせて区画を書き換える。
///          試合の更新の数で進めるので、通信でも全員同じに倒れる。DirectX を使わないので単体テストできる

/// @brief 1つのステージに置ける柱の最大数
static constexpr int PILLAR_MAX = 8;
/// @brief 柱に攻撃を当てる元の数（スロット × 左右の手。同じ元からは続けて当たらない）
static constexpr int PILLAR_SOURCE_MAX = FIGHTER_COUNT * 2;

/// @brief 柱の状態
enum PILLAR_STATE : uint8_t
{
	PILLAR_STANDING,	///< 立っている（攻撃を受けると耐久が減る）
	PILLAR_WARNING,		///< 耐久が尽き、倒れる前に揺れている（予兆）
	PILLAR_FALLING,		///< 倒れている途中（通った所の体に当たる）
	PILLAR_FALLEN,		///< 倒れ終わり、横たわった遮蔽物になった
};

/// @brief 倒れる柱が体に当たったこと（当てる処理は呼んだ側がする）
struct PILLAR_IMPACT
{
	int Pillar;		///< 柱の番号
	int Victim;		///< 当たった人のスロット番号
	int Attacker;	///< 柱に最後に攻撃を当てた人のスロット番号（いなければ -1）
	VEC3 Direction;	///< 倒れる水平の向き（長さ1）
};

/// @brief 柱を描くための姿勢
struct PILLAR_POSE
{
	VEC3 Base;		///< 柱の根元の中心（倒れると、横たわった柱の太さの半分だけ浮く）
	VEC3 Axis;		///< 根元から先へ向かう柱の軸（長さ1。立っていれば真上）
	VEC3 Front;		///< 倒れる向きの側の、軸に直角な向き（長さ1）
	VEC3 Side;		///< 倒れる向きの横の向き（長さ1。水平）
	float Damage;	///< 傷みの割合（0：無傷、1：耐久が尽きた）
};

/// @brief ステージの柱を、攻撃で倒して区画に反映する
class PillarCalc
{
public:
	/// @brief ステージの柱の区画を覚え、全部立った状態に戻す（マップを作り直すたびに呼ぶ）
	/// @param stage 柱の区画（STAGE_SURFACE_PILLAR）を持つステージ（柱の根元は stage.TopY の高さとみなす）
	void Setup(const STAGE& stage);
	/// @brief 柱の数を取得する
	int GetCount() const { return m_Count; }
	/// @brief 攻撃を当てられる（立っている）柱の当たりを取得する
	/// @param index 柱の番号
	/// @param body 立っていれば柱の軸に沿ったカプセルを書き込む先
	/// @return 立っていれば true（予兆・倒れ中・倒れたあとは false）
	bool GetBody(int index, CAPSULE& body) const;
	/// @brief 柱に攻撃を当てる（同じ元からは、少しの間（pillarcalc.cpp の HIT_INTERVAL）は当たらない）
	/// @param index 柱の番号
	/// @param source 当てた元（スロット × 2 ＋ 手。0〜PILLAR_SOURCE_MAX − 1）
	/// @param attacker 当てた人のスロット番号
	/// @param damage 減らす耐久
	/// @param direction 攻撃の進む向き（水平にして倒れる向きにする。水平の長さがなければ +Z）
	/// @return 耐久を減らしたら true
	bool Hit(int index, int source, int attacker, float damage, const VEC3& direction);
	/// @brief 予兆と倒れる動きを進め、倒れる柱が通った所の体を探し、区画を今の状態に合わせる
	/// @param dt 進める秒数
	/// @param bodies 全員の体
	/// @param hittable 体ごとに、当たる状態かを設定する（true：当たる、false：無敵・つかまれ中・復活待ちなどで当たらない）
	/// @param stage 柱の区画を書き換えるステージ
	/// @param impacts 当たったことを書き込む先
	/// @param maxImpacts 書き込める数
	/// @return 書き込んだ数（1本の柱は同じ人に1回だけ当たる）
	int Step(float dt, const CAPSULE (&bodies)[FIGHTER_COUNT], const bool (&hittable)[FIGHTER_COUNT], STAGE& stage,
		PILLAR_IMPACT impacts[], int maxImpacts);
	/// @brief 柱の状態を取得する
	PILLAR_STATE GetState(int index) const { return m_Pillars[index].State; }
	/// @brief 柱を描くための姿勢を取得する
	PILLAR_POSE GetPose(int index) const;

private:
	/// @brief 柱1本の状態
	struct PILLAR
	{
		int Region = -1;			// ステージの区画の番号
		STAGE_REGION Home{};		// 立っているときの区画
		float Durability = 0.0f;	// 残りの耐久
		PILLAR_STATE State = PILLAR_STANDING;
		float Timer = 0.0f;			// 予兆の残り秒数
		float Fall = 0.0f;			// 倒れ方（0：立っている、1：倒れ終わり）
		VEC3 Direction{ 0.0f, 0.0f, 1.0f };	// 倒れる水平の向き
		int Attacker = -1;			// 最後に攻撃を当てた人
		uint32_t HitMask = 0;		// 倒れる柱が当たった人のビット
		float LastHit[PILLAR_SOURCE_MAX] = {};	// 元ごとに最後に当たった時刻
	};
	VEC3 GetBase(const PILLAR& pillar, float angle) const;
	STAGE_REGION GetRegion(const PILLAR& pillar) const;

	PILLAR m_Pillars[PILLAR_MAX];
	int m_Count = 0;
	float m_BaseY = 0.0f;	// 柱の根元の高さ
	float m_Clock = 0.0f;	// Setup からの秒数（同じ元から続けて当てない時刻に使う）
};
