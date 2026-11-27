#pragma once

/// @file damagecolorcalc.h
/// @brief ダメージ% から、% の数字の色・体の赤み・煙の段を決める計算（スマブラのように、たまるほど危なく見せる）
/// @details DirectX と Windows を使わないので、単体テスト（tests/Proto1Tests）からそのまま使える。見た目だけで使う。

/// @brief 色（0〜1）
struct DAMAGE_COLOR
{
	float R; ///< 赤
	float G; ///< 緑
	float B; ///< 青
};

/// @brief 体から出す煙の段
enum DAMAGE_SMOKE
{
	DAMAGE_SMOKE_NONE, ///< 出さない
	DAMAGE_SMOKE_GRAY, ///< ときどき灰色の煙（100% 以上）
	DAMAGE_SMOKE_BLACK, ///< 黒い煙が出続け、ときどき火花（150% 以上）
};

/// @brief % の数字の色を求める（0% 白 → 50% 黄 → 100% オレンジ → 150% 赤 → 200% 以上 暗い赤。間はなめらかに混ぜる）
/// @param percent ダメージ%
DAMAGE_COLOR DamageColorCalcText(float percent);

/// @brief 体の色に混ぜる赤の割合（0〜1）を求める（100% で少し、150% 以上ではっきり）
/// @param percent ダメージ%
float DamageColorCalcBodyTint(float percent);

/// @brief 体から出す煙の段を求める
/// @param percent ダメージ%
DAMAGE_SMOKE DamageColorCalcSmoke(float percent);
