#include <gtest/gtest.h>
#include "render/damagecolorcalc.h"

// % の数字は 0% で白、100% でオレンジ、間はなめらかに混ざり、200% 以上は暗い赤のまま。
TEST(DamageColorCalcTest, TextColorWarmsWithDamage)
{
	const DAMAGE_COLOR white = DamageColorCalcText(0.0f);
	EXPECT_NEAR(1.0f, white.B, 1.0e-4f);
	const DAMAGE_COLOR orange = DamageColorCalcText(100.0f);
	EXPECT_NEAR(0.55f, orange.G, 1.0e-4f);
	const DAMAGE_COLOR between = DamageColorCalcText(25.0f);
	EXPECT_NEAR(0.65f, between.B, 1.0e-4f);
	const DAMAGE_COLOR dark = DamageColorCalcText(300.0f);
	EXPECT_NEAR(0.6f, dark.R, 1.0e-4f);
}

// 体の赤みは % が低いうちは 0 で、高くなるほど増えて上限で止まる。煙は 100% と 150% で段が上がる。
TEST(DamageColorCalcTest, BodyTintAndSmokeRiseWithDamage)
{
	EXPECT_NEAR(0.0f, DamageColorCalcBodyTint(50.0f), 1.0e-4f);
	EXPECT_GT(DamageColorCalcBodyTint(150.0f), DamageColorCalcBodyTint(100.0f));
	EXPECT_NEAR(0.55f, DamageColorCalcBodyTint(250.0f), 1.0e-4f);
	EXPECT_EQ(DAMAGE_SMOKE_NONE, DamageColorCalcSmoke(99.0f));
	EXPECT_EQ(DAMAGE_SMOKE_GRAY, DamageColorCalcSmoke(100.0f));
	EXPECT_EQ(DAMAGE_SMOKE_BLACK, DamageColorCalcSmoke(150.0f));
}
