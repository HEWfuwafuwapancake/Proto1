#pragma once

#include "core/dx11.h"
#include "physics/armcostcalc.h"
#include "physics/scenecalc.h"
#include "physics/stagecalc.h"
#include "physics/versuscalc.h"
#include "render/glyphcalc.h"
#include "render/skinnedmodel.h"
#include <array>
#include <chrono>

class NetSession;
class SessionAddressCalc;

/// @brief 試合以外の画面（タイトル・対戦方法・キャラ選択・オプション・結果）を、ポップな見た目（ピンク・黄緑・紫の斜めの帯）で描く
/// @details 背景は、ステージのまわりをゆっくり回るカメラで描いた3Dを ぼかしたもの（render/backdrop.h）。試合の HUD・腕の描画とは独立している
class CodexMenu
{
public:
	/// @brief 日本語の文字画像と選択画面専用のモデル・待機姿勢を初期化する
	void Initialize();
	/// @brief 選択画面専用のモデルと姿勢、背景をぼかす画面を解放する
	void Finalize();
	/// @brief 画面や選んでいる項目が変わった時刻を覚える（選んだボタンが弾む動きに使う。毎フレーム呼ぶ）
	/// @param scene 現在のメニューとカーソル
	void Update(const SceneCalc& scene);
	/// @brief 背景用に、ステージのまわりを斜め上からゆっくり回るカメラの行列をシェーダーに設定する
	/// @details BackdropBegin のあと、ステージなどの3Dを描く前に呼ぶ
	/// @param stage 見るステージ（浮島の中心と大きさから、回る中心と距離を決める）
	void SetBackdropCamera(const STAGE& stage) const;
	/// @brief 背景と現在の画面を描く（全画面のビューポート・深度無効で呼ぶ）
	/// @param scene 現在のメニューとカーソル
	/// @param participants ロビーの参加者、結果画面では試合の順位
	/// @param net 接続状態とホストのアドレス
	/// @param address 参加先の編集中のアドレス
	/// @param characters 各参加者のキャラクター
	/// @param tuning 次の試合へ適用する設定
	/// @param costs オフラインで使う腕の耐久の減る量の表
	void Draw(const SceneCalc& scene, const VersusCalc& participants, const NetSession& net,
		const SessionAddressCalc& address, const std::array<CHARACTER_TYPE, FIGHTER_COUNT>& characters,
		const MATCH_TUNING& tuning, const ARM_COST_TABLE& costs) const;
	/// @brief 試合中のメニュー（再開・ロビーへ・タイトルへ）を、止まった試合の画面の上に重ねて描く（全画面のビューポート・深度無効で呼ぶ）
	/// @param scene 現在のメニューとカーソル
	void DrawPause(const SceneCalc& scene) const;

private:
	float GetSeconds() const; // メニューを作ってからの秒数（背景のカメラ・帯の流れ・揺れに使う）
	float GetPop() const; // 選んでいる項目が変わってからの弾み（0 から始まり少し行き過ぎて 1）
	void DrawCharacterPreview(CHARACTER_TYPE character, const RECT2D& area, float yaw, float viewHeight) const;
	void Background() const;
	void Header(const char* title, const char* eyebrow) const;
	void Footer(const char* help) const;
	void Button(const char* title, const char* subtitle, bool selected, bool dark, float x, float y, float width, float height, float growWidth) const; // growWidth：選んだとき右へ伸びる長さ
	void InfoCard(const RECT2D& area, const char* title, const char* line1, const char* line2, const char* line3) const;
	void DrawTitle(const SceneCalc& scene, const std::array<CHARACTER_TYPE, FIGHTER_COUNT>& characters) const;
	void DrawModeSelect(const SceneCalc& scene) const;
	void DrawJoinAddress(const SessionAddressCalc& address) const;
	void DrawOptions(const SceneCalc& scene, const MATCH_TUNING& tuning) const;
	void DrawArmCosts(const SceneCalc& scene, const ARM_COST_TABLE& costs) const;
	void DrawLobby(const SceneCalc& scene, const VersusCalc& participants, const NetSession& net,
		const std::array<CHARACTER_TYPE, FIGHTER_COUNT>& characters) const;
	void DrawResult(const SceneCalc& scene, const VersusCalc& participants,
		const std::array<CHARACTER_TYPE, FIGHTER_COUNT>& characters) const;
	void DrawMapSelect(const SceneCalc& scene, const NetSession& net, float sizeScale) const;
	void DrawMiniMap(STAGE_MAP map, const RECT2D& area) const;
	std::array<SkinnedModel, CHARACTER_COUNT> m_PreviewModels;
	std::array<SKINNED_POSE, CHARACTER_COUNT> m_PreviewPoses;
	std::chrono::steady_clock::time_point m_PreviewStart{};
	std::chrono::steady_clock::time_point m_ChangeTime{}; // 画面か選んでいる項目が最後に変わった時刻
	SCENE m_LastScene = SCENE_GAME_START; // 前のフレームの画面
	int m_LastCursor = -1; // 前のフレームのカーソル
	STAGE_MAP m_LastMap = STAGE_MAP_DEFAULT; // 前のフレームのマップ選択の選択
};
