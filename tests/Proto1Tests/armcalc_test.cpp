// 腕の巡回・直接選択（DirectX なし）の単体テスト
#include "CppUnitTest.h"
#include "physics/armcalc.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace Proto1Tests
{
	TEST_CLASS(ArmCalcTest)
	{
	public:
		TEST_METHOD(Shoulder_BothHandsAreBehindAndSymmetricAtSameHeight)
		{
			const VEC3 position{ 3, 4, 5 };
			const VEC3 left = ArmCalcShoulder(position, 0, 0, 0.65f, 1.35f, -0.1f);
			const VEC3 right = ArmCalcShoulder(position, 0, 1, 0.65f, 1.35f, -0.1f);
			Assert::AreEqual(2.35f, left.X, 0.0001f);
			Assert::AreEqual(3.65f, right.X, 0.0001f);
			Assert::AreEqual(5.35f, left.Y, 0.0001f);
			Assert::AreEqual(left.Y, right.Y, 0.0001f);
			Assert::AreEqual(4.9f, left.Z, 0.0001f);
			Assert::AreEqual(left.Z, right.Z, 0.0001f);
		}

		TEST_METHOD(Shoulder_TurnsWithCharacterInsteadOfStayingInWorldAxes)
		{
			const VEC3 position{ 3, 4, 5 };
			const VEC3 left = ArmCalcShoulder(position, 1.57079632679f, 0, 0.65f, 1.35f, -0.1f);
			const VEC3 right = ArmCalcShoulder(position, 1.57079632679f, 1, 0.65f, 1.35f, -0.1f);
			Assert::AreEqual(2.9f, left.X, 0.0001f);
			Assert::AreEqual(left.X, right.X, 0.0001f);
			Assert::AreEqual(5.35f, left.Y, 0.0001f);
			Assert::AreEqual(left.Y, right.Y, 0.0001f);
			Assert::AreEqual(5.65f, left.Z, 0.0001f);
			Assert::AreEqual(4.35f, right.Z, 0.0001f);
		}

		TEST_METHOD(Rest_OpensBothHands15DegreesAndKeepsMinus45Pitch)
		{
			const VEC3 expected[] = {
				{ -0.1830127f, -0.70710678f, 0.6830127f },
				{ 0.1830127f, -0.70710678f, 0.6830127f }
			};
			for (int hand = 0; hand < 2; ++hand) {
				const VEC3 actual = ArmCalcRestDirection(0, hand, -0.78539816339f, 0.2617993878f);
				Assert::AreEqual(0.0f, Vec3Length(Vec3Sub(expected[hand], actual)), 0.0001f);
				Assert::AreEqual(1.0f, Vec3Length(actual), 0.0001f);
			}
		}

		TEST_METHOD(Rest_OpeningTurnsWithCharacter)
		{
			const VEC3 expected[] = {
				{ 0.6830127f, -0.70710678f, 0.1830127f },
				{ 0.6830127f, -0.70710678f, -0.1830127f }
			};
			for (int hand = 0; hand < 2; ++hand) {
				const VEC3 actual = ArmCalcRestDirection(1.57079632679f, hand, -0.78539816339f, 0.2617993878f);
				Assert::AreEqual(0.0f, Vec3Length(Vec3Sub(expected[hand], actual)), 0.0001f);
			}
		}

		TEST_METHOD(Rest_ElbowCanFaceHorizontallyWhileKeepingOpening)
		{
			const VEC3 expected[] = {
				{ -0.25881905f, 0.0f, 0.96592583f },
				{ 0.25881905f, 0.0f, 0.96592583f }
			};
			for (int hand = 0; hand < 2; ++hand) {
				const VEC3 actual = ArmCalcRestDirection(0, hand, 0, 0.2617993878f);
				Assert::AreEqual(0.0f, Vec3Length(Vec3Sub(expected[hand], actual)), 0.0001f);
			}
		}

		TEST_METHOD(Pitch_Minus45HasEqualForwardAndDown)
		{
			const VEC3 actual = ArmCalcPitchDirection({ 0, 0, 1 }, -0.78539816339f);
			Assert::AreEqual(0.0f, actual.X, 0.0001f);
			Assert::AreEqual(-sqrtf(0.5f), actual.Y, 0.0001f);
			Assert::AreEqual(sqrtf(0.5f), actual.Z, 0.0001f);
		}

		TEST_METHOD(Blend_EndsKeepDirections)
		{
			const VEC3 a{ 0, -0.6f, 0.8f }, b{ 1, 0, 0 };
			Assert::AreEqual(0.0f, Vec3Length(Vec3Sub(a, ArmCalcBlendDirection(a, b, 0))), 0.0001f);
			Assert::AreEqual(0.0f, Vec3Length(Vec3Sub(b, ArmCalcBlendDirection(a, b, 1))), 0.0001f);
		}

		TEST_METHOD(Blend_NormalizesLinearMixture)
		{
			const VEC3 a{ 0, 0, 1 }, b{ 1, 0, 0 };
			const float ratios[] = { 0.25f, 0.5f, 0.75f };
			for (float ratio : ratios) {
				const VEC3 actual = ArmCalcBlendDirection(a, b, ratio);
				const float length = sqrtf(ratio * ratio + (1.0f - ratio) * (1.0f - ratio));
				Assert::AreEqual(1.0f, Vec3Length(actual), 0.0001f);
				Assert::AreEqual(ratio / length, actual.X, 0.0001f);
				Assert::AreEqual(0.0f, actual.Y, 0.0001f);
				Assert::AreEqual((1.0f - ratio) / length, actual.Z, 0.0001f);
			}
		}

		TEST_METHOD(Blend_OppositeAndNearlyOppositeUseEndDirection)
		{
			const VEC3 a{ 0, 0, 1 };
			const VEC3 ends[] = { { 0, 0, -1 }, { 0.000001f, 0, -1 } };
			for (const VEC3& b : ends) {
				const VEC3 actual = ArmCalcBlendDirection(a, b, 0.5f);
				const VEC3 expected = Vec3Scale(b, 1.0f / Vec3Length(b));
				Assert::AreEqual(1.0f, Vec3Length(actual), 0.0001f);
				Assert::AreEqual(0.0f, Vec3Length(Vec3Sub(expected, actual)), 0.0001f);
			}
		}

		TEST_METHOD(Roll_ZeroKeepsAxesExactly)
		{
			const TENTACLE_PIECE_AXES axes = TentacleCalcPieceAxes({ 1, 2, 3 }, { 4, 7, 8 }, { 0, -1, 0 });
			const TENTACLE_PIECE_AXES actual = ArmCalcRollAxes(axes, 0);
			Assert::AreEqual(0.0f, Vec3Length(Vec3Sub(axes.X, actual.X)));
			Assert::AreEqual(0.0f, Vec3Length(Vec3Sub(axes.Y, actual.Y)));
			Assert::AreEqual(0.0f, Vec3Length(Vec3Sub(axes.Z, actual.Z)));
		}

		TEST_METHOD(Roll_90ExchangesXAndZWithSigns)
		{
			const TENTACLE_PIECE_AXES axes{ { 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 } };
			const float signs[] = { -1.0f, 1.0f };
			for (float sign : signs) {
				const TENTACLE_PIECE_AXES actual = ArmCalcRollAxes(axes, sign * 1.57079632679f);
				Assert::AreEqual(0.0f, Vec3Length(Vec3Sub(Vec3Scale(axes.Z, sign), actual.X)), 0.0001f);
				Assert::AreEqual(0.0f, Vec3Length(Vec3Sub(Vec3Scale(axes.X, -sign), actual.Z)), 0.0001f);
				Assert::AreEqual(0.0f, Vec3Length(Vec3Sub(axes.Y, actual.Y)));
			}
		}

		TEST_METHOD(Select_AvailableNumber)
		{
			const bool available[] = { true, false, true, false, false, true };
			Assert::AreEqual(0, ArmCalcSelect(0, available, 6));
			Assert::AreEqual(2, ArmCalcSelect(2, available, 6));
			Assert::AreEqual(5, ArmCalcSelect(5, available, 6));
		}

		TEST_METHOD(Select_UnavailableReturnsMinusOne)
		{
			const bool available[] = { true, false, false, false, false, false };
			for (int number = 1; number < 6; ++number) Assert::AreEqual(-1, ArmCalcSelect(number, available, 6));
		}

	};
}
