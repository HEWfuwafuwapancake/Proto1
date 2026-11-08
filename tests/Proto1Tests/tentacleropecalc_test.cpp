#include <gtest/gtest.h>
#include "physics/tentacleropecalc.h"
#include <cmath>

namespace
{
	constexpr float DT = 1.0f / 60.0f;
	const float LENGTHS[3] = { 0.2f, 0.2f, 0.1f };

	// つなぎ目の曲がる上限を外した、体のない鎖
	TENTACLE_ROPE_PARAMS FreeChain()
	{
		TENTACLE_ROPE_PARAMS params;
		params.MaxBend = 3.2f;
		params.MaxRootBend = 3.2f;
		params.MaxTipBend = 3.2f;
		params.Damping = 1.2f;
		params.AirDrag = 0.8f;
		return params;
	}

	float Angle(const VEC3& a, const VEC3& b)
	{
		const float c = Vec3Dot(a, b) / (Vec3Length(a) * Vec3Length(b));
		return acosf(fmaxf(-1.0f, fminf(1.0f, c)));
	}
}

// 横に伸ばした鎖を放すと、重さで根元の真下へ垂れ下がって止まり、節の長さはほぼ変わらない。
TEST(TentacleRopeCalcTest, HangsStraightDownKeepingLengths)
{
	TentacleRopeCalc rope;
	rope.Reset({ 0, 2, 0 }, { 1, 0, 0 }, LENGTHS, 3);
	for (int i = 0; i < 900; ++i) rope.Step({ 0, 2, 0 }, FreeChain(), DT);
	const VEC3 tip = rope.GetPoint(3);
	EXPECT_NEAR(0.0f, tip.X, 0.01f);
	EXPECT_NEAR(1.5f, tip.Y, 0.01f);
	EXPECT_NEAR(0.2f, Vec3Length(Vec3Sub(rope.GetPoint(1), rope.GetPoint(0))), 0.005f);
	EXPECT_NEAR(0.1f, Vec3Length(Vec3Sub(rope.GetPoint(3), rope.GetPoint(2))), 0.005f);
}

// 根元が一定の速さで動き続けると、体と一緒に動きつつ空気抵抗で少し後ろへなびき、その形で落ち着く。
TEST(TentacleRopeCalcTest, TrailsBehindMovingAnchorAndSettles)
{
	TentacleRopeCalc rope;
	rope.Reset({ 0, 2, 0 }, { 0, -1, 0 }, LENGTHS, 3);
	VEC3 anchor{ 0, 2, 0 };
	for (int i = 0; i < 300; ++i) {
		anchor.Z += 6.0f * DT;
		rope.Step(anchor, FreeChain(), DT);
	}
	const float behind = rope.GetPoint(3).Z - anchor.Z;
	EXPECT_LT(behind, -0.1f);
	for (int i = 0; i < 40; ++i) {
		anchor.Z += 6.0f * DT;
		rope.Step(anchor, FreeChain(), DT);
	}
	EXPECT_NEAR(behind, rope.GetPoint(3).Z - anchor.Z, 0.02f);
}

// 重さで勢いが残るので、走っていた根元が止まると、先は根元より前へ振り出される。
TEST(TentacleRopeCalcTest, SwingsForwardWhenAnchorStops)
{
	TentacleRopeCalc rope;
	rope.Reset({ 0, 2, 0 }, { 0, -1, 0 }, LENGTHS, 3);
	VEC3 anchor{ 0, 2, 0 };
	for (int i = 0; i < 300; ++i) {
		anchor.Z += 6.0f * DT;
		rope.Step(anchor, FreeChain(), DT);
	}
	float farthest = -1.0f;
	for (int i = 0; i < 30; ++i) {
		rope.Step(anchor, FreeChain(), DT);
		farthest = fmaxf(farthest, rope.GetPoint(3).Z - anchor.Z);
	}
	EXPECT_GT(farthest, 0.15f);
}

// つなぎ目ごとの可動域より曲がらない（肩は広め・途中は狭め・先っぽは少し広め）。
TEST(TentacleRopeCalcTest, LimitsBendAtEachJoint)
{
	TentacleRopeCalc rope;
	rope.Reset({ 0, 2, 0 }, { 1, 0, 0 }, LENGTHS, 3);
	TENTACLE_ROPE_PARAMS params;
	params.MaxBend = 0.2f;
	params.MaxRootBend = 0.8f;
	params.MaxTipBend = 0.4f;
	rope.Step({ 0, 2, 0 }, params, DT);
	// 横から放した直後なので、肩のつなぎ目は広い可動域いっぱいまで曲がっている。
	EXPECT_NEAR(0.8f, Angle(params.RootDirection, Vec3Sub(rope.GetPoint(1), rope.GetPoint(0))), 0.05f);
	EXPECT_LE(Angle(Vec3Sub(rope.GetPoint(1), rope.GetPoint(0)), Vec3Sub(rope.GetPoint(2), rope.GetPoint(1))), 0.2f + 0.05f);
	EXPECT_LE(Angle(Vec3Sub(rope.GetPoint(2), rope.GetPoint(1)), Vec3Sub(rope.GetPoint(3), rope.GetPoint(2))), 0.4f + 0.05f);
}

// 体の円柱の中には入らず、外側へ押し出される。長さの割合の位置は節の上をたどる。
TEST(TentacleRopeCalcTest, StaysOutsideBodyAndSamplesAlongLength)
{
	TentacleRopeCalc rope;
	rope.Reset({ 0.3f, 2, 0 }, { -1, 0, 0 }, LENGTHS, 3);
	TENTACLE_ROPE_PARAMS params = FreeChain();
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
