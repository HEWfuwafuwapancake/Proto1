// 腕の巡回・直接選択（DirectX なし）の単体テスト
#include "CppUnitTest.h"
#include "physics/armcalc.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace Proto1Tests
{
	TEST_CLASS(ArmCalcTest)
	{
	public:
		TEST_METHOD(Pitch_ZeroKeepsHorizontalForward)
		{
			const VEC3 forward{ 0.6f, 0, 0.8f };
			const VEC3 actual = ArmCalcPitchDirection(forward, 0);
			Assert::AreEqual(0.0f, Vec3Length(Vec3Sub(forward, actual)), 0.0001f);
		}

		TEST_METHOD(Pitch_Minus45HasEqualForwardAndDown)
		{
			const VEC3 actual = ArmCalcPitchDirection({ 0, 0, 1 }, -0.78539816339f);
			Assert::AreEqual(0.0f, actual.X, 0.0001f);
			Assert::AreEqual(-sqrtf(0.5f), actual.Y, 0.0001f);
			Assert::AreEqual(sqrtf(0.5f), actual.Z, 0.0001f);
		}

		TEST_METHOD(Pitch_NormalizesHorizontalForward)
		{
			const float pitches[] = { -1.57079632679f, -0.78539816339f, 0.0f, 0.4f, 1.57079632679f };
			for (float pitch : pitches) {
				const VEC3 actual = ArmCalcPitchDirection({ 3, 0, 4 }, pitch);
				Assert::AreEqual(1.0f, Vec3Length(actual), 0.0001f);
				Assert::AreEqual(0.6f * cosf(pitch), actual.X, 0.0001f);
				Assert::AreEqual(sinf(pitch), actual.Y, 0.0001f);
				Assert::AreEqual(0.8f * cosf(pitch), actual.Z, 0.0001f);
			}
		}

		TEST_METHOD(Pitch_IgnoresForwardHeight)
		{
			const VEC3 expected = ArmCalcPitchDirection({ 3, 0, 4 }, -0.78539816339f);
			const float heights[] = { -20.0f, 20.0f };
			for (float height : heights) {
				const VEC3 actual = ArmCalcPitchDirection({ 3, height, 4 }, -0.78539816339f);
				Assert::AreEqual(0.0f, Vec3Length(Vec3Sub(expected, actual)), 0.0001f);
			}
		}

		TEST_METHOD(Pitch_NoHorizontalForwardUsesPositiveZ)
		{
			const VEC3 actual = ArmCalcPitchDirection({ 0, 5, 0 }, 0);
			Assert::AreEqual(0.0f, Vec3Length(Vec3Sub(VEC3{ 0, 0, 1 }, actual)), 0.0001f);
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

		TEST_METHOD(Blend_ClampsRatioAndNeverReturnsZeroForZeroInputs)
		{
			const VEC3 a{ 0, 0, 1 }, b{ 1, 0, 0 };
			Assert::AreEqual(0.0f, Vec3Length(Vec3Sub(a, ArmCalcBlendDirection(a, b, -1))), 0.0001f);
			Assert::AreEqual(0.0f, Vec3Length(Vec3Sub(b, ArmCalcBlendDirection(a, b, 2))), 0.0001f);
			Assert::AreEqual(1.0f, Vec3Length(ArmCalcBlendDirection({}, {}, 0.5f)), 0.0001f);
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

		TEST_METHOD(Roll_PreservesUnitLengthsAndOrthogonality)
		{
			const TENTACLE_PIECE_AXES axes = TentacleCalcPieceAxes({}, { 1, 2, 3 }, { 0, -1, 0 });
			const TENTACLE_PIECE_AXES actual = ArmCalcRollAxes(axes, 0.7f);
			Assert::AreEqual(1.0f, Vec3Length(actual.X), 0.0001f);
			Assert::AreEqual(1.0f, Vec3Length(actual.Y), 0.0001f);
			Assert::AreEqual(1.0f, Vec3Length(actual.Z), 0.0001f);
			Assert::AreEqual(0.0f, Vec3Dot(actual.X, actual.Y), 0.0001f);
			Assert::AreEqual(0.0f, Vec3Dot(actual.X, actual.Z), 0.0001f);
			Assert::AreEqual(0.0f, Vec3Dot(actual.Y, actual.Z), 0.0001f);
		}

		TEST_METHOD(Cycle_Next)
		{
			const bool available[] = { true, true, true, true, true, true };
			Assert::AreEqual(3, ArmCalcCycle(2, 1, available, 6));
		}

		TEST_METHOD(Cycle_Previous)
		{
			const bool available[] = { true, true, true, true, true, true };
			Assert::AreEqual(1, ArmCalcCycle(2, -1, available, 6));
		}

		TEST_METHOD(Cycle_WrapsBothEnds)
		{
			const bool available[] = { true, true, true, true, true, true };
			Assert::AreEqual(0, ArmCalcCycle(5, 1, available, 6));
			Assert::AreEqual(5, ArmCalcCycle(0, -1, available, 6));
		}

		TEST_METHOD(Cycle_SkipsUnavailableInBothDirections)
		{
			const bool available[] = { true, false, false, true, false, false };
			Assert::AreEqual(3, ArmCalcCycle(0, 1, available, 6));
			Assert::AreEqual(0, ArmCalcCycle(3, -1, available, 6));
			Assert::AreEqual(0, ArmCalcCycle(3, 1, available, 6));
			Assert::AreEqual(3, ArmCalcCycle(0, -1, available, 6));
		}

		TEST_METHOD(Cycle_SkipsUnavailableAtBothEnds)
		{
			const bool available[] = { false, true, false, false, true, false };
			Assert::AreEqual(1, ArmCalcCycle(4, 1, available, 6));
			Assert::AreEqual(4, ArmCalcCycle(1, -1, available, 6));
		}

		TEST_METHOD(Cycle_OnlyCurrentAvailableDoesNotChange)
		{
			const bool tentacleOnly[] = { true, false, false, false, false, false };
			Assert::AreEqual(0, ArmCalcCycle(0, 1, tentacleOnly, 6));
			Assert::AreEqual(0, ArmCalcCycle(0, -1, tentacleOnly, 6));
			const bool middleOnly[] = { false, false, true, false, false, false };
			Assert::AreEqual(2, ArmCalcCycle(2, 1, middleOnly, 6));
			Assert::AreEqual(2, ArmCalcCycle(2, -1, middleOnly, 6));
			const bool single[] = { true };
			Assert::AreEqual(0, ArmCalcCycle(0, 1, single, 1));
			Assert::AreEqual(0, ArmCalcCycle(0, -1, single, 1));
		}

		TEST_METHOD(Cycle_NoAvailableReturnsMinusOne)
		{
			const bool available[] = { false, false, false, false, false, false };
			Assert::AreEqual(-1, ArmCalcCycle(0, 1, available, 6));
			Assert::AreEqual(-1, ArmCalcCycle(0, -1, available, 6));
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

		TEST_METHOD(Select_OutOfRangeReturnsMinusOne)
		{
			const bool available[] = { true, true, true, true, true, true };
			Assert::AreEqual(-1, ArmCalcSelect(-1, available, 6));
			Assert::AreEqual(-1, ArmCalcSelect(6, available, 6));
			Assert::AreEqual(-1, ArmCalcSelect(100, available, 6));
		}

		TEST_METHOD(InvalidArgumentsReturnMinusOne)
		{
			const bool available[] = { true };
			Assert::AreEqual(-1, ArmCalcCycle(0, 1, nullptr, 1));
			Assert::AreEqual(-1, ArmCalcCycle(0, 1, available, 0));
			Assert::AreEqual(-1, ArmCalcCycle(-1, 1, available, 1));
			Assert::AreEqual(-1, ArmCalcCycle(1, 1, available, 1));
			Assert::AreEqual(-1, ArmCalcCycle(0, 0, available, 1));
			Assert::AreEqual(-1, ArmCalcCycle(0, 2, available, 1));
			Assert::AreEqual(-1, ArmCalcSelect(0, nullptr, 1));
			Assert::AreEqual(-1, ArmCalcSelect(0, available, 0));
			Assert::AreEqual(-1, ArmCalcSelect(0, available, -1));
		}
	};
}
