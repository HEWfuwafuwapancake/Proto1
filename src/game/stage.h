#pragma once

#include "physics/stagecalc.h"
#include "physics/mapcalc.h"
#include "physics/pillarcalc.h"


/// @file stage.h
/// @brief ステージ（浮島と高台・対戦マップの地形）。場外の線は下だけ
/// @details 浮島の大きさ・高台の配置・場外の下の線は stage.cpp の「調整用の値」にある。計算は physics/stagecalc.h・mapcalc.h。
///          ネオンビル群・コロシアム・廃駅は、見た目を assets/models/maps/ のモデル（頂点の色にライトを焼き込んだ GLB）で描く

/// @brief 浮島・高台・場外の線と、対戦マップの地形・倒れる柱を管理する
class Stage
{
public:

	/// @brief 浮島・高台・場外の線を「調整用の値」から決め、マップのモデルを読む（ShapeInitialize・ModelInitialize のあと、FighterManager::Initialize の前に呼ぶ）
	void Initialize(void);

	/// @brief 状態を最初に戻す（浮島にし、空の色も浮島の色にする）
	void Reset(void);
	/// @brief 標準の広さの浮島と高台のステージを作る（マップ選択の上から見た図にも使う）
	/// @return 浮島のステージ
	static STAGE MakeIsland(void);
	/// @brief 浮島の幅・奥行きと高台の配置を元の値から指定倍率にする（高さと高台の大きさはそのまま）
	void SetSizeScale(float scale);
	/// @brief 試合で使うマップを、指定の広さの倍率で作り直す（浮島は SetSizeScale と同じ）
	/// @param map マップ
	/// @param scale マップの広さの倍率
	/// @param ringOutDistance 外周から横の場外の壁までの距離（0 より小さければ横の壁なし）
	void SetMap(STAGE_MAP map, float scale, float ringOutDistance);
	/// @brief 今のマップを取得する
	STAGE_MAP GetMap() const { return m_Map; }

	/// @brief ステージの終了処理（今は何もしない）
	void Finalize(void);

	/// @brief ステージを更新する（毎フレーム呼ぶ。今は何もしない。崩れる・縮むなどは入れないと決まっている）
	void Update(void);

	/// @brief 浮島と高台、または対戦マップの地形を描く（render/shape.h の箱を使う）
	void Draw(void) const;
	/// @brief 景観のモデルで描くマップ（ネオンビル群・コロシアム・廃駅）か取得する（true：モデルで描く、false：区画の箱で描く）
	bool HasSceneryModel(void) const { return m_MapModels[m_Map] >= 0; }
	/// @brief 地形の区画（柱も含む）を、色を変えずに深度だけ書く（景観のモデルの深度を消したあと、地形の前後の判定を作り直すため）
	void DrawTerrainDepth(void) const;

	/// @brief 浮島・高台・場外の線を取得する（着地や場外の判定に使う）
	/// @return ステージ
	const STAGE& GetStage(void) const;

	/// @brief 倒れる柱（コロシアム）を取得する（攻撃を当てられる柱の当たりを読む）
	const PillarCalc& GetPillars() const { return m_Pillars; }
	/// @brief 柱に攻撃を当てる（耐久が尽きると、攻撃の進む向きへ倒れ始める）
	/// @param index 柱の番号
	/// @param source 当てた元（スロット × 2 ＋ 手。同じ元からは続けて当たらない）
	/// @param attacker 当てた人のスロット番号
	/// @param damage 減らす耐久
	/// @param direction 攻撃の進む向き
	void HitPillar(int index, int source, int attacker, float damage, const VEC3& direction);
	/// @brief 柱の予兆と倒れる動きを進め、地形の区画を今の柱に合わせる（試合中の体の更新のあとに呼ぶ）
	/// @param bodies 全員の体
	/// @param hittable 体ごとに、倒れる柱が当たる状態かを設定する（true：当たる、false：当たらない）
	/// @param impacts 倒れる柱が体に当たったことを書き込む先
	/// @param maxImpacts 書き込める数
	/// @return 書き込んだ数
	int StepPillars(const CAPSULE (&bodies)[FIGHTER_COUNT], const bool (&hittable)[FIGHTER_COUNT], PILLAR_IMPACT impacts[], int maxImpacts);

private:
	void DrawModelMap(int model) const;
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
	PillarCalc m_Pillars;	// 倒れる柱（コロシアム）
	int m_MapModels[STAGE_MAP_COUNT] = {};	// マップごとの景観のモデル（モデルで描かないマップと読めなかったマップは -1）
	int m_ColumnModel = -1;	// 倒れる柱のモデル
};
