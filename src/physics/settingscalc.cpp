#include "physics/settingscalc.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace
{
	// 「名前=値」の値を整数として読む（数でなければ false）。
	bool ReadInt(const std::string& value, int& out)
	{
		if (value.empty()) return false;
		char* end = nullptr;
		const long number = std::strtol(value.c_str(), &end, 10);
		if (end == value.c_str() || *end != '\0') return false;
		out = static_cast<int>(number);
		return true;
	}

	// 「前置き番号」「前置き番号.残り」の番号を読む（前置きが違う・番号が範囲外なら -1）。rest に残りを返す。
	int ReadIndex(const std::string& key, const char* prefix, int count, std::string& rest)
	{
		const size_t length = std::strlen(prefix);
		if (key.compare(0, length, prefix) != 0) return -1;
		const size_t dot = key.find('.', length);
		int index = 0;
		if (!ReadInt(key.substr(length, dot == std::string::npos ? std::string::npos : dot - length), index)) return -1;
		rest = dot == std::string::npos ? "" : key.substr(dot + 1);
		return index >= 0 && index < count ? index : -1;
	}

	// 1行の「名前=値」を設定に上書きする。
	void ApplyLine(const std::string& key, int value, OPTION_SETTINGS& settings)
	{
		std::string rest;
		if (key == "look") {
			settings.LookSensitivityLevel = value;
			return;
		}
		if (key == "guide") {
			if (value >= 0 && value < CONTROL_GUIDE_COUNT) settings.ControlGuide = static_cast<CONTROL_GUIDE>(value);
			return;
		}
		int index = ReadIndex(key, "tuning.", TUNING_ITEM_COUNT, rest);
		if (index >= 0 && rest.empty()) {
			if (value >= 0 && value < TuningCalcLevelCount(static_cast<TUNING_ITEM>(index))) settings.Tuning.Levels[index] = static_cast<uint8_t>(value);
			return;
		}
		index = ReadIndex(key, "volume.", SETTINGS_VOLUME_COUNT, rest);
		if (index >= 0 && rest.empty()) {
			settings.VolumeLevels[index] = value;
			return;
		}
		const int row = ReadIndex(key, "armcost.", ARM_COST_ROW_COUNT, rest);
		int item = 0;
		if (row >= 0 && ReadInt(rest, item) && item >= 0 && item < ARM_COST_ITEM_COUNT) ArmCostCalcSet(settings.ArmCosts, row, item, value);
	}
}

std::string SettingsCalcFormat(const OPTION_SETTINGS& settings)
{
	std::string text = "# Proto1 options (written by the game)\n";
	char line[64];
	for (int i = 0; i < TUNING_ITEM_COUNT; ++i) {
		sprintf_s(line, "tuning.%d=%d\n", i, settings.Tuning.Levels[i]);
		text += line;
	}
	sprintf_s(line, "look=%d\n", settings.LookSensitivityLevel);
	text += line;
	sprintf_s(line, "guide=%d\n", static_cast<int>(settings.ControlGuide));
	text += line;
	for (int i = 0; i < SETTINGS_VOLUME_COUNT; ++i) {
		sprintf_s(line, "volume.%d=%d\n", i, settings.VolumeLevels[i]);
		text += line;
	}
	for (int row = 0; row < ARM_COST_ROW_COUNT; ++row) {
		for (int item = 0; item < ARM_COST_ITEM_COUNT; ++item) {
			if (!ArmCostCalcIsUsed(row, item)) continue;
			sprintf_s(line, "armcost.%d.%d=%d\n", row, item, settings.ArmCosts.Values[row][item]);
			text += line;
		}
	}
	return text;
}

void SettingsCalcParse(const std::string& text, OPTION_SETTINGS& settings)
{
	size_t start = 0;
	while (start < text.size()) {
		size_t end = text.find('\n', start);
		if (end == std::string::npos) end = text.size();
		std::string line = text.substr(start, end - start);
		start = end + 1;
		if (!line.empty() && line.back() == '\r') line.pop_back();
		if (line.empty() || line[0] == '#') continue;
		const size_t equal = line.find('=');
		int value = 0;
		if (equal == std::string::npos || !ReadInt(line.substr(equal + 1), value)) continue;
		ApplyLine(line.substr(0, equal), value, settings);
	}
}

bool SettingsCalcEquals(const OPTION_SETTINGS& a, const OPTION_SETTINGS& b)
{
	if (a.Tuning.Levels != b.Tuning.Levels || a.LookSensitivityLevel != b.LookSensitivityLevel
		|| a.ControlGuide != b.ControlGuide) return false;
	for (int i = 0; i < SETTINGS_VOLUME_COUNT; ++i) {
		if (a.VolumeLevels[i] != b.VolumeLevels[i]) return false;
	}
	return std::memcmp(a.ArmCosts.Values, b.ArmCosts.Values, sizeof(a.ArmCosts.Values)) == 0;
}
