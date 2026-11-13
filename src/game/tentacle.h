#pragma once

#include "game/arm.h"
#include "physics/armphasecalc.h"
#include "physics/tentaclecalc.h"
#include "physics/tentacleropecalc.h"

/// @file tentacle.h
/// @brief 触手（単押しで敵をつかんで投げる・足場へ飛ぶ。長押しで周囲を1回転）

/// @brief 片手の触手の状態・モデルと、命中・保持・投げ・足場への引っぱり・回転・描画を管理する
class TentacleArm : public Arm
{
public:
	/// @brief 片手のモデルを読み、待機状態にする（ダミー初期化後）
	/// @param hand 左 0・右 1
	/// @param links 腕が使うゲームの部品への参照
	TentacleArm(int hand, const ARM_LINKS& links);
	/// @brief 保持している敵をダメージなしで離して終了する
	~TentacleArm() override;
	/// @brief 左右の腕を接続し、足場への引っぱりを後から付いた手へ渡せるようにする（起動時に1回）
	/// @param other 反対の手の触手
	void ConnectOtherArm(TentacleArm& other);
	/// @brief カメラ更新後に入力・命中・保持・投げ・引っぱり・回転を更新する
	/// @param input このフレームの腕の入力（単押しで伸ばす・投げる、長押しで回転する）
	void Update(const ARM_INPUT& input) override;
	/// @brief OBJ をつないで触腕を描く（読み込み失敗時は球）
	void Draw() override;
	/// @brief つかんでいる敵をダメージなしで離し、引っぱりと回転もやめて待機状態に戻す
	void Reset() override;
	/// @brief 伸長・保持・収縮・引っぱり・回転中なら true を返す
	bool IsActive() const override;
	/// @brief 保持中なら true を返す
	bool IsHolding() const override;
	/// @brief 腕を戻している間と回転中を除いて単押しを受け付ける（つかんでいる間は投げ、引っぱり中は取りやめになる）
	bool CanTakeTap() const override;
	/// @brief 腕が待機で回転もしていないなら true を返す
	bool CanTakeHold() const override;

protected:
	/// @brief 付け根の箱の中央が体の取付口に来るよう、取付位置を上げる
	VEC3 GetMountOffset() const override;

private:
	VEC3 HoldPosition(float side, float forward, float height) const;
	void StepRope(const VEC3& origin, float idleLength, float modelScale);
	bool CanAnchor() const;
	void StartReach(const TENTACLE_TARGET* targets, const VEC3& origin, const VEC3& cameraForward, VEC3& aim, STAGE_GRIP& grip, bool& hasGrip) const;
	void StopPulling();
	void StepPull(const TENTACLE_EVENT& event, float dt);
	void StepSpin(const ARM_INPUT& input, float dt);
	VEC3 GetSpinTip() const;
	ArmPhaseCalc m_Spin; // 長押しの溜め → 回転 → 休み
	HitRecord m_SpinHits; // 1回の回転で当たった相手
	VEC3 m_SpinForward = { 0.0f, 0.0f, 1.0f }; // 回転を始めた向き
	float m_SpinAngle = 0.0f; // 回転し終わった角度（ラジアン）
	bool m_ReachPending = false; // 単押しの結果が未確定ならtrue
	bool m_SpinConsumed = false; // 今の回転で耐久を消費したか
	TentacleArm* m_OtherArm = nullptr; // 反対の手の触手（起動時につなぐ）
	bool m_Pulling = false; // 左右のうち、後から付いて引っぱりを受け持つ手だけ true
	TentacleCalc m_Arm;
	int m_BaseModel = -1;
	int m_SegmentModel = -1;
	int m_TipModel = -1;
	TentacleRopeCalc m_Rope; // 付け根からぶら下がる節の鎖の物理（描画だけ）
	float m_RopeTime = -1.0f; // 鎖を最後に進めたゲームの経過秒（1フレームに1回だけ進める）
	float m_RopeScale = 0.0f; // 鎖を並べたときの大きさ（キャラが替わったら並べ直す）
};
