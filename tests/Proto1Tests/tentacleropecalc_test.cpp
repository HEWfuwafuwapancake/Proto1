#include <gtest/gtest.h>
#include "physics/tentacleropecalc.h"

namespace
{
	constexpr float DT = 1.0f / 60.0f;
	const float LENGTHS[3] = { 0.2f, 0.2f, 0.1f };

	TENTACLE_ROPE_PARAMS NoBody()
	{
		TENTACLE_ROPE_PARAMS params;
		params.Stiffness = 0.0f;
		return params;
	}
}

// 横に伸ばした鎖を放すと、重力で根元の真下へ垂れ下がり、節の長さは変わらない。
TEST(TentacleRopeCalcTest, HangsStraightDownKeepingLengths)
{
	TentacleRopeCalc rope;
	rope.Reset({ 0, 2, 0 }, { 1, 0, 0 }, LENGTHS, 3);
	for (int i = 0; i < 600; ++i) rope.Step({ 0, 2, 0 }, NoBody(), DT);
	const VEC3 tip = rope.GetPoint(3);
	EXPECT_NEAR(0.0f, tip.X, 0.01f);
	EXPECT_NEAR(1.5f, tip.Y, 0.01f);
	EXPECT_NEAR(0.2f, Vec3Length(Vec3Sub(rope.GetPoint(1), rope.GetPoint(0))), 0.0001f);
	EXPECT_NEAR(0.1f, Vec3Length(Vec3Sub(rope.GetPoint(3), rope.GetPoint(2))), 0.0001f);
}

// 根元が一定の速さで動き続けると、空気抵抗で先は進む向きの反対へなびく。
TEST(TentacleRopeCalcTest, TrailsBehindMovingAnchor)
{
	TentacleRopeCalc rope;
	rope.Reset({ 0, 2, 0 }, { 0, -1, 0 }, LENGTHS, 3);
	VEC3 anchor{ 0, 2, 0 };
	for (int i = 0; i < 240; ++i) {
		anchor.Z += 6.0f * DT;
		rope.Step(anchor, NoBody(), DT);
	}
	const VEC3 tip = rope.GetPoint(3);
	EXPECT_LT(tip.Z, anchor.Z - 0.2f);
	EXPECT_GT(tip.Y, anchor.Y - 0.5f + 0.05f);
	// 振り子のように揺れ続けず、なびいた形で落ち着く（振り子の半周期あとも同じ位置）。
	for (int i = 0; i < 42; ++i) {
		anchor.Z += 6.0f * DT;
		rope.Step(anchor, NoBody(), DT);
	}
	EXPECT_NEAR(tip.Z - (anchor.Z - 42 * 6.0f * DT), rope.GetPoint(3).Z - anchor.Z, 0.02f);
}

// 体の円柱の中には入らず、外側へ押し出される。長さの割合の位置は節の上をたどる。
TEST(TentacleRopeCalcTest, StaysOutsideBodyAndSamplesAlongLength)
{
	TentacleRopeCalc rope;
	rope.Reset({ 0.3f, 2, 0 }, { -1, 0, 0 }, LENGTHS, 3);
	TENTACLE_ROPE_PARAMS params = NoBody();
	params.BodyBottom = { 0, 0, 0 };
	params.BodyTop = { 0, 2, 0 };
	params.BodyRadius = 0.15f;
	rope.Step({ 0.3f, 2, 0 }, params, DT);
	for (int i = 1; i < rope.GetPointCount(); ++i) {
		const VEC3 p = rope.GetPoint(i);
		EXPECT_GE(sqrtf(p.X * p.X + p.Z * p.Z), 0.15f - 0.0001f);
	}
	TentacleRopeCalc straight;
	straight.Reset({ 0, 2, 0 }, { 0, -1, 0 }, LENGTHS, 3);
	EXPECT_NEAR(1.8f, straight.GetPointAt(0.4f).Y, 0.0001f);
	EXPECT_NEAR(1.65f, straight.GetPointAt(0.7f).Y, 0.0001f);
}
