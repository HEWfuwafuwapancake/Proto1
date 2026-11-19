#pragma once

/// @file missilecalc.h
/// @brief ミサイル腕の、1回の発射で撃つ指と腕の数の決め方（DirectX・Windows なし）

/// @brief ミサイル腕の指の本数（指を撃ち切ったあとに腕を撃つ）
static constexpr int MISSILE_FINGER_COUNT = 5;

/// @brief 1回の発射で撃つもの
struct MISSILE_VOLLEY
{
	int Fingers = 0; ///< 撃つ指の本数（0〜MISSILE_FINGER_COUNT）
	bool Arm = false; ///< 腕も撃つかを設定する（true：撃って残りの耐久を全部使う、false：指だけ撃つ）
};

/// @brief 1回の発射で撃つ指の本数と、腕も撃つかを決める
/// @details ふだんは指を1本撃つ。耐久の持ちの倍率で1回に減る量が増えると、減る量 ÷ shotCost（四捨五入）本ずつ撃つ。
///          撃っている途中で耐久が0になるか、指がなくなったら腕も撃つ。耐久が減らないとき（無制限・DEBUG で0）は
///          1本ずつ撃ち、指を撃ち切ったら次は腕を撃つ
/// @param fingersLeft 残っている指の本数（0 なら次は腕）
/// @param durability 残りの耐久（減らないときは 0 以下を渡す）
/// @param pressCost 1回の発射で減る量（耐久の持ちの倍率で割ったあと）
/// @param shotCost 指1本ぶんの減る量（等倍のとき。0 以下なら 1 本ずつ）
/// @param all 残りを全部撃つかを設定する（true：長押し。残りの指と腕を全部、false：単押し）
/// @return 撃つ指の本数と、腕も撃つか
MISSILE_VOLLEY MissileCalcVolley(int fingersLeft, int durability, int pressCost, int shotCost, bool all);
