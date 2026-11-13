#pragma once

#include "game/arm.h"
#include "physics/armphasecalc.h"
#include "physics/collision.h"
#include "physics/delayedhitcalc.h"
#include "physics/versuscalc.h"

/// @file arm_katana.h
/// @brief 片手の刀の入力・斬撃・狙った場所へのアサルトと描画

/// @brief 単押しですぐ斬り、長押しで構えて離すと空間を斬って一直線に飛び、着いた瞬間に居合の一閃（ダメージは少しあとから入る）を出す腕
class KatanaArm : public Arm
{
public:
	/// @brief モデルを読み、待機状態にする
	/// @param hand 左 0・右 1
	/// @param links 腕が使うゲームの部品への参照
	KatanaArm(int hand, const ARM_LINKS& links);
	/// @brief 斬撃とアサルトの段階を進める
	/// @param input このフレームの腕の入力（単押しで斬撃、長押しで構え、離すと空間を斬って飛ぶ）
	void Update(const ARM_INPUT& input) override;
	/// @brief 腕と拳、握った刀（構えの間は鞘も）、狙いの輪・裂け目・斬撃を描画する
	void Draw() override;
	/// @brief 待機へ戻し、待っている斬撃も消す（飛んでいる途中なら引っぱられ状態も解く）
	void Reset() override;
	/// @brief 動作をやめる（すでに当てた斬撃は残して進める）
	void Interrupt() override;
	/// @brief 待機以外なら true を返す
	bool IsActive() const override;
	/// @brief 遅れて効く斬撃や、その表示が残っていれば true を返す
	bool IsBusy() const override;
	/// @brief 待機中に単押しを受け付ける（GAME では前の居合が効くまで待つ）
	bool CanTakeTap() const override;
	/// @brief 単押しと同じ条件で長押しを受け付ける
	bool CanTakeHold() const override { return CanTakeTap(); }
	/// @brief 敵をつかまないので、常に false を返す
	bool IsHolding() const override;
	/// @brief 斬撃の間の歩く速さの倍率を取得する（空間を斬る間は止まる）
	float GetMoveScale() const override;
	/// @brief 飛んでいる間だけ全方向のガードを取得する
	GUARD GetGuard() const override;
	/// @brief 斬撃とアサルトの攻撃クリップを取得する
	ARM_BODY_CLIP GetBodyClip() const override;

private:
	enum BREACH_STATE { BREACH_NONE, BREACH_AIM, BREACH_CUT, BREACH_DASH, BREACH_RECOVER };
	void CancelMotion();
	void SetBreach(BREACH_STATE state);
	void StartIai();
	void BeginDraw();
	void SlashTargets();
	void IaiTargets();
	void DashTargets(const VEC3& from, const VEC3& to);
	void StepIai(float dt);
	void StepBreach(const ARM_INPUT& input, float dt);
	void StepDelayedHits(float dt);
	void UpdateLanding();
	void StartCut();
	void StartDash();
	void ReleasePull();
	void GetPose(VEC3& arm, VEC3& sword, bool& sheathed) const;
	void DrawRift(const VEC3& feet) const;
	void DrawCutRift() const;
	void DrawSlashes() const;
	ArmPhaseCalc m_Phase; // 単押しの斬撃の、振りかぶり → 斬る → 休み
	HitRecord m_Hits; // 1回の斬撃・一閃で当たった相手
	DelayedHitCalc m_Delayed; // 着いたときの一閃で当てて、あとから効かせる斬撃
	bool m_DashHitConsumed = false; // 飛行と着地を通して追加の耐久10を減らしたか
	bool m_IaiConsumed = false; // 今の動作で耐久を消費したか
	bool m_OwnPull = false; // true：この腕が体の計算を止めている（空中で斬っている間・飛んでいる間）
	VEC3 m_IaiDirection = { 0.0f, 0.0f, 1.0f }; // 着いたときの一閃の水平の向き。
	float m_SlashSeconds[FIGHTER_COUNT] = {}; // 相手ごとの、斬撃を見せる残りの秒数。
	VEC3 m_SlashDirections[FIGHTER_COUNT] = {}; // 相手ごとの、斬撃を見せる水平の向き。
	BREACH_STATE m_Breach = BREACH_NONE;
	float m_BreachTime = 0.0f; // アサルトの今の段階に入ってからの秒数。
	int m_ArmModel = -1;
	int m_FistModel = -1;
	int m_BladeModel = -1;
	int m_GripModel = -1;
	int m_SheathModel = -1;
	bool m_HasLanding = false; // 狙っている着地点があるか
	VEC3 m_Landing = {}; // 飛ぶ先の足元の位置。
	VEC3 m_Direction = { 0.0f, 0.0f, 1.0f }; // 斬る水平の向き。飛んでいる間は飛ぶ向き。
	float m_Straighten = 0.0f; // 描画だけの構え度（0：待機、1：動作の形）。
	float m_RiftSeconds = 0.0f; // 裂け目を見せる残りの秒数。
	VEC3 m_CutCenter = {}; // 空間を斬った裂け目の中心。
	VEC3 m_CutAxis = { 0.0f, 1.0f, 0.0f }; // 空間を斬った裂け目の向き（単位方向）。
	VEC3 m_RiftStart = {}; // 飛び始めの足元。
	VEC3 m_RiftEnd = {}; // 飛ぶ先の裂け目の足元。
};
