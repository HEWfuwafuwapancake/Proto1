#pragma once

/// @file settingscalc.h
/// @brief オプションの設定を、ファイルに書く文字にする・文字から読む（ファイルの読み書きは Game。DirectX・Windows 不使用）
/// @details 1行に「名前=値」を1つ書く。知らない名前・数でない値・範囲外の段は読み飛ばし、元の値を残す
///          （古いファイルや手で書き換えたファイルでも起動できるように）。

#include <string>
#include "physics/armcostcalc.h"
#include "physics/scenecalc.h"
#include "physics/tuningcalc.h"

/// @brief 音量を保存するジャンルの数（サウンドの画面の音量の行の数。audio/se.h の SE_VOLUME_COUNT と同じ）
static constexpr int SETTINGS_VOLUME_COUNT = SOUND_ITEM_BACK;

/// @brief ファイルに保存するオプションの設定（この PC だけの設定も含む）
struct OPTION_SETTINGS
{
	MATCH_TUNING Tuning = TuningCalcDefault(); ///< 試合の設定の段
	ARM_COST_TABLE ArmCosts = ArmCostCalcDefault(); ///< 腕の耐久の減る量の表
	int LookSensitivityLevel = 0; ///< 視点感度の段（範囲にそろえるのは input/action）
	int VolumeLevels[SETTINGS_VOLUME_COUNT] = {}; ///< ジャンルごとの音量の段（SOUND_ITEM の順。範囲にそろえるのは audio/se）
};

/// @brief 設定をファイルに書く文字にする
/// @param settings 書く設定
/// @return 「名前=値」を1行ずつ並べた文字（改行は \n）
std::string SettingsCalcFormat(const OPTION_SETTINGS& settings);

/// @brief ファイルの文字を読み、書いてある項目だけを設定に上書きする
/// @param text ファイルの中身（改行は \n でも \r\n でもよい）
/// @param settings 上書きする設定（書いていない項目・読めない項目は元のまま）
void SettingsCalcParse(const std::string& text, OPTION_SETTINGS& settings);

/// @brief 2つの設定がすべて同じか調べる（保存していない変更があるかを見るため）
/// @param a 1つ目
/// @param b 2つ目
/// @return すべての項目が同じなら true
bool SettingsCalcEquals(const OPTION_SETTINGS& a, const OPTION_SETTINGS& b);
