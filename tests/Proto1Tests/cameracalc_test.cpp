// physics/cameracalc.h（TPS カメラの位置・浮島にめり込まない・なめらかに追う）の単体テスト
#include <math.h>
#include <initializer_list>
#include <limits>
#include "CppUnitTest.h"
#include "physics/cameracalc.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace Proto1Tests
{
	static constexpr float CAMERA_EPSILON = 1.0e-4f;	// float の比較で許す誤差
	static constexpr float CAMERA_PI = 3.14159265f;

	// ゲームと同じ形の浮島（30 × 30、上の面 y = 0、厚さ 2）
	static STAGE MakeCameraStage(void)
	{
		STAGE stage{};
		stage.HalfWidth = 15.0f;
		stage.HalfDepth = 15.0f;
		stage.TopY = 0.0f;
		stage.Thickness = 2.0f;
		stage.OutY = -15.0f;
		return stage;
	}

	static float Distance(const VEC3& a, const VEC3& b)
	{
		const float x = a.X - b.X, y = a.Y - b.Y, z = a.Z - b.Z;
		return sqrtf(x * x + y * y + z * z);
	}

	TEST_CLASS(CameraCalcTest)
	{
	public:
		// 向き 0・水平なら、+Z を見ているので、カメラは見ている点の -Z 側に distance 離れる
		TEST_METHOD(Orbit_StraightBehind)
		{
			VEC3 pos = CameraCalcOrbitPosition({ 1.0f, 2.0f, 3.0f }, 0.0f, 0.0f, 8.0f);
			Assert::AreEqual(1.0f, pos.X, CAMERA_EPSILON);
			Assert::AreEqual(2.0f, pos.Y, CAMERA_EPSILON);
			Assert::AreEqual(-5.0f, pos.Z, CAMERA_EPSILON);
		}

		// 右に 90 度回ると +X を見るので、カメラは -X 側。下を見ると、カメラは上にある
		TEST_METHOD(Orbit_TurnedAndLookingDown)
		{
			VEC3 right = CameraCalcOrbitPosition({ 0.0f, 0.0f, 0.0f }, CAMERA_PI * 0.5f, 0.0f, 8.0f);
			Assert::AreEqual(-8.0f, right.X, CAMERA_EPSILON);
			Assert::AreEqual(0.0f, right.Z, CAMERA_EPSILON);

			VEC3 down = CameraCalcOrbitPosition({ 0.0f, 0.0f, 0.0f }, 0.0f, -CAMERA_PI / 6.0f, 8.0f);
			Assert::AreEqual(4.0f, down.Y, CAMERA_EPSILON);	// 8 × sin(30°)
			Assert::AreEqual(8.0f, Distance(down, { 0.0f, 0.0f, 0.0f }), CAMERA_EPSILON);
		}

		// 線が浮島にぶつからなければ、そのまま
		TEST_METHOD(Clip_NoHitKeepsCamera)
		{
			VEC3 cam = CameraCalcClipToStage({ 0.0f, 1.5f, 0.0f }, { 0.0f, 5.0f, -8.0f }, MakeCameraStage(), 0.3f);
			Assert::AreEqual(0.0f, cam.X, CAMERA_EPSILON);
			Assert::AreEqual(5.0f, cam.Y, CAMERA_EPSILON);
			Assert::AreEqual(-8.0f, cam.Z, CAMERA_EPSILON);
		}

		// 見上げてカメラが浮島の下に行くときは、上の面（＋margin）の高さまで近づける
		TEST_METHOD(Clip_LookingUpStopsAboveTop)
		{
			VEC3 target = { 0.0f, 1.5f, 0.0f };
			VEC3 cam = CameraCalcClipToStage(target, { 0.0f, -2.5f, -8.0f }, MakeCameraStage(), 0.3f);
			Assert::AreEqual(0.3f, cam.Y, CAMERA_EPSILON);
			// 線の上にある（向きは変わらず、近づいただけ）
			Assert::AreEqual(0.0f, cam.X, CAMERA_EPSILON);
			Assert::AreEqual(-8.0f * (1.2f / 4.0f), cam.Z, CAMERA_EPSILON);
		}

		// 浮島の外で下から見上げるとき、線が浮島の横の面にぶつかるなら、横の面の手前で止まる
		TEST_METHOD(Clip_HitsSideFace)
		{
			VEC3 target = { 20.0f, -1.0f, 0.0f };	// 浮島の右の外、上の面より少し下
			VEC3 cam = CameraCalcClipToStage(target, { 10.0f, -1.0f, 0.0f }, MakeCameraStage(), 0.3f);
			Assert::AreEqual(15.3f, cam.X, CAMERA_EPSILON);
			Assert::AreEqual(-1.0f, cam.Y, CAMERA_EPSILON);
		}

		// 正負どちらの目標へも近づく。大きな持ち越し速度でも目標を通り過ぎない。
		TEST_METHOD(SmoothDamp_ApproachesWithoutOvershoot)
		{
			for (float goal : {10.0f, -10.0f}) {
				float pos = 0.0f, velocity = 0.0f;
				for (int i = 0; i < 60; ++i) {
					const float previous = pos;
					pos = CameraCalcSmoothDamp(pos, goal, velocity, 0.3f, 1.0f / 60.0f);
					Assert::IsTrue(fabsf(goal - pos) <= fabsf(goal - previous));
					Assert::IsTrue(pos * goal >= 0.0f && fabsf(pos) <= fabsf(goal));
				}
				Assert::AreEqual(goal, pos, 0.11f);
				velocity = goal * 1000.0f;
				Assert::AreEqual(goal, CameraCalcSmoothDamp(0, goal, velocity, 0.3f, 1.0f / 30.0f), CAMERA_EPSILON);
				Assert::AreEqual(0.0f, velocity, CAMERA_EPSILON);
			}
		}

		TEST_METHOD(SmoothDampAngle_CrossesPiInBothDirectionsByShortestPath)
		{
			for (float sign : {1.0f, -1.0f}) {
				float angle = sign * (CAMERA_PI - 0.1f), velocity = 0.0f;
				const float goal = sign * (-CAMERA_PI + 0.1f);
				float previous = 0.2f;
				for (int i = 0; i < 60; ++i) {
					angle = CameraCalcSmoothDampAngle(angle, goal, velocity, 0.12f, 1.0f / 60.0f);
					const float gap = fabsf(atan2f(sinf(goal - angle), cosf(goal - angle)));
					Assert::IsTrue(gap <= previous + CAMERA_EPSILON);
					Assert::IsTrue(fabsf(angle) >= CAMERA_PI - 0.101f);
					Assert::IsTrue(velocity * sign >= -CAMERA_EPSILON);
					previous = gap;
				}
				Assert::AreEqual(goal, angle, CAMERA_EPSILON);
			}
		}

		// 1秒後の値と速度を比較。許容差は移動量10に対して0.01（0.1%）。
		TEST_METHOD(SmoothDamp_FrameRatesReachNearlySameValueAndVelocity)
		{
			float value60 = 0, value30 = 0, velocity60 = 0, velocity30 = 0;
			float angle60 = 3, angle30 = 3, angularVelocity60 = 0, angularVelocity30 = 0;
			for (int i = 0; i < 60; ++i) {
				value60 = CameraCalcSmoothDamp(value60, 10, velocity60, 0.5f, 1.0f / 60.0f);
				angle60 = CameraCalcSmoothDampAngle(angle60, -3, angularVelocity60, 0.5f, 1.0f / 60.0f);
			}
			for (int i = 0; i < 30; ++i) {
				value30 = CameraCalcSmoothDamp(value30, 10, velocity30, 0.5f, 1.0f / 30.0f);
				angle30 = CameraCalcSmoothDampAngle(angle30, -3, angularVelocity30, 0.5f, 1.0f / 30.0f);
			}
			Assert::AreEqual(value60, value30, 0.01f);
			Assert::AreEqual(velocity60, velocity30, 0.01f);
			Assert::AreEqual(angle60, angle30, 0.001f);
			Assert::AreEqual(angularVelocity60, angularVelocity30, 0.001f);
		}

		TEST_METHOD(SmoothDamp_ChangingGoalCarriesVelocity)
		{
			float value = 0, velocity = 0;
			for (int i = 0; i < 6; ++i) {
				value = CameraCalcSmoothDamp(value, 10, velocity, 0.3f, 1.0f / 60.0f);
			}
			const float beforeVelocity = velocity;
			float resetVelocity = 0;
			const float restarted = CameraCalcSmoothDamp(value, -10, resetVelocity, 0.3f, 1.0f / 60.0f);
			const float continued = CameraCalcSmoothDamp(value, -10, velocity, 0.3f, 1.0f / 60.0f);
			Assert::IsTrue(beforeVelocity > 0 && velocity > 0);
			Assert::IsTrue(continued > value && restarted < value);
			Assert::IsTrue(continued - restarted > 0.1f);
		}

		TEST_METHOD(Focus_ChoosesClosestAngleAndKeepsSmallPriority)
		{
			CAMERA_FOCUS_TARGET targets[]{ {{2, 0, 10}, true}, {{1.5f, 0, 10}, true} };
			Assert::AreEqual(1, CameraCalcPickFocus(0, {}, targets, 2, -1, 0.5f, 20, 0.06f));
			Assert::AreEqual(0, CameraCalcPickFocus(0, {}, targets, 2, 0, 0.5f, 20, 0.06f));
			targets[1].Position.X = 0;
			Assert::AreEqual(1, CameraCalcPickFocus(0, {}, targets, 2, 0, 0.5f, 20, 0.06f));
		}
		TEST_METHOD(Focus_RejectsAngleDistanceUnavailableAndEmpty)
		{
			CAMERA_FOCUS_TARGET targets[]{ {{8, 0, 1}, true}, {{0, 0, 30}, true}, {{0, 0, 5}, false} };
			Assert::AreEqual(-1, CameraCalcPickFocus(0, {}, targets, 3, 0, 0.5f, 20, 2));
			Assert::AreEqual(-1, CameraCalcPickFocus(0, {}, nullptr, 0, -1, 0.5f, 20, 0));
			targets[1].Position = {0, 30, 1};
			Assert::AreEqual(-1, CameraCalcPickFocus(0, {}, targets, 3, 1, 0.5f, 20, 2));
		}
		// 候補の並び・距離ではなく、現在の敵から指定側の最小角を選ぶ。
		TEST_METHOD(NextTarget_ChoosesNearestAngleOnRightAndLeft)
		{
			const VEC3 player{ 3, 2, -4 };
			CAMERA_FOCUS_TARGET targets[]{
				{{3, 2, 6}, true}, {{13, 2, -4}, true}, {{5, 2, 6}, true},
				{{-7, 2, -4}, true}, {{1, 2, 6}, true}
			};
			Assert::AreEqual(2, CameraCalcNextTarget(player, targets, 5, 0, 1));
			Assert::AreEqual(4, CameraCalcNextTarget(player, targets, 5, 0, -1));
			Assert::AreEqual(0, CameraCalcNextTarget(player, targets, 5, 0, 0));
		}

		TEST_METHOD(NextTarget_NoEnemyOnThatSideKeepsCurrent)
		{
			CAMERA_FOCUS_TARGET targets[]{ {{0, 0, 10}, true}, {{3, 0, 10}, true} };
			Assert::AreEqual(0, CameraCalcNextTarget({}, targets, 2, 0, -1));
			Assert::AreEqual(1, CameraCalcNextTarget({}, targets, 2, 1, 1));
			Assert::AreEqual(0, CameraCalcNextTarget({}, targets, 1, 0, 1));
		}

		TEST_METHOD(NextTarget_SkipsUnavailableSameBearingAndVerticalEnemy)
		{
			CAMERA_FOCUS_TARGET targets[]{
				{{0, 0, 10}, true}, {{1, 0, 10}, false}, {{4, 0, 10}, true},
				{{0, 0, 20}, true}, {{0, 4, 0}, true}, {{-2, 0, 10}, false}
			};
			Assert::AreEqual(2, CameraCalcNextTarget({}, targets, 6, 0, 1));
			Assert::AreEqual(0, CameraCalcNextTarget({}, targets, 6, 0, -1));
		}

		// No.33 の開幕・自動再選択は角度・距離で絞らない。
		TEST_METHOD(Focus_UnlimitedSelectionIncludesBehindAndDistantEnemies)
		{
			const float unlimited = (std::numeric_limits<float>::infinity)();
			CAMERA_FOCUS_TARGET targets[]{ {{0, 0, -10000}, true}, {{10000, 0, -10000}, true} };
			Assert::AreEqual(1, CameraCalcPickFocus(0, {}, targets, 2, -1, CAMERA_PI, unlimited, 0));
			targets[1].Available = false;
			Assert::AreEqual(0, CameraCalcPickFocus(0, {}, targets, 2, -1, CAMERA_PI, unlimited, 0));
			targets[0].Available = false;
			Assert::AreEqual(-1, CameraCalcPickFocus(0, {}, targets, 2, -1, CAMERA_PI, unlimited, 0));
		}

		TEST_METHOD(YawTo_UsesHorizontalDirectionAndNearFallback)
		{
			Assert::AreEqual(CAMERA_PI * 0.5f, CameraCalcYawTo({}, {10, 7, 0}, 0), CAMERA_EPSILON);
			Assert::AreEqual(1.2f, CameraCalcYawTo({}, {0.00001f, 10, 0}, 1.2f), CAMERA_EPSILON);
		}
		TEST_METHOD(FocusPoint_UsesRequestedRateForNearbyEnemy)
		{
			const VEC3 p = CameraCalcFocusPoint({1,2,3}, {3,2,5}, 0, 0, 8, CAMERA_PI/3, 1.7f, 0.15f, 1.5f, 0.25f);
			Assert::AreEqual(1.5f, p.X, CAMERA_EPSILON);
			Assert::AreEqual(2.0f, p.Y, CAMERA_EPSILON);
			Assert::AreEqual(3.5f, p.Z, CAMERA_EPSILON);
		}
		TEST_METHOD(FocusPoint_KeepsWholePlayerInsideBothScreenAxes)
		{
			for (float aspect : {0.6f, 1.777f}) {
				for (VEC3 enemy : {VEC3{100, 0, 10}, VEC3{0, 60, 100}, VEC3{-80, -40, 100}}) {
					const float pitch = -0.6f, radius = 1.6f, distance = 8;
					const VEC3 point = CameraCalcFocusPoint({}, enemy, 0, pitch, distance, CAMERA_PI/3, aspect, 0.15f, radius, 0.4f);
					Assert::IsTrue(Vec3Length(point) < Vec3Length(enemy) * 0.4f);
					const VEC3 eye = CameraCalcOrbitPosition(point, 0, pitch, distance);
					const VEC3 relative = Vec3Scale(eye, -1);
					const float depth = relative.Y * sinf(pitch) + relative.Z * cosf(pitch);
					const float vertical = relative.Y * cosf(pitch) - relative.Z * sinf(pitch);
					const float tanY = tanf(CAMERA_PI/6) * 0.85f;
					Assert::IsTrue(fabsf(relative.X) + radius * sqrtf(1 + tanY*tanY*aspect*aspect) <= depth*tanY*aspect + CAMERA_EPSILON);
					Assert::IsTrue(fabsf(vertical) + radius * sqrtf(1 + tanY*tanY) <= depth*tanY + CAMERA_EPSILON);
				}
			}
		}
	};
}
