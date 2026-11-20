#include "gtest/gtest.h"
#include "physics/missilecalc.h"

namespace
{
	constexpr int SHOT_COST = 16; // 指1本ぶんの減る量（等倍）

	// 単押しを指がなくなるか腕を撃つまで続け、1回ごとに撃った指の本数を返す（腕を撃った回は -本数）。
	int Fire(int& fingers, int& durability, int pressCost, int* shots, int maxShots)
	{
		int count = 0;
		while (count < maxShots) {
			const MISSILE_VOLLEY volley = MissileCalcVolley(fingers, durability, pressCost, SHOT_COST, false);
			fingers -= volley.Fingers;
			if (durability > 0) durability = volley.Arm ? 0 : durability - pressCost;
			shots[count++] = volley.Arm ? -volley.Fingers : volley.Fingers;
			if (volley.Arm) break;
		}
		return count;
	}
}

// 等倍（1回に16減る）では、指を1本ずつ5回撃ち、6回目で腕を撃つ。
TEST(MissileCalcTest, NormalUsesFiveFingersThenArm)
{
	int fingers = MISSILE_FINGER_COUNT, durability = 100, shots[8];
	ASSERT_EQ(6, Fire(fingers, durability, 16, shots, 8));
	for (int i = 0; i < 5; ++i) EXPECT_EQ(1, shots[i]);
	EXPECT_EQ(0, shots[5]); // 6回目は腕だけ
}

// 耐久の持ち ×0.5（1回に32減る）では、指を2本ずつ撃ち、3回目で最後の指と腕を撃つ。
TEST(MissileCalcTest, HalfDurabilityFiresTwoFingersPerPress)
{
	int fingers = MISSILE_FINGER_COUNT, durability = 100, shots[8];
	ASSERT_EQ(3, Fire(fingers, durability, 32, shots, 8));
	EXPECT_EQ(2, shots[0]);
	EXPECT_EQ(2, shots[1]);
	EXPECT_EQ(-1, shots[2]); // 最後の指1本と腕
}

// 耐久の持ち ×2（1回に8減る）でも6回で使い切り、耐久が残っていても腕を撃ったら終わる。
TEST(MissileCalcTest, DoubleDurabilityStillUsesUpInSixPresses)
{
	int fingers = MISSILE_FINGER_COUNT, durability = 100, shots[8];
	ASSERT_EQ(6, Fire(fingers, durability, 8, shots, 8));
	EXPECT_EQ(0, shots[5]);
}

// 耐久が足りなくなったら（ガードで減ったなど）、その回で指と腕を撃って使い切る。
TEST(MissileCalcTest, ArmFiresWhenDurabilityRunsOut)
{
	const MISSILE_VOLLEY volley = MissileCalcVolley(3, 10, 16, SHOT_COST, false);
	EXPECT_EQ(1, volley.Fingers);
	EXPECT_TRUE(volley.Arm);
}

// 耐久が減らないとき（無制限）は1本ずつ撃ち、長押しは残りの指と腕を全部撃つ。
TEST(MissileCalcTest, UnlimitedFiresOneAndHoldFiresAll)
{
	const MISSILE_VOLLEY tap = MissileCalcVolley(4, -1, 0, SHOT_COST, false);
	EXPECT_EQ(1, tap.Fingers);
	EXPECT_FALSE(tap.Arm);
	const MISSILE_VOLLEY hold = MissileCalcVolley(4, 100, 16, SHOT_COST, true);
	EXPECT_EQ(4, hold.Fingers);
	EXPECT_TRUE(hold.Arm);
}
