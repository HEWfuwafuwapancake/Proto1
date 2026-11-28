#pragma once

#include "game/fighter.h"
#include "game/effect.h"
#include "physics/scenecalc.h"
#include "physics/spawncalc.h"
#include "physics/escapecalc.h"
#include "physics/tuningcalc.h"

/// @brief 4体の寿命・体当たり・攻撃と保持・KOの記録を管理する
class FighterManager
{
public:
	/// @brief キャラごとの描画モデルを読み込む（起動時に1回）
	void LoadModels();
	/// @brief キャラごとの描画モデルを解放する
	void Finalize();
	/// @brief 参加した機器を各スロットに割り当て、空きはダミーとして初期化する
	/// @param stage 浮島と場外の線を読むステージ
	/// @param participants 参加者
	/// @param effects 当たりの火花などを出すエフェクトの管理（腕からも SpawnEffect で使う）
	void Initialize(const Stage& stage, const VersusCalc& participants, EffectManager& effects);
	/// @brief 参加者・KO数・順位の計算結果を読み取る
	const VersusCalc& GetVersus() const { return m_Versus; }
	/// @brief 人のスロットにカメラと腕をつなぐ（どちらのResetより前でもよい）
	void BindPlayer(int slot, const Camera& camera, const ArmManager& arms);
	/// @brief 試合のルール・復活先を選ぶ乱数の種・攻撃と配置の設定を渡す
	void SetMatchSettings(GAME_RULE rule, uint32_t seed, const MATCH_TUNING& tuning);
	/// @brief 全員の体・攻撃記録・得点を戻す（割り当てはそのまま）
	void Reset();
	/// @brief 全員の体を更新し、場外の得点を記録してから体当たりを解く
	void Update();
	/// @brief カメラ更新後、人の向きを更新する
	void UpdateFacing();
	/// @brief 左右の腕の処理後、全員の描画専用のアニメを更新する
	void UpdateAnimation();
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
	/// @brief 近接攻撃で当てられる相手の体を取得する（つかまれている相手にも当たる。自分・無効・復活待ちは空の候補）
	/// @param index 相手のスロット番号（0〜3）
	/// @param self 候補を読む自分のスロット番号（0〜3）
	TENTACLE_TARGET GetMeleeTarget(int index, int self) const;
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
	/// @brief 自分のダメージ%を減らす（無効な番号・復活待ちは無視。リペア）
	void Heal(int index, float amount);
	/// @brief 攻撃者を記録し、ダメージ%だけを足す（速度はそのまま）
	/// @return 相手に効いたら true（ガードされた場合などは false）
	bool AddDamage(int index, int attacker, float damage);
	/// @brief 攻撃者を記録し、ダメージを足して指定した3D方向へ吹っ飛ばす（つかまれている相手はダメージだけ足し、つかまれたままにする）
	/// @return 相手に効いたら true（ガードされた場合などは false）
	bool Launch(int index, int attacker, const VEC3& direction, float damage, float power);
	/// @brief 状態異常をかけて攻撃者を記録する（隠れるだけは自分にもかけられ、攻撃者を記録しない）
	/// @return かけられたら true
	bool ApplyStatus(int index, int attacker, STATUS_TYPE type, float seconds, float value);
	/// @brief 地面に残る攻撃（トゲの茨）の効き目を、ガードを通さずにかける（毎フレーム呼ぶのでガードの耐久は減らさない）
	/// @param index 相手のスロット番号
	/// @param attacker 置いた人のスロット番号
	/// @param type かける状態異常
	/// @param seconds 状態異常の続く秒数
	/// @param value 状態異常の値（茨なら歩く速さの倍率）
	/// @param damage 足すダメージ%（0 以下なら足さない）
	/// @return かけられたら true（相手でない・復活待ちなら false）
	bool ApplyHazard(int index, int attacker, STATUS_TYPE type, float seconds, float value, float damage);
	/// @brief 相手が攻撃した人の側をガードしているか取得する（攻撃が来た位置は攻撃した人の体の中心とみなす）
	bool IsGuarded(int index, int attacker) const;
	/// @brief 前に読んでからガードで攻撃を防いだかを取得し、記録を消す（刀の居合の返し）
	/// @param index スロット番号（0〜3）
	/// @return 防いでいたら true
	bool ConsumeGuardedHit(int index);
	/// @brief 最後にガードで防いだ攻撃をした人のスロット番号を取得する（ガードのヒットストップに使う）
	/// @param index 防いだ人のスロット番号（0〜3）
	/// @return 攻撃した人のスロット番号（なし・無効な番号は -1）
	int GetGuardedAttacker(int index) const;
	/// @brief ヒットストップを始める（無効な番号・復活待ちは無視）
	/// @param index スロット番号（0〜3）
	/// @param frames 止めるフレーム数
	/// @param shake 止まっている間、体を震わせるか（true：当てられた人、false：当てた人・ガード）
	void StartHitstop(int index, int frames, bool shake);
	/// @brief その人の画面の揺れ（画ぶれ）を足す（Game が TakeShake でカメラへ渡す。描画だけ）
	/// @param index スロット番号（0〜3。無効な番号は無視）
	/// @param strength 足す強さ（0〜1）
	void AddShake(int index, float strength);
	/// @brief 足された画面の揺れを取り出し、記録を消す
	/// @param index スロット番号（0〜3）
	/// @return 取り出した強さ（なし・無効な番号は 0）
	float TakeShake(int index);
	/// @brief その人の画面に重ねる白い光を足す（Game が TakeScreenFlash で HUD へ渡す。描画だけ）
	/// @param index スロット番号（0〜3。無効な番号は無視）
	/// @param flash 足す強さ（ふち・全体。それぞれ上限 1）
	void AddScreenFlash(int index, const SCREEN_FLASH& flash);
	/// @brief 足された画面の光を取り出し、記録を消す
	/// @param index スロット番号（0〜3）
	/// @return 取り出した強さ（なし・無効な番号は 0）
	SCREEN_FLASH TakeScreenFlash(int index);
	/// @brief 足元から、背の高さの割合だけ上の点を取得する（エフェクトを出す場所に使う）
	/// @param index スロット番号（0〜3）
	/// @param heightRate 背の高さに対する割合（0：足元、1：頭のてっぺん）
	VEC3 GetBodyPoint(int index, float heightRate) const;
	/// @brief エフェクトを出す（見た目だけ。EffectManager::Spawn と同じ）
	/// @param id 出し方の型
	/// @param position 出す位置
	/// @param direction 向き（火花の飛ぶ向きなど）
	/// @param tint 粒の色を変えるとき、その色（nullptr なら表の色）
	void SpawnEffect(EFFECT_ID id, const VEC3& position, const VEC3& direction, const PARTICLE_COLOR* tint = nullptr);
	/// @brief ガードを無視してしびれとダメージ%を与え、攻撃者を記録する（しびれで腕が戻り、ガードが解ける。アックス）
	/// @return 与えられたら true
	bool BreakGuard(int index, int attacker, float seconds, float damage);
	/// @brief ほかの人からロックオンされない（隠れている）か取得する（無効な番号はfalse）
	bool IsHidden(int index) const;
	/// @brief 保持されていない体を指定速度で押し出し、攻撃者を記録する（%は変えない）
	void Push(int index, int attacker, const VEC3& velocity);

private:
	bool CanAttack(int index, int attacker);
	bool m_GuardedHit[FIGHTER_COUNT] = {}; // ガードで攻撃を防いだか（ConsumeGuardedHit で読むまで残す）
	int m_GuardedBy[FIGHTER_COUNT] = { -1, -1, -1, -1 }; // 最後にガードで防いだ攻撃をした人（なしは -1）
	EffectManager* m_Effects = nullptr; // 当たりの火花などを出すエフェクトの管理（Game が持つ）
	float m_SmokeSeconds[FIGHTER_COUNT] = {}; // % の煙を次に出すまでの秒数（見た目だけ）
	int m_SmokeCount[FIGHTER_COUNT] = {}; // % の煙を出した回数（何回かに1回火花も出す）
	SCREEN_FLASH m_PendingFlash[FIGHTER_COUNT] = {}; // HUD へ渡す前の画面の光（描画だけ）
	float m_PendingShake[FIGHTER_COUNT] = {}; // カメラへ渡す前の画面の揺れ（描画だけ。通信の確かめに入れない）
	void PushBodies();
	void RespawnAtSafePoint(int slot);
	void UpdateEscape();
	void SpawnHitEffects(int index, int attacker, const VEC3& direction, int frames);
	void UpdateDamageEffects(int index, float dt); // % が高い人から煙と火花を出す（見た目だけ）
	void SpawnMoveEffects(int index); // 跳んだ・着地した・ダッシュした瞬間の土ぼこりや輪（見た目だけ）
	PARTICLE_COLOR GetEffectColor(int index) const; // その人の色（光の柱などに使う）
	EscapeCalc m_Escapes[FIGHTER_COUNT];
	SkinnedModel m_Models[CHARACTER_COUNT];
	Fighter m_Fighters[FIGHTER_COUNT];
	VersusCalc m_Versus;
	const Stage* m_Stage = nullptr;
	GAME_RULE m_Rule = GAME_RULE_DEBUG;
	float m_MapScale = 1.0f;
	SpawnCalc m_Spawn;
	const Camera* m_Cameras[FIGHTER_COUNT] = {};
	const ArmManager* m_Arms[FIGHTER_COUNT] = {};
};
