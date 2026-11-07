#include "game/character.h"

// 調整用の値（肩は足元を基準にしたゲーム内の長さ）
static constexpr CHARACTER_DATA CHARACTERS[CHARACTER_COUNT] = {
	{ "MEKA", { "assets/models/meka_idle.fbx", "assets/models/meka_run.fbx",
		"assets/models/meka_armL_forward.fbx", "assets/models/meka_armR_forward.fbx" },
		54.7f, -2.9f, XM_PI, { 0.29f, 1.30f, 0.0f } },
	{ "MEKAGIRL", { "assets/models/mekagirl_idle.fbx", "assets/models/mekagirl_run.fbx",
		"assets/models/mekagirl_armL_forward.fbx", "assets/models/mekagirl_armR_forward.fbx" },
		110.76f, 0.0f, XM_PI, { 0.22f, 1.28f, 0.0f } }, // 正面と胸の横の肩位置は画面確認で合わせる
};

const CHARACTER_DATA& CHARACTER_DATA::Get(CHARACTER_TYPE character)
{
	return CHARACTERS[character];
}
