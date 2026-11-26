// オプションの設定を文字にして読み戻す計算（DirectX なし）
#include <gtest/gtest.h>
#include "physics/settingscalc.h"

namespace Proto1Tests
{
	// はじめと違う値を入れた設定
	static OPTION_SETTINGS MakeChanged()
	{
		OPTION_SETTINGS settings;
		settings.Tuning.Levels[TUNING_DAMAGE] = 0;
		settings.LookSensitivityLevel = 6;
		settings.VolumeLevels[SOUND_ITEM_MASTER] = 7;
		settings.VolumeLevels[SOUND_ITEM_VOICE] = 3;
		ArmCostCalcSet(settings.ArmCosts, ARM_COST_ROW_HAMMER, ARM_COST_TAP_HIT, 42);
		return settings;
	}

	// 書いた文字を読むと、試合の設定・視点感度・音量・耐久の表がすべて元どおりになる。
	TEST(SettingsCalcTest, FormatThenParseRestoresAllSettings)
	{
		const OPTION_SETTINGS saved = MakeChanged();
		OPTION_SETTINGS loaded;
		SettingsCalcParse(SettingsCalcFormat(saved), loaded);
		EXPECT_TRUE(SettingsCalcEquals(saved, loaded));
		EXPECT_EQ(42, loaded.ArmCosts.Values[ARM_COST_ROW_HAMMER][ARM_COST_TAP_HIT]);
		EXPECT_EQ(3, loaded.VolumeLevels[SOUND_ITEM_VOICE]);
	}

	// 書いていない項目・知らない名前・範囲外の段・数でない値は読み飛ばし、元の値を残す（Windows の改行でも読める）。
	TEST(SettingsCalcTest, ParseKeepsCurrentValuesForMissingOrBadLines)
	{
		OPTION_SETTINGS settings = MakeChanged();
		SettingsCalcParse("volume.0=2\r\nunknown=5\r\ntuning.0=999\r\nlook=abc\r\n", settings);
		EXPECT_EQ(2, settings.VolumeLevels[SOUND_ITEM_MASTER]);
		EXPECT_EQ(TuningCalcDefault().Levels[0], settings.Tuning.Levels[0]);
		EXPECT_EQ(6, settings.LookSensitivityLevel);
		EXPECT_EQ(3, settings.VolumeLevels[SOUND_ITEM_VOICE]);
	}

	// 1つでも違えば「同じではない」（保存していない変更があると分かる）。
	TEST(SettingsCalcTest, EqualsDetectsAnyChange)
	{
		const OPTION_SETTINGS a = MakeChanged();
		OPTION_SETTINGS b = a;
		EXPECT_TRUE(SettingsCalcEquals(a, b));
		b.VolumeLevels[SOUND_ITEM_BATTLE] = 1;
		EXPECT_FALSE(SettingsCalcEquals(a, b));
	}
}
