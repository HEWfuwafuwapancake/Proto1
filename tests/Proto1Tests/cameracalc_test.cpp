// physics/cameracalc.h（TPS カメラの位置・浮島にめり込まない・なめらかに追う）の単体テスト
#include <math.h>
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

		// 見ている点が浮島の箱の中なら、近づけようがないのでそのまま
		TEST_METHOD(Clip_TargetInsideKeepsCamera)
		{
			VEC3 cam = CameraCalcClipToStage({ 0.0f, -1.0f, 0.0f }, { 0.0f, 3.0f, -8.0f }, MakeCameraStage(), 0.3f);
			Assert::AreEqual(3.0f, cam.Y, CAMERA_EPSILON);
			Assert::AreEqual(-8.0f, cam.Z, CAMERA_EPSILON);
		}

		// なめらかに追う：少しずつ近づき、行き過ぎず、1 秒たつと残りが e^(-speed) 倍になる
		TEST_METHOD(Smooth_ApproachesWithoutOvershoot)
		{
			VEC3 pos = { 0.0f, 0.0f, 0.0f };
			const VEC3 goal = { 10.0f, 0.0f, -4.0f };
			float previous = Distance(pos, goal);
			for (int i = 0; i < 60; i++) {
				pos = CameraCalcSmooth(pos, goal, 5.0f, 1.0f / 60.0f);
				const float now = Distance(pos, goal);
				Assert::IsTrue(now < previous);		// 毎フレーム近づく
				Assert::IsTrue(pos.X <= 10.0f);		// 行き過ぎない
				previous = now;
			}
			Assert::AreEqual(Distance({ 0.0f, 0.0f, 0.0f }, goal) * expf(-5.0f), previous, 1.0e-3f);
		}

		// dt が 0（時間が止まっている）なら動かない
		TEST_METHOD(Smooth_ZeroDtDoesNotMove)
		{
			VEC3 pos = CameraCalcSmooth({ 1.0f, 2.0f, 3.0f }, { 9.0f, 9.0f, 9.0f }, 5.0f, 0.0f);
			Assert::AreEqual(1.0f, pos.X, CAMERA_EPSILON);
			Assert::AreEqual(2.0f, pos.Y, CAMERA_EPSILON);
			Assert::AreEqual(3.0f, pos.Z, CAMERA_EPSILON);
		}
	};
}
