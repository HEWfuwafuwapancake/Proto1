#pragma once

/// @brief 回転する腕名の1面の形（表示枠の上端が原点）
struct CUBE_FLIP_FACE
{
	float Y; ///< 面の上端のずれ
	float Height; ///< 縦につぶした面の高さ
	float Facing; ///< 面の正面の向き具合（0：真横、1：正面）
};

/// @brief 前の面と次の面の形
struct CUBE_FLIP
{
	CUBE_FLIP_FACE Previous; ///< 倒れて消える面
	CUBE_FLIP_FACE Next; ///< 起き上がって出てくる面
};

/// @brief 回転中の2面を、合計が枠の高さになるよう上下に並べる
/// @param progress 回転の進み具合（0：始め、1：終わり）
/// @param height 枠の高さ
/// @param direction 次へ +1（前の面が上）、前へ -1（前の面が下）
CUBE_FLIP CubeFlipCalc(float progress, float height, int direction);
