#include "game/character.h"

// 調整用の値（肩は足元を基準にしたゲーム内の長さ）
static constexpr CHARACTER_DATA CHARACTERS[CHARACTER_COUNT] = {
	{ "MEKA", { "assets/models/meka_idle.fbx", "assets/models/meka_run.fbx",
		"assets/models/meka_armL_forward.fbx", "assets/models/meka_armR_forward.fbx", nullptr, nullptr, nullptr, nullptr },
		{ "joint2", { "joint2", "joint3", "joint4" } }, // joint1 が腰、joint2〜4 が背骨
		54.7f, -2.9f, XM_PI, { 0.29f, 1.30f, 0.0f }, {}, 1.0f, 0.0f }, // 腕の骨はないので肩は固定
	{ "MEKAGIRL", { "assets/models/mekagirl_idle.fbx", "assets/models/mekagirl_run.fbx",
		"assets/models/mekagirl_armL_forward.fbx", "assets/models/mekagirl_armR_forward.fbx",
		"assets/models/mekagirl_run_back.fbx", "assets/models/mekagirl_jump.fbx",
		"assets/models/mekagirl_fall.fbx", "assets/models/mekagirl_land.fbx",
		"assets/models/mekagirl_atk_katana_iai_l.fbx", "assets/models/mekagirl_atk_katana_iai_r.fbx",
		"assets/models/mekagirl_atk_katana_assault_l.fbx", "assets/models/mekagirl_atk_katana_assault_r.fbx",
		"assets/models/mekagirl_atk_hammer_tap_l.fbx", "assets/models/mekagirl_atk_hammer_tap_r.fbx",
		"assets/models/mekagirl_atk_hammer_slam_l.fbx", "assets/models/mekagirl_atk_hammer_slam_r.fbx",
		"assets/models/mekagirl_atk_gunlance_shot_l.fbx", "assets/models/mekagirl_atk_gunlance_shot_r.fbx",
		"assets/models/mekagirl_atk_gunlance_rush_l.fbx", "assets/models/mekagirl_atk_gunlance_rush_r.fbx",
		"assets/models/mekagirl_atk_knuckle_punch_l.fbx", "assets/models/mekagirl_atk_knuckle_punch_r.fbx",
		"assets/models/mekagirl_atk_knuckle_rush_l.fbx", "assets/models/mekagirl_atk_knuckle_rush_r.fbx" },
		{ "spine_01", { "spine_01", "spine_02", "chest" }, { "upperarm_l", "upperarm_r" } },
		110.76f, 0.0f, XM_PI, { 0.22f, 1.28f, 0.0f },
		{ { { "upperarm_l", "lowerarm_l", "hand_l", "hand_end_l" }, { "upperarm_r", "lowerarm_r", "hand_r", "hand_end_r" } } },
		0.6f, 0.08f }, // 触腕は体に見合った大きさ。付け根は肩の骨より 0.08m 外（肩幅）
};

const CHARACTER_DATA& CHARACTER_DATA::Get(CHARACTER_TYPE character)
{
	return CHARACTERS[character];
}
