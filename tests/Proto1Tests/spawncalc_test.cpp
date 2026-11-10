// physics/spawncalc.h（復活先の選択）の単体テスト
#include <gtest/gtest.h>
#include "physics/spawncalc.h"

// どちらかの相手に近い2点を除き、安全な2点（高台も含む）から選ぶ。
TEST(SpawnCalcTest, ChoosesOnlyPointsSafeFromAllOpponents)
{
	const VEC3 points[] = {
		{ 0.0f, 0.0f, 0.0f }, { 20.0f, 0.0f, 0.0f }, { 10.0f, 0.0f, 0.0f }, { 0.0f, 10.0f, 0.0f },
	};
	const VEC3 opponents[] = { { 0.0f, 0.0f, 0.0f }, { 20.0f, 0.0f, 0.0f } };
	SpawnCalc spawn;
	spawn.SetSeed(42u);
	bool choseIsland = false;
	bool chosePlatform = false;
	for (int i = 0; i < 32; ++i) {
		const int point = spawn.ChoosePoint(points, 4, opponents, 2, 10.0f);
		ASSERT_TRUE(point == 2 || point == 3);
		if (point == 2) choseIsland = true;
		else chosePlatform = true;
	}
	EXPECT_TRUE(choseIsland);
	EXPECT_TRUE(chosePlatform);
}

// すべて近ければ、一番近い相手までの距離が最大になる点を選ぶ。
TEST(SpawnCalcTest, MaximizesNearestOpponentDistanceWithoutSafePoints)
{
	const VEC3 points[] = { { 1.0f, 0.0f, 0.0f }, { 9.0f, 0.0f, 0.0f }, { 5.0f, 0.0f, 0.0f } };
	const VEC3 opponents[] = { { 0.0f, 0.0f, 0.0f }, { 10.0f, 0.0f, 0.0f } };
	SpawnCalc spawn;
	EXPECT_EQ(spawn.ChoosePoint(points, 3, opponents, 2, 10.0f), 2);
}
