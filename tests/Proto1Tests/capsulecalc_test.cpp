// physics/capsulecalc.h（腕カプセル）の単体テスト
#include <gtest/gtest.h>
#include "physics/capsulecalc.h"

static STAGE MakeCapsuleStage()
{
	STAGE stage{ 2.0f, -3.0f, 15.0f, 15.0f, 1.0f, 2.0f, -15.0f };
	stage.Platforms[0] = { 8.0f, -8.0f, 3.0f, 3.0f, 5.0f, 0.6f };
	stage.PlatformCount = 1;
	return stage;
}

static int CountActive(const ArmCapsuleCalc& calc)
{
	int count = 0;
	for (const auto& item : calc.GetItems()) if (item.Active) ++count;
	return count;
}

// 最初に8個置き、最初の6個だけで全種類がそろう。
TEST(ArmCapsuleCalcTest, StartsWithEightAndDrawsEveryTypeInFirstBag)
{
	ArmCapsuleCalc calc;
	calc.Start(MakeCapsuleStage(), 42u);
	EXPECT_EQ(CountActive(calc), 8);
	bool seen[6]{};
	for (int i = 0; i < 6; ++i) {
		const auto& item = calc.GetItems()[i];
		ASSERT_TRUE(item.Active);
		ASSERT_GE(item.Type, 0);
		ASSERT_LT(item.Type, 6);
		EXPECT_FALSE(seen[item.Type]);
		seen[item.Type] = true;
	}
}

// 60fps で90フレームごとに増え、16個を超えない。拾った枠は再利用できる。
TEST(ArmCapsuleCalcTest, SpawnsAtIntervalsUntilFull)
{
	const STAGE stage = MakeCapsuleStage();
	ArmCapsuleCalc calc;
	calc.Start(stage, 42u);
	constexpr float dt = 1.0f / 60.0f;
	for (int i = 0; i < 89; ++i) calc.Step(stage, dt);
	EXPECT_EQ(CountActive(calc), 8);
	calc.Step(stage, dt);
	EXPECT_EQ(CountActive(calc), 9);
	calc.Step(stage, 30.0f);
	EXPECT_EQ(CountActive(calc), 16);
	calc.Remove(0);
	EXPECT_EQ(CountActive(calc), 15);
	calc.Step(stage, 1.5f);
	EXPECT_EQ(CountActive(calc), 16);
}

// 出現する点はすべて浮島か高台の上面にある。
TEST(ArmCapsuleCalcTest, PlacesCapsulesOnIslandAndPlatforms)
{
	const STAGE stage = MakeCapsuleStage();
	ArmCapsuleCalc calc;
	calc.Start(stage, 42u);
	calc.Step(stage, 12.0f);
	bool onIsland = false;
	bool onPlatform = false;
	for (const auto& item : calc.GetItems()) {
		ASSERT_TRUE(item.Active);
		const VEC3& p = item.Position;
		EXPECT_TRUE(StageCalcHasSupport(stage, p.X, p.Y, p.Z));
		onIsland = onIsland || p.Y == stage.TopY;
		onPlatform = onPlatform || p.Y == stage.Platforms[0].TopY;
	}
	EXPECT_TRUE(onIsland);
	EXPECT_TRUE(onPlatform);
}

// 触れている体で枠を探せる。取得して空いた枠はもう拾えない。
TEST(ArmCapsuleCalcTest, FindsTouchingBodyAndIgnoresRemovedItem)
{
	ArmCapsuleCalc calc;
	calc.Start(MakeCapsuleStage(), 42u);
	const VEC3 position = calc.GetItems()[0].Position;
	const CAPSULE body{ Vec3Add(position, { 0.0f, 0.5f, 0.0f }),
		Vec3Add(position, { 0.0f, 1.3f, 0.0f }), 0.5f };
	EXPECT_EQ(calc.FindTouching(body), 0);
	calc.Remove(0);
	EXPECT_NE(calc.FindTouching(body), 0);
}
