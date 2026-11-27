#pragma once


#include "core/dx11.h"
#include "game/arm.h"
#include "physics/scenecalc.h"
#include "physics/versuscalc.h"
#include <array>

class Camera;
class FighterManager;
class Match;
class NetSession;
class SessionAddressCalc;

/// @file hud.h
/// @brief シーンごとの画面表示（タイトル・試合の HUD・結果のメニュー）
/// @details 数字は 7 セグ、英字は 5×7 ドット（render/number2d.h）。置き場所・大きさ・色は hud.cpp の「調整用の値」。
///          タイトルと結果は半透明のおおいとメニュー、時間切れは TIME UP・スコア・結果への案内を出す

/// @brief シーン・試合・ダミーの状態から画面表示を管理する
class Hud
{
public:

	/// @brief 全スロット分の腕・自分のカメラと試合への参照を受け取り、腕名の画像を用意する（DX11 の初期化後）
	void Initialize(const Camera& camera, const FighterManager& fighters, int slot, const Match& match, const SceneCalc& scene, const ArmManager* arms);

	/// @brief 状態を最初に戻す（参照はそのまま）
	void Reset(void);

	/// @brief 両手の装備の変更を読み、腕名の回転を進める
	void Update(void);

	/// @brief 自分の画面の大きさを設定する（HUD は 1280×720 を縦横同じ倍率で縮めて当てはめる）
	/// @param width 幅（ピクセル）
	/// @param height 高さ（ピクセル）
	void SetViewSize(float width, float height);

	/// @brief 試合中の自分の画面の HUD を描く（自分のビューポートで、深度テストを切りライトを消してから呼ぶ）
	void Draw(void) const;

	/// @brief 起動・キャラ・モード・IP・ロビー・結果を画面全体に描く（全画面のビューポートに戻してから呼ぶ）
	void DrawScene(const VersusCalc& participants, const NetSession& net, const SessionAddressCalc& address,
		const std::array<CHARACTER_TYPE, FIGHTER_COUNT>& characters) const;

	/// @brief 試合中の操作一覧を画面全体の右端の真ん中に、薄く小さく描く（画面全体のビューポートで呼ぶ）
	/// @param guide 表示の種類（オフなら何も描かない）
	void DrawControlGuide(CONTROL_GUIDE guide) const;

	/// @brief オンラインのずれ・VS の版の警告を画面左上に描く（画面全体のビューポートで最後に呼ぶ）
	void DrawCornerWarning(const NetSession& net) const;

private:
	void DrawPlayerTag(int slot, float centerX, float y, float digitWidth) const;
	void DrawPercent(float percent, float centerX, float y, float digitWidth, float pop) const;
	void DrawFighterPercent(int index) const;
	void DrawArmName(int hand) const;
	void DrawGuardFeedback(int slot) const;
	void DrawReticle(void) const; // ハード・ソフトロックで狙う相手を囲むかぎ括弧と距離、フリーでは画面の真ん中の十字
	void DrawChargeGauge(float centerX, float centerY) const; // レーザーを溜めている間、レティクルの周りに溜めの輪を出す
	void DrawCameraMode(void) const; // カメラの状態（ハードロック・ソフトロック・フリー）の名前
	void DrawMenuItem(const char* text, float y, bool selected) const;
	void DrawMenuHelp(float selectY, float decideY) const;
	const Camera* m_Camera = nullptr;
	const FighterManager* m_Fighters = nullptr;
	const Match* m_Match = nullptr;
	const SceneCalc* m_Scene = nullptr;
	const ArmManager* m_Arms = nullptr;
	const ArmManager* m_AllArms = nullptr; // 全員の結果を各画面のカメラで描くための参照
	int m_ArmNames[ARM_TYPE_COUNT] = {};
	int m_GuideTexts[10][3] = {}; // 操作一覧の文字画像（hud.cpp の CONTROL_GUIDE_LINES と同じ並び）
	struct ARM_FLIP
	{
		ARM_TYPE Previous = ARM_TYPE_TENTACLE;
		ARM_TYPE Current = ARM_TYPE_TENTACLE;
		float Elapsed = 0.0f;
		int Direction = 1;
	};
	ARM_FLIP m_ArmFlips[2];
	float m_LastPercents[FIGHTER_COUNT] = {}; // 前のフレームの % （増えたら数字を弾ませる）
	float m_PercentPops[FIGHTER_COUNT] = {}; // % の数字の弾みの残り（0〜1）
	int m_LastScore = 0; // 前のフレームの自分の KO 数（増えたら数字を弾ませる）
	float m_ScorePop = 0.0f; // KO 数の弾みの残り（0〜1）
	int m_Slot = 0;
	float m_Width = 1280.0f;	// HUD の座標での画面の幅
	float m_Height = 720.0f;	// HUD の座標での画面の高さ
	float m_Scale = 1.0f;		// HUD の座標 → ビューポートのピクセルの倍率
};
