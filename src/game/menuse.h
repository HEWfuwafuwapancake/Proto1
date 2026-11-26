#pragma once

#include <array>
#include "audio/se.h"
#include "physics/scenecalc.h"
#include "physics/versuscalc.h"

/// @brief 効果音を選ぶために見る、メニューの1フレーム分の状態
struct MENU_SE_STATE
{
	SCENE Scene = SCENE_GAME_START; ///< 今の画面
	int Cursor = 0; ///< モード選択・オプション・結果・ポーズで選んでいる項目
	int Map = 0; ///< マップ選択で選んでいるマップ
	int Rule = 0; ///< キャラ選択で選んでいる試合のルール
	int Focus = 0; ///< キャラ選択で 1P が選んでいるカード（ルールの行は VERSUS_LOBBY_FOCUS_RULE）
	unsigned Values = 0; ///< オプションの設定・耐久の表・視点感度・音量をまとめた数（どれかが変わると変わる）
	std::array<int, FIGHTER_COUNT> Characters{}; ///< スロットごとのキャラ
	std::array<int, FIGHTER_COUNT> CpuLevels{}; ///< スロットごとの CPU の強さ（ダミーは VERSUS_CPU_NONE）
	unsigned Joined = 0; ///< 参加している人のスロット（1 << スロット）
	unsigned Ready = 0; ///< 準備OKのスロット（1 << スロット）
};

/// @brief メニューの状態の変わり方から、カーソル・決定・戻る・キャラ選択の効果音を鳴らす
/// @details Game が更新の前と後の状態を渡す。メニューの計算（SceneCalc・VersusCalc）には手を入れず、変わったことだけを見る。
///          戻るの音は「前の画面へ戻った・準備OKを取り消した」という結果で鳴らす（押したボタンでは選ばない）。
///          案内の声（「キャラクターを選んでね」「よろしくおねがいね」）は1つの枠で鳴らし、次の声・取り消し・画面を出るときに止める
class MenuSe
{
public:
	/// @brief 1フレーム分の変わり方を見て、効果音を鳴らす
	/// @param before 更新の前の状態
	/// @param after 更新の後の状態
	/// @param back このフレームにどれかの機器で戻るを押したか（true：押した、false：押していない）
	/// @param decide このフレームにどれかの機器で決定を押したか（true：押した、false：押していない）
	/// @param tuningReset このフレームにオプションの「標準に戻す」を受け付けたか（true：受け付けた、false：受け付けていない）
	void Update(const MENU_SE_STATE& before, const MENU_SE_STATE& after, bool back, bool decide, bool tuningReset);

private:
	SeVoice m_Guide; // 案内の声（前の声を止めてから鳴らす）
};
