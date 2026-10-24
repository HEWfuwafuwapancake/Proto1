// core/mathutil.h（Saturate・SmoothStep01・Lerp）の単体テスト
#include "CppUnitTest.h"
#include "core/mathutil.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace Proto1Tests
{
	static constexpr float EPSILON = 1.0e-6f;	// float の比較で許す誤差

	TEST_CLASS(MathUtilTest)
	{
	public:
		// カメラとプレイヤーで共用する回転：速さの上限と、±180° をまたぐ最短方向
		TEST_METHOD(TurnYaw_LimitsSpeedAndCrossesPiByShortestPath)
		{
			const float pi = 3.14159265358979323846f;
			Assert::AreEqual(0.1f, TurnYaw(0, 1, 0.2f, 0.5f), EPSILON);
			Assert::AreEqual(-pi + 0.05f,
				TurnYaw(pi - 0.05f, -pi + 0.2f, 0.1f, 1), EPSILON);
			Assert::AreEqual(pi - 0.05f,
				TurnYaw(-pi + 0.05f, pi - 0.2f, 0.1f, 1), EPSILON);
		}

		TEST_METHOD(TurnYaw_DoesNotOvershootOrMoveWithoutTimeOrSpeed)
		{
			Assert::AreEqual(0.2f, TurnYaw(0, 0.2f, 2, 1), EPSILON);
			Assert::AreEqual(0.3f, TurnYaw(0.3f, 1, 2, 0), EPSILON);
			Assert::AreEqual(0.3f, TurnYaw(0.3f, 1, 0, 1), EPSILON);
			Assert::AreEqual(0.3f, TurnYaw(0.3f, 1, -2, 1), EPSILON);
			Assert::AreEqual(0.3f, TurnYaw(0.3f, 1, 2, -1), EPSILON);
		}

		// Saturate：0〜1 の中はそのまま、外は端に収める
		TEST_METHOD(Saturate_KeepsValueInRange)
		{
			Assert::AreEqual(0.0f, Saturate(0.0f), EPSILON);
			Assert::AreEqual(0.25f, Saturate(0.25f), EPSILON);
			Assert::AreEqual(1.0f, Saturate(1.0f), EPSILON);
		}

		TEST_METHOD(Saturate_ClampsOutOfRange)
		{
			Assert::AreEqual(0.0f, Saturate(-3.0f), EPSILON);
			Assert::AreEqual(1.0f, Saturate(5.0f), EPSILON);
		}

		// SmoothStep01：両端と真ん中の値、端の近くでは変化がゆっくり
		TEST_METHOD(SmoothStep01_EndsAndMiddle)
		{
			Assert::AreEqual(0.0f, SmoothStep01(0.0f), EPSILON);
			Assert::AreEqual(0.5f, SmoothStep01(0.5f), EPSILON);
			Assert::AreEqual(1.0f, SmoothStep01(1.0f), EPSILON);
		}

		TEST_METHOD(SmoothStep01_SlowNearEnds)
		{
			// 0.1 のときは、ふつうの直線（0.1）より小さい
			Assert::IsTrue(SmoothStep01(0.1f) < 0.1f);
			// 0.9 のときは、ふつうの直線（0.9）より大きい
			Assert::IsTrue(SmoothStep01(0.9f) > 0.9f);
		}

		// Lerp：t = 0 で a、t = 1 で b、t = 0.5 で真ん中
		TEST_METHOD(Lerp_EndsAndMiddle)
		{
			Assert::AreEqual(2.0f, Lerp(2.0f, 6.0f, 0.0f), EPSILON);
			Assert::AreEqual(6.0f, Lerp(2.0f, 6.0f, 1.0f), EPSILON);
			Assert::AreEqual(4.0f, Lerp(2.0f, 6.0f, 0.5f), EPSILON);
		}

		TEST_METHOD(Lerp_WorksWhenBIsSmaller)
		{
			Assert::AreEqual(-1.0f, Lerp(3.0f, -5.0f, 0.5f), EPSILON);
		}
	};
}
