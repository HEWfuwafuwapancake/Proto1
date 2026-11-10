// 腕の巡回・直接選択（DirectX なし）の単体テスト
#include <gtest/gtest.h>
#include "physics/armcalc.h"

namespace Proto1Tests
{
	TEST(ArmCalcTest, Cycle_MovesBothWaysSkipsUnavailableAndWraps)
	{
		const bool available[] = { true, true, false, true };
		EXPECT_EQ(1, ArmCalcCycle(0, 1, available, 4));
		EXPECT_EQ(0, ArmCalcCycle(1, -1, available, 4));
		EXPECT_EQ(3, ArmCalcCycle(1, 1, available, 4));
		EXPECT_EQ(1, ArmCalcCycle(3, -1, available, 4));
		EXPECT_EQ(0, ArmCalcCycle(3, 1, available, 4));
		EXPECT_EQ(3, ArmCalcCycle(0, -1, available, 4));
	}

	// 段階の進みをクリップの区切りへ写し、段階の終わりでは次の段階の手前で止まる。
	TEST(ArmCalcTest, ClipTime_MapsPhaseProgressToClipSegments)
	{
		const float phaseSeconds[] = { 0.3f, 0.12f, 0.3f };
		const float clipSeconds[] = { 0.5f, 0.25f, 0.5f };
		EXPECT_NEAR(0.25f, ArmCalcClipTime(0, 0.15f, phaseSeconds, clipSeconds), 0.0001f);
		EXPECT_NEAR(0.625f, ArmCalcClipTime(1, 0.06f, phaseSeconds, clipSeconds), 0.0001f);
		EXPECT_NEAR(1.25f, ArmCalcClipTime(2, 10.0f, phaseSeconds, clipSeconds), 0.0001f);
	}

	TEST(ArmCalcTest, Shoulder_BothHandsAreBehindAndSymmetricAtSameHeight)
	{
		const VEC3 position{ 3, 4, 5 };
		const VEC3 left = ArmCalcShoulder(position, 0, 0, 0.65f, 1.35f, -0.1f);
		const VEC3 right = ArmCalcShoulder(position, 0, 1, 0.65f, 1.35f, -0.1f);
		EXPECT_NEAR(2.35f, left.X, 0.0001f);
		EXPECT_NEAR(3.65f, right.X, 0.0001f);
		EXPECT_NEAR(5.35f, left.Y, 0.0001f);
		EXPECT_NEAR(left.Y, right.Y, 0.0001f);
		EXPECT_NEAR(4.9f, left.Z, 0.0001f);
		EXPECT_NEAR(left.Z, right.Z, 0.0001f);
	}

	TEST(ArmCalcTest, Shoulder_TurnsWithCharacterInsteadOfStayingInWorldAxes)
	{
		const VEC3 position{ 3, 4, 5 };
		const VEC3 left = ArmCalcShoulder(position, 1.57079632679f, 0, 0.65f, 1.35f, -0.1f);
		const VEC3 right = ArmCalcShoulder(position, 1.57079632679f, 1, 0.65f, 1.35f, -0.1f);
		EXPECT_NEAR(2.9f, left.X, 0.0001f);
		EXPECT_NEAR(left.X, right.X, 0.0001f);
		EXPECT_NEAR(5.35f, left.Y, 0.0001f);
		EXPECT_NEAR(left.Y, right.Y, 0.0001f);
		EXPECT_NEAR(5.65f, left.Z, 0.0001f);
		EXPECT_NEAR(4.35f, right.Z, 0.0001f);
	}

	TEST(ArmCalcTest, Rest_OpensBothHands15DegreesAndKeepsMinus45Pitch)
	{
		const VEC3 expected[] = {
			{ -0.1830127f, -0.70710678f, 0.6830127f },
			{ 0.1830127f, -0.70710678f, 0.6830127f }
		};
		for (int hand = 0; hand < 2; ++hand) {
			const VEC3 actual = ArmCalcRestDirection(0, hand, -0.78539816339f, 0.2617993878f);
			EXPECT_NEAR(0.0f, Vec3Length(Vec3Sub(expected[hand], actual)), 0.0001f);
			EXPECT_NEAR(1.0f, Vec3Length(actual), 0.0001f);
		}
	}

	TEST(ArmCalcTest, Rest_OpeningTurnsWithCharacter)
	{
		const VEC3 expected[] = {
			{ 0.6830127f, -0.70710678f, 0.1830127f },
			{ 0.6830127f, -0.70710678f, -0.1830127f }
		};
		for (int hand = 0; hand < 2; ++hand) {
			const VEC3 actual = ArmCalcRestDirection(1.57079632679f, hand, -0.78539816339f, 0.2617993878f);
			EXPECT_NEAR(0.0f, Vec3Length(Vec3Sub(expected[hand], actual)), 0.0001f);
		}
	}

