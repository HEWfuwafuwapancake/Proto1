#pragma once

#include "game/fighter.h"

/// @brief 4体の寿命・体当たり・攻撃と保持・KOの記録を管理する
class FighterManager
{
public:
	/// @brief 参加した機器を各スロットに割り当て、空きはダミーとして初期化する
	void Initialize(const Stage& stage, const VersusCalc& participants);
	/// @brief 参加者・KO数・順位の計算結果を読み取る
	const VersusCalc& GetVersus() const { return m_Versus; }
	/// @brief 人のスロットにカメラと腕をつなぐ（どちらのResetより前でもよい）
	void BindPlayer(int slot, const Camera& camera, const ArmManager& arms);
	/// @brief 全員の体・攻撃記録・得点を戻す（割り当てはそのまま）
	void Reset();
	/// @brief 全員の体を更新し、場外の得点を記録してから体当たりを解く
	void Update();
	/// @brief カメラ更新後、人の向きを更新する
	void UpdateFacing();
	/// @brief 全員の体を描く（targetは強調するスロット、なしは-1）
	void Draw(int target) const;
	/// @brief 体への参照を取得する（slotは0〜3）
	Fighter& GetFighter(int slot) { return m_Fighters[slot]; }
	/// @brief 読み取り専用の体への参照を取得する（slotは0〜3）
	const Fighter& GetFighter(int slot) const { return m_Fighters[slot]; }
	/// @brief 相手の体とつかめる状態を取得する（自分・無効・復活待ちは空の候補）
	/// @param index 相手のスロット番号（0〜3。全APIで同じ番号を使う）
	/// @param self 候補を読む自分のスロット番号（0〜3）
	TENTACLE_TARGET GetTarget(int index, int self) const;
	/// @brief 体が生存しているか取得する（無効な番号はfalse）
	bool IsActive(int index) const;
	/// @brief ダメージ%を取得する（無効な番号は0）
	float GetPercent(int index) const;
	/// @brief KO数を取得する（無効な番号は0）
	int GetScore(int slot) const { return m_Versus.GetScore(slot); }
	/// @brief 自分以外の生存中の体をつかむ（保持中ならfalse）。成功時に攻撃者も記録する
	bool Grab(int index, int attacker);
	/// @brief 指定したスロットが相手を保持しているか取得する
	bool IsHeldBy(int index, int holder) const;
	/// @brief 自分が保持中の体の中心位置を設定する
	void MoveHeld(int index, int holder, const VEC3& center);
	/// @brief 自分の保持を解除する
	/// @param throwTarget 投げを設定する（true：ダメージを足して指定した3D方向へ投げる、false：その場で離す）
	void Release(int index, int holder, bool throwTarget, const VEC3& direction, float damage, float power);
	/// @brief 攻撃者を記録し、ダメージを足して固定角で吹っ飛ばす（自分・無効・復活待ちは無視）
	void Hit(int index, int attacker, const XMFLOAT3& direction, float damage, float power);
	/// @brief 攻撃者を記録し、ダメージ%だけを足す（速度はそのまま）
	void AddDamage(int index, int attacker, float damage);
	/// @brief 攻撃者を記録し、ダメージを足して指定した3D方向へ吹っ飛ばす
	void Launch(int index, int attacker, const VEC3& direction, float damage, float power);
	/// @brief 保持されていない体を指定速度で押し出し、攻撃者を記録する（%は変えない）
	void Push(int index, int attacker, const VEC3& velocity);

private:
	bool CanAttack(int index, int attacker) const;
	void PushBodies();
	Fighter m_Fighters[FIGHTER_COUNT];
	VersusCalc m_Versus;
	const Camera* m_Cameras[FIGHTER_COUNT] = {};
	const ArmManager* m_Arms[FIGHTER_COUNT] = {};
};
