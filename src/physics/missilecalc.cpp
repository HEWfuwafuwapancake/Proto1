#include "physics/missilecalc.h"

MISSILE_VOLLEY MissileCalcVolley(int fingersLeft, int durability, int pressCost, int shotCost, bool all)
{
	const int left = fingersLeft < 0 ? 0 : (fingersLeft > MISSILE_FINGER_COUNT ? MISSILE_FINGER_COUNT : fingersLeft);
	MISSILE_VOLLEY volley;
	if (all || left == 0) {
		volley.Fingers = all ? left : 0;
		volley.Arm = true;
		return volley;
	}
	// 耐久が減らないときは1本ずつ。減るときは、1回に減る量が指何本ぶんかで撃つ本数を決める。
	const bool consumes = durability > 0 && pressCost > 0 && shotCost > 0;
	int count = consumes ? (pressCost + shotCost / 2) / shotCost : 1;
	if (count < 1) count = 1;
	volley.Fingers = count < left ? count : left;
	volley.Arm = volley.Fingers < count || (consumes && durability - pressCost <= 0);
	return volley;
}
