#include "CppUnitTest.h"
#include "physics/areacalc.h"
#include <math.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace Proto1Tests
{
	static constexpr float AREA_EPSILON = 0.0001f;

	// 足元が (x, 0, z) の高さ 1.8・半径 0.5 の体。
	static CAPSULE AreaBody(float x, float z)
	{
		return { { x, 0.5f, z }, { x, 1.3f, z }, 0.5f };
	}

	TEST_CLASS(AreaCalcTest)
	{
	public:
		// 正面の扇には入り、後ろ・横の外・遠く・上下の外には入らない。
		TEST_METHOD(InSector_FrontOnly)
		{
			const VEC3 origin{ 0.0f, 1.2f, 0.0f };
			const VEC3 forward{ 0.0f, 0.0f, 1.0f };
			const float half = 3.14159265f / 3.0f;
			Assert::IsTrue(AreaCalcInSector(origin, forward, 0.0f, 2.0f, half, 1.0f, AreaBody(0.0f, 1.5f)));
			Assert::IsFalse(AreaCalcInSector(origin, forward, 0.0f, 2.0f, half, 1.0f, AreaBody(0.0f, -1.5f)));
			Assert::IsFalse(AreaCalcInSector(origin, forward, 0.0f, 2.0f, half, 1.0f, AreaBody(1.9f, 0.3f)));
			Assert::IsFalse(AreaCalcInSector(origin, forward, 0.0f, 2.0f, half, 1.0f, AreaBody(0.0f, 4.0f)));
			const CAPSULE high{ { 0.0f, 5.0f, 1.5f }, { 0.0f, 5.8f, 1.5f }, 0.5f };
			Assert::IsFalse(AreaCalcInSector(origin, forward, 0.0f, 2.0f, half, 1.0f, high));
		}

		// 吹っ飛ばす向きは水平＋上で長さ 1、まわりへの向きは中心から外。
		TEST_METHOD(LaunchAndRadialDirections)
		{
			const VEC3 launch = AreaCalcLaunchDirection({ 0.0f, -3.0f, 2.0f }, 1.0f);
			Assert::AreEqual(1.0f, Vec3Length(launch), AREA_EPSILON);
			Assert::AreEqual(launch.Y, launch.Z, AREA_EPSILON);
			const VEC3 radial = AreaCalcRadialDirection({ 0.0f, 0.0f, 0.0f }, AreaBody(-2.0f, 0.0f), 0.0f, { 0.0f, 0.0f, 1.0f });
			Assert::AreEqual(-1.0f, radial.X, AREA_EPSILON);
			const VEC3 turned = AreaCalcRotateYaw({ 0.0f, 0.0f, 1.0f }, 3.14159265f / 2.0f);
			Assert::AreEqual(1.0f, turned.X, AREA_EPSILON);
		}

		// 線に並んだ相手のうち近い方が選ばれ、つかめない相手は飛ばす。
		TEST_METHOD(FirstOnRay_PicksNearestAvailable)
		{
			TENTACLE_TARGET targets[3] = {
				{ AreaBody(0.0f, 8.0f), true }, { AreaBody(0.0f, 4.0f), true }, { AreaBody(0.0f, 2.0f), false }
			};
			float distance = -1.0f;
			Assert::AreEqual(1, AreaCalcFirstOnRay({ 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, 20.0f, 0.2f, targets, 3, &distance));
			Assert::AreEqual(4.0f, distance, 0.01f);
			Assert::AreEqual(-1, AreaCalcFirstOnRay({ 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f }, 20.0f, 0.2f, targets, 3, nullptr));
		}

		// 扇に並べた線は、近い相手ほど多く当たる。
		TEST_METHOD(CountRays_MoreHitsWhenClose)
		{
			const VEC3 origin{ 0.0f, 1.0f, 0.0f };
			const VEC3 forward{ 0.0f, 0.0f, 1.0f };
			const float spread = 20.0f * 3.14159265f / 180.0f;
			const int near = AreaCalcCountRays(origin, forward, 7.5f, 0.18f, 7, spread, AreaBody(0.0f, 1.5f));
			const int far = AreaCalcCountRays(origin, forward, 7.5f, 0.18f, 7, spread, AreaBody(0.0f, 6.0f));
			Assert::AreEqual(7, near);
			Assert::IsTrue(far >= 1 && far < near);
		}
	};
}
