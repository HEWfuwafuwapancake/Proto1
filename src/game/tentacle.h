#pragma once

#include "game/arm.h"
#include "physics/tentaclecalc.h"

/// @file tentacle.h
/// @brief 触腕（片手の腕で敵をつかんで、カメラの向きへ投げる）

/// @brief 片手の触腕の状態・モデルと、命中・保持・投げ・描画を管理する
class TentacleArm : public Arm
{
public:
	/// @brief 片手のモデルを読み、待機状態にする（ダミー初期化後）
	/// @param hand 左 0・右 1
	explicit TentacleArm(int hand);
	/// @brief 保持している敵をダメージなしで離して終了する
	~TentacleArm() override;
	/// @brief カメラ更新後に入力・命中・保持・投げを更新する
	/// @param trigger 入力を設定する（true：押した瞬間、false：それ以外）
	void Update(bool trigger) override;
	/// @brief OBJ をつないで触腕を描く（読み込み失敗時は球）
	void Draw() override;
	/// @brief つかんでいる敵をダメージなしで離し、待機状態に戻す
	void Reset() override;
	/// @brief 伸長・保持・収縮中なら true を返す
	bool IsActive() const override;
	/// @brief 保持中なら true を返す
	bool IsHolding() const override;

private:
	TentacleCalc m_Arm;
	int m_BaseModel = -1;
	int m_SegmentModel = -1;
	int m_TipModel = -1;
};