	TEST(ArmCalcTest, Rest_ElbowCanFaceHorizontallyWhileKeepingOpening)
	{
		const VEC3 expected[] = {
			{ -0.25881905f, 0.0f, 0.96592583f },
			{ 0.25881905f, 0.0f, 0.96592583f }
		};
		for (int hand = 0; hand < 2; ++hand) {
			const VEC3 actual = ArmCalcRestDirection(0, hand, 0, 0.2617993878f);
			EXPECT_NEAR(0.0f, Vec3Length(Vec3Sub(expected[hand], actual)), 0.0001f);
		}
	}

	TEST(ArmCalcTest, Pitch_Minus45HasEqualForwardAndDown)
	{
		const VEC3 actual = ArmCalcPitchDirection({ 0, 0, 1 }, -0.78539816339f);
		EXPECT_NEAR(0.0f, actual.X, 0.0001f);
		EXPECT_NEAR(-sqrtf(0.5f), actual.Y, 0.0001f);
		EXPECT_NEAR(sqrtf(0.5f), actual.Z, 0.0001f);
	}

	TEST(ArmCalcTest, Blend_EndsKeepDirections)
	{
		const VEC3 a{ 0, -0.6f, 0.8f }, b{ 1, 0, 0 };
		EXPECT_NEAR(0.0f, Vec3Length(Vec3Sub(a, ArmCalcBlendDirection(a, b, 0))), 0.0001f);
		EXPECT_NEAR(0.0f, Vec3Length(Vec3Sub(b, ArmCalcBlendDirection(a, b, 1))), 0.0001f);
	}

	TEST(ArmCalcTest, Blend_NormalizesLinearMixture)
	{
		const VEC3 a{ 0, 0, 1 }, b{ 1, 0, 0 };
		const float ratios[] = { 0.25f, 0.5f, 0.75f };
		for (float ratio : ratios) {
			const VEC3 actual = ArmCalcBlendDirection(a, b, ratio);
			const float length = sqrtf(ratio * ratio + (1.0f - ratio) * (1.0f - ratio));
			EXPECT_NEAR(1.0f, Vec3Length(actual), 0.0001f);
			EXPECT_NEAR(ratio / length, actual.X, 0.0001f);
			EXPECT_NEAR(0.0f, actual.Y, 0.0001f);
			EXPECT_NEAR((1.0f - ratio) / length, actual.Z, 0.0001f);
		}
	}

	TEST(ArmCalcTest, Blend_OppositeAndNearlyOppositeUseEndDirection)
	{
		const VEC3 a{ 0, 0, 1 };
		const VEC3 ends[] = { { 0, 0, -1 }, { 0.000001f, 0, -1 } };
		for (const VEC3& b : ends) {
			const VEC3 actual = ArmCalcBlendDirection(a, b, 0.5f);
			const VEC3 expected = Vec3Scale(b, 1.0f / Vec3Length(b));
			EXPECT_NEAR(1.0f, Vec3Length(actual), 0.0001f);
			EXPECT_NEAR(0.0f, Vec3Length(Vec3Sub(expected, actual)), 0.0001f);
		}
	}

	TEST(ArmCalcTest, Roll_ZeroKeepsAxesExactly)
	{
		const TENTACLE_PIECE_AXES axes = TentacleCalcPieceAxes({ 1, 2, 3 }, { 4, 7, 8 }, { 0, -1, 0 });
		const TENTACLE_PIECE_AXES actual = ArmCalcRollAxes(axes, 0);
		EXPECT_EQ(0.0f, Vec3Length(Vec3Sub(axes.X, actual.X)));
		EXPECT_EQ(0.0f, Vec3Length(Vec3Sub(axes.Y, actual.Y)));
		EXPECT_EQ(0.0f, Vec3Length(Vec3Sub(axes.Z, actual.Z)));
	}

	TEST(ArmCalcTest, Roll_90ExchangesXAndZWithSigns)
	{
		const TENTACLE_PIECE_AXES axes{ { 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 } };
		const float signs[] = { -1.0f, 1.0f };
		for (float sign : signs) {
			const TENTACLE_PIECE_AXES actual = ArmCalcRollAxes(axes, sign * 1.57079632679f);
			EXPECT_NEAR(0.0f, Vec3Length(Vec3Sub(Vec3Scale(axes.Z, sign), actual.X)), 0.0001f);
			EXPECT_NEAR(0.0f, Vec3Length(Vec3Sub(Vec3Scale(axes.X, -sign), actual.Z)), 0.0001f);
			EXPECT_EQ(0.0f, Vec3Length(Vec3Sub(axes.Y, actual.Y)));
		}
	}

	TEST(ArmCalcTest, Select_AvailableNumber)
	{
		const bool available[] = { true, false, true, false, false, true };
		EXPECT_EQ(0, ArmCalcSelect(0, available, 6));
		EXPECT_EQ(2, ArmCalcSelect(2, available, 6));
		EXPECT_EQ(5, ArmCalcSelect(5, available, 6));
	}

	TEST(ArmCalcTest, Select_UnavailableReturnsMinusOne)
	{
		const bool available[] = { true, false, false, false, false, false };
		for (int number = 1; number < 6; ++number) EXPECT_EQ(-1, ArmCalcSelect(number, available, 6));
	}
}
