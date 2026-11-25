#pragma once

#include "physics/stagecalc.h"
#include "physics/mapcalc.h"


/// @file stage.h
/// @brief ステージ（浮島と高台）。場外の線は下だけ
/// @details 浮島の大きさ・高台の配置・場外の下の線は stage.cpp の「調整用の値」にある。計算は physics/stagecalc.h

/// @brief 浮島・高台・場外の線を管理する
class Stage
{
public:

	/// @brief 浮島・高台・場外の線を「調整用の値」から決める（ShapeInitialize のあと、FighterManager::Initialize の前に呼ぶ）
	void Initialize(void);

	/// @brief 状態を最初に戻す（参照はそのまま）
	void Reset(void);
	/// @brief 浮島の幅・奥行きと高台の配置を元の値から指定倍率にする（高さと高台の大きさはそのまま）
	void SetSizeScale(float scale);
	/// @brief 試合で使うマップを、指定の広さの倍率で作り直す（浮島は SetSizeScale と同じ）
	/// @param map マップ
	/// @param scale マップの広さの倍率
	void SetMap(STAGE_MAP map, float scale);
	/// @brief 今のマップを取得する
	STAGE_MAP GetMap() const { return m_Map; }

	/// @brief ステージの終了処理（今は何もしない）
	void Finalize(void);

	/// @brief ステージを更新する（毎フレーム呼ぶ。今は何もしない。崩れる・縮むなどは入れないと決まっている）
	void Update(void);

	/// @brief 浮島と高台、または対戦マップの地形を描く（render/shape.h の箱を使う）
	void Draw(void) const;

	/// @brief 浮島・高台・場外の線を取得する（着地や場外の判定に使う）
	/// @return ステージ
	const STAGE& GetStage(void) const;

private:
	void DrawTerrain(void) const;
	void DrawRegion(const STAGE_REGION& region) const;
	float GetSolidDrawBottom(int index) const;	// 実体の区画を描く下端の高さ（下に床があればその上面か穴の底、なければ地盤の下面）
	float GetHoleBottom(int floorIndex) const;	// 床に開ける穴の底の高さ（穴がなければ床の上面）
	static float GetRegionLow(const STAGE_REGION& region);	// 区画の上面のいちばん低い高さ
	static bool IsHole(const STAGE_REGION& region, const STAGE_REGION& floor);	// 区画が床の穴になるか（重なっていて、床より低い所がある）
	void DrawMarks(void) const;

	STAGE m_Stage = {};		// 浮島・高台・場外の線
	STAGE_MAP m_Map = STAGE_MAP_ISLAND;	// 今のマップ
	float m_Scale = 1.0f;	// 今のマップの広さの倍率
};
