#pragma once

#include "core/dx11.h"
#include "physics/tentaclecalc.h"
#include "physics/playercalc.h"
#include "physics/dummycalc.h"
#include "physics/knockbackcalc.h"

class Stage;
class Player;
class Camera;

/// @file dummy.h
/// @brief ダミーの敵（その場に立つだけのカプセルが3体。動かない・攻撃しない）
/// @details プレイヤーがぶつかると押される。攻撃されるとダメージ%が増え、% が高いほど遠くへ吹っ飛ぶ（DummyManager::Hit）。
///          場外に落ちたら、しばらく消えてから最初の場所に 0% で戻る。
///          置き場所・復活までの秒数・形・吹っ飛びは dummy.cpp の「調整用の値」、計算は physics/dummycalc.h・physics/knockbackcalc.h

/// @brief ダミーの数
static constexpr int DUMMY_COUNT = 3;

/// @brief ダミー全員の状態・命中・保持・描画を管理する
class DummyManager
{
public:


	/// @brief ダミーを最初の場所に立たせる（Stage::Initialize のあとに呼ぶ）
	void Initialize(const Stage& stage, Player& player, const Camera& camera);

	/// @brief 状態を最初に戻す（参照はそのまま）
	void Reset(void);

	/// @brief ダミーの終了処理（今は何もしない）
	void Finalize(void);

	/// @brief ダミーを1フレーム分動かす（重力・着地・プレイヤーに押される・吹っ飛ばされている間の動き・場外と復活）
	/// @details 毎フレーム、Player::Update のあとに呼ぶ（動いたあとのプレイヤーに押される）
	void Update(void);

	/// @brief ダミーを描く（復活を待っている間は描かない）
	void Draw(void) const;

	/// @brief ダミーを攻撃する（ダメージ%を足して、% に応じた速さで吹っ飛ばす）
	/// @details スマブラと同じく、ダメージを足してから、足したあとの % で吹っ飛ぶ速さを決める。
	///          吹っ飛ぶ向きは攻撃の向き（横）に少し上向きの角度をつけたもの。復活を待っているダミーには当たらない
	/// @param index ダミーの番号（0〜DUMMY_COUNT-1）
	/// @param direction 攻撃の向き（攻撃した側から、ダミーへ。上下の向きと長さは使わない）
	/// @param damage ダメージ（%）
	/// @param power 攻撃の強さ（1 でふつう。大きいほど速く飛ぶ）
	void Hit(int index, const XMFLOAT3& direction, float damage, float power);

	/// @brief ダメージ%だけを足す（速度・保持状態は変えない。無効な番号・復活待ちは何もしない）
	/// @param index ダミーの番号（0〜DUMMY_COUNT-1）
	/// @param damage 足すダメージ%（負なら 0）
	void AddDamage(int index, float damage);

	/// @brief ダメージを足し、保持を解除して上下を含む指定方向へ吹っ飛ばす（無効な番号・復活待ちは何もしない）
	/// @param index ダミーの番号（0〜DUMMY_COUNT-1）
	/// @param direction 吹っ飛ばす向き（正規化して使う。ゼロなら真上）
	/// @param damage 足すダメージ%（負なら 0）
	/// @param power 攻撃の強さ（DummyManager::Release と同じ速さの計算）
	void Launch(int index, const VEC3& direction, float damage, float power);

	/// @brief 敵の体とつかめる状態を取得する（無効な番号なら空の候補）
	TENTACLE_TARGET GetTarget(int index) const;
	/// @brief 敵が生存しているか取得する
	bool IsActive(int index) const;
	/// @brief 敵をつかむ。ほかの腕が保持中、または消えていれば false
	bool Grab(int index);
	/// @brief 保持中の敵の中心位置を設定する
	void MoveHeld(int index, const VEC3& center);
	/// @brief 保持を解除し、投げるかを設定する（true：指定した3D方向へ投げる、false：ダメージなしで離す）
	void Release(int index, bool throwTarget, const VEC3& direction, float damage, float power);

	/// @brief ダミーのダメージ%を取得する（画面表示に使う）
	/// @param index ダミーの番号（0〜DUMMY_COUNT-1）
	/// @return ダメージ%（無効な番号なら 0）
	float GetPercent(int index) const;

	/// @brief ダミー全員が場外に落ちた回数の合計を取得する（No.14 のスコアに使う）
	/// @return 落ちた回数の合計
	int GetTotalOutCount(void) const;

private:
	KNOCKBACK_PARAMS KnockbackParams(void) const;
	void PlaceAtSpawn(int index);
	/// @brief ダミー1体の状態（管理クラスだけが書き換える）
	struct DUMMY
	{
		PlayerBodyCalc Body;	///< 位置・速度・立っているか（立っている間はプレイヤーと同じ計算で動かす）
		DummyLifeCalc Life;	///< 場外に落ちて復活する流れ
		float Percent;		///< ダメージ%（復活したら 0 に戻る）
		bool Launched;		///< 吹っ飛ばされているか（自分では止まれない。浮島の上で止まったら false）
		bool Held;          ///< 触腕が保持しているか
	};
	DUMMY m_Dummies[DUMMY_COUNT] = {};
	const Stage* m_Stage = nullptr;
	Player* m_Player = nullptr;
	const Camera* m_Camera = nullptr;
};
