#pragma once

#include "game/arm.h"
#include "physics/tentaclecalc.h"
#include "physics/tentacleropecalc.h"

/// @file tentacle.h
/// @brief 触腕（片手の腕で敵をつかんで、カメラの向きへ投げる）

/// @brief 片手の触腕の状態・モデルと、命中・保持・投げ・描画を管理する
class TentacleArm : public Arm
{
public:
	/// @brief 片手のモデルを読み、待機状態にする（ダミー初期化後）
	/// @param hand 左 0・右 1
	/// @param links 腕が使うゲームの部品への参照
	TentacleArm(int hand, const ARM_LINKS& links);
	/// @brief 保持している敵をダメージなしで離して終了する
	~TentacleArm() override;
	/// @brief カメラ更新後に入力・命中・保持・投げを更新する
	/// @param input このフレームの腕の入力（単押しで伸ばす・投げる）
	void Update(const ARM_INPUT& input) override;
	/// @brief OBJ をつないで触腕を描く（読み込み失敗時は球）
	void Draw() override;
	/// @brief つかんでいる敵をダメージなしで離し、待機状態に戻す
	void Reset() override;
	/// @brief 伸長・保持・収縮中なら true を返す
	bool IsActive() const override;
	/// @brief 保持中なら true を返す
	bool IsHolding() const override;

protected:
	/// @brief 付け根の箱の中央が体の取付口に来るよう、取付位置を上げる
	VEC3 GetMountOffset() const override;

private:
	VEC3 HoldPosition(float side, float forward, float height) const;
	void StepRope(const VEC3& origin, float idleLength, float modelScale);
	TentacleCalc m_Arm;
	int m_BaseModel = -1;
	int m_SegmentModel = -1;
	int m_TipModel = -1;
	TentacleRopeCalc m_Rope; // 付け根からぶら下がる節の鎖の物理（描画だけ）
	float m_RopeTime = -1.0f; // 鎖を最後に進めたゲームの経過秒（1フレームに1回だけ進める）
	float m_RopeScale = 0.0f; // 鎖を並べたときの大きさ（キャラが替わったら並べ直す）
};
