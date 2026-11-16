#pragma once

#include "core/dx11.h"
#include "physics/scenecalc.h"
#include "physics/versuscalc.h"
#include <array>

class NetSession;
class SessionAddressCalc;

/// @brief Codex が担当する専用メニュー。試合の HUD・腕の描画とは独立している
class CodexMenu
{
public:
	/// @brief 日本語の見出し・説明・設定値の文字画像を初期化する
	void Initialize();
	/// @brief 専用背景と現在の画面を描く（全画面のビューポート・深度無効で呼ぶ）
	/// @param scene 現在のメニューとカーソル
	/// @param participants ロビーの参加者、結果画面では試合の順位
	/// @param net 接続状態とホストのアドレス
	/// @param address 参加先の編集中のアドレス
	/// @param characters 各参加者のキャラクター
	/// @param tuning 次の試合へ適用する設定
	void Draw(const SceneCalc& scene, const VersusCalc& participants, const NetSession& net,
		const SessionAddressCalc& address, const std::array<CHARACTER_TYPE, FIGHTER_COUNT>& characters,
		const MATCH_TUNING& tuning) const;

private:
	void Text(const char* text, float x, float y, float height, const XMFLOAT4& color) const;
	void Background() const;
	void Reactor(float x, float y, float radius) const;
	void Header(const char* title, const char* eyebrow, int step) const;
	void Row(const char* title, const char* subtitle, int index, bool selected, float y, float height = 66.0f) const;
	void Detail(const char* tag, const char* title, const char* line1, const char* line2, const char* line3) const;
	void Footer(const char* help) const;
	void DrawOptions(const SceneCalc& scene, const MATCH_TUNING& tuning) const;
	void DrawLobby(const VersusCalc& participants, const NetSession& net,
		const std::array<CHARACTER_TYPE, FIGHTER_COUNT>& characters) const;
	void DrawResult(const SceneCalc& scene, const VersusCalc& participants) const;
};
