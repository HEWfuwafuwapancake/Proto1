// physics/cameracalc.h（TPS カメラの位置・浮島にめり込まない・なめらかに追う）の単体テスト
#include <math.h>
#include <initializer_list>
#include <limits>
#include <gtest/gtest.h>
#include "physics/cameracalc.h"

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

	// 向き 0・水平なら、+Z を見ているので、カメラは見ている点の -Z 側に distance 離れる
	TEST(CameraCalcTest, Orbit_StraightBehind)
	{
		VEC3 pos = CameraCalcOrbitPosition({ 1.0f, 2.0f, 3.0f }, 0.0f, 0.0f, 8.0f);
		EXPECT_NEAR(1.0f, pos.X, CAMERA_EPSILON);
		EXPECT_NEAR(2.0f, pos.Y, CAMERA_EPSILON);
		EXPECT_NEAR(-5.0f, pos.Z, CAMERA_EPSILON);
	}

	// 右に 90 度回ると +X を見るので、カメラは -X 側。下を見ると、カメラは上にある
	TEST(CameraCalcTest, Orbit_TurnedAndLookingDown)
	{
		VEC3 right = CameraCalcOrbitPosition({ 0.0f, 0.0f, 0.0f }, CAMERA_PI * 0.5f, 0.0f, 8.0f);
		EXPECT_NEAR(-8.0f, right.X, CAMERA_EPSILON);
		EXPECT_NEAR(0.0f, right.Z, CAMERA_EPSILON);

		VEC3 down = CameraCalcOrbitPosition({ 0.0f, 0.0f, 0.0f }, 0.0f, -CAMERA_PI / 6.0f, 8.0f);
		EXPECT_NEAR(4.0f, down.Y, CAMERA_EPSILON);	// 8 × sin(30°)
		EXPECT_NEAR(8.0f, Distance(down, { 0.0f, 0.0f, 0.0f }), CAMERA_EPSILON);
	}

	// 線が浮島にぶつからなければ、そのまま
	TEST(CameraCalcTest, Clip_NoHitKeepsCamera)
	{
		VEC3 cam = CameraCalcClipToStage({ 0.0f, 1.5f, 0.0f }, { 0.0f, 5.0f, -8.0f }, MakeCameraStage(), 0.3f);
		EXPECT_NEAR(0.0f, cam.X, CAMERA_EPSILON);
		EXPECT_NEAR(5.0f, cam.Y, CAMERA_EPSILON);
		EXPECT_NEAR(-8.0f, cam.Z, CAMERA_EPSILON);
	}

	// 見上げてカメラが浮島の下に行くときは、上の面（＋margin）の高さまで近づける
	TEST(CameraCalcTest, Clip_LookingUpStopsAboveTop)
	{
		VEC3 target = { 0.0f, 1.5f, 0.0f };
		VEC3 cam = CameraCalcClipToStage(target, { 0.0f, -2.5f, -8.0f }, MakeCameraStage(), 0.3f);
		EXPECT_NEAR(0.3f, cam.Y, CAMERA_EPSILON);
		// 線の上にある（向きは変わらず、近づいただけ）
		EXPECT_NEAR(0.0f, cam.X, CAMERA_EPSILON);
		EXPECT_NEAR(-8.0f * (1.2f / 4.0f), cam.Z, CAMERA_EPSILON);
	}

	// 浮島の外で下から見上げるとき、線が浮島の横の面にぶつかるなら、横の面の手前で止まる
	TEST(CameraCalcTest, Clip_HitsSideFace)
	{
		VEC3 target = { 20.0f, -1.0f, 0.0f };	// 浮島の右の外、上の面より少し下
		VEC3 cam = CameraCalcClipToStage(target, { 10.0f, -1.0f, 0.0f }, MakeCameraStage(), 0.3f);
		EXPECT_NEAR(15.3f, cam.X, CAMERA_EPSILON);
		EXPECT_NEAR(-1.0f, cam.Y, CAMERA_EPSILON);
	}

	// 正負どちらの目標へも近づく。大きな持ち越し速度でも目標を通り過ぎない。
	TEST(CameraCalcTest, SmoothDamp_ApproachesWithoutOvershoot)
	{
		for (float goal : {10.0f, -10.0f}) {
			float pos = 0.0f, velocity = 0.0f;
			for (int i = 0; i < 60; ++i) {
				const float previous = pos;
				pos = CameraCalcSmoothDamp(pos, goal, velocity, 0.3f, 1.0f / 60.0f);
				EXPECT_TRUE(fabsf(goal - pos) <= fabsf(goal - previous));
				EXPECT_TRUE(pos * goal >= 0.0f && fabsf(pos) <= fabsf(goal));
			}
			EXPECT_NEAR(goal, pos, 0.11f);
			velocity = goal * 1000.0f;
			EXPECT_NEAR(goal, CameraCalcSmoothDamp(0, goal, velocity, 0.3f, 1.0f / 30.0f), CAMERA_EPSILON);
			EXPECT_NEAR(0.0f, velocity, CAMERA_EPSILON);
		}
	}

	TEST(CameraCalcTest, SmoothDampAngle_CrossesPiInBothDirectionsByShortestPath)
	{
		for (float sign : {1.0f, -1.0f}) {
			float angle = sign * (CAMERA_PI - 0.1f), velocity = 0.0f;
			const float goal = sign * (-CAMERA_PI + 0.1f);
			float previous = 0.2f;
			for (int i = 0; i < 60; ++i) {
				angle = CameraCalcSmoothDampAngle(angle, goal, velocity, 0.12f, 1.0f / 60.0f);
				const float gap = fabsf(atan2f(sinf(goal - angle), cosf(goal - angle)));
				EXPECT_TRUE(gap <= previous + CAMERA_EPSILON);
				EXPECT_TRUE(fabsf(angle) >= CAMERA_PI - 0.101f);
				EXPECT_TRUE(velocity * sign >= -CAMERA_EPSILON);
				previous = gap;
			}
			EXPECT_NEAR(goal, angle, CAMERA_EPSILON);
		}
	}

	// 1秒後の値と速度を比較。許容差は移動量10に対して0.01（0.1%）。
	TEST(CameraCalcTest, SmoothDamp_FrameRatesReachNearlySameValueAndVelocity)
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
		EXPECT_NEAR(value60, value30, 0.01f);
		EXPECT_NEAR(velocity60, velocity30, 0.01f);
		EXPECT_NEAR(angle60, angle30, 0.001f);
		EXPECT_NEAR(angularVelocity60, angularVelocity30, 0.001f);
	}

	TEST(CameraCalcTest, SmoothDamp_ChangingGoalCarriesVelocity)
	{
		float value = 0, velocity = 0;
		for (int i = 0; i < 6; ++i) {
			value = CameraCalcSmoothDamp(value, 10, velocity, 0.3f, 1.0f / 60.0f);
		}
		const float beforeVelocity = velocity;
		float resetVelocity = 0;
		const float restarted = CameraCalcSmoothDamp(value, -10, resetVelocity, 0.3f, 1.0f / 60.0f);
		const float continued = CameraCalcSmoothDamp(value, -10, velocity, 0.3f, 1.0f / 60.0f);
		EXPECT_TRUE(beforeVelocity > 0 && velocity > 0);
		EXPECT_TRUE(continued > value && restarted < value);
		EXPECT_TRUE(continued - restarted > 0.1f);
	}

	TEST(CameraCalcTest, Focus_ChoosesClosestAngleAndKeepsSmallPriority)
	{
		CAMERA_FOCUS_TARGET targets[]{ {{2, 0, 10}, true}, {{1.5f, 0, 10}, true} };
		EXPECT_EQ(1, CameraCalcPickFocus(0, {}, targets, 2, -1, 0.5f, 20, 0.06f));
		EXPECT_EQ(0, CameraCalcPickFocus(0, {}, targets, 2, 0, 0.5f, 20, 0.06f));
		targets[1].Position.X = 0;
		EXPECT_EQ(1, CameraCalcPickFocus(0, {}, targets, 2, 0, 0.5f, 20, 0.06f));
	}

	TEST(CameraCalcTest, Focus_RejectsAngleDistanceUnavailableAndEmpty)
	{
		CAMERA_FOCUS_TARGET targets[]{ {{8, 0, 1}, true}, {{0, 0, 30}, true}, {{0, 0, 5}, false} };
		EXPECT_EQ(-1, CameraCalcPickFocus(0, {}, targets, 3, 0, 0.5f, 20, 2));
		EXPECT_EQ(-1, CameraCalcPickFocus(0, {}, nullptr, 0, -1, 0.5f, 20, 0));
		targets[1].Position = {0, 30, 1};
		EXPECT_EQ(-1, CameraCalcPickFocus(0, {}, targets, 3, 1, 0.5f, 20, 2));
	}

	// 候補の並び・距離ではなく、現在の敵から指定側の最小角を選ぶ。
	TEST(CameraCalcTest, NextTarget_ChoosesNearestAngleOnRightAndLeft)
	{
		const VEC3 player{ 3, 2, -4 };
		CAMERA_FOCUS_TARGET targets[]{
			{{3, 2, 6}, true}, {{13, 2, -4}, true}, {{5, 2, 6}, true},
			{{-7, 2, -4}, true}, {{1, 2, 6}, true}
		};
		EXPECT_EQ(2, CameraCalcNextTarget(player, targets, 5, 0, 1));
		EXPECT_EQ(4, CameraCalcNextTarget(player, targets, 5, 0, -1));
		EXPECT_EQ(0, CameraCalcNextTarget(player, targets, 5, 0, 0));
	}

	TEST(CameraCalcTest, NextTarget_NoEnemyOnThatSideKeepsCurrent)
	{
		CAMERA_FOCUS_TARGET targets[]{ {{0, 0, 10}, true}, {{3, 0, 10}, true} };
		EXPECT_EQ(0, CameraCalcNextTarget({}, targets, 2, 0, -1));
		EXPECT_EQ(1, CameraCalcNextTarget({}, targets, 2, 1, 1));
		EXPECT_EQ(0, CameraCalcNextTarget({}, targets, 1, 0, 1));
	}

	TEST(CameraCalcTest, NextTarget_SkipsUnavailableSameBearingAndVerticalEnemy)
	{
		CAMERA_FOCUS_TARGET targets[]{
			{{0, 0, 10}, true}, {{1, 0, 10}, false}, {{4, 0, 10}, true},
			{{0, 0, 20}, true}, {{0, 4, 0}, true}, {{-2, 0, 10}, false}
		};
		EXPECT_EQ(2, CameraCalcNextTarget({}, targets, 6, 0, 1));
		EXPECT_EQ(0, CameraCalcNextTarget({}, targets, 6, 0, -1));
	}

	// No.33 の開幕・自動再選択は角度・距離で絞らない。
	TEST(CameraCalcTest, Focus_UnlimitedSelectionIncludesBehindAndDistantEnemies)
	{
		const float unlimited = (std::numeric_limits<float>::infinity)();
		CAMERA_FOCUS_TARGET targets[]{ {{0, 0, -10000}, true}, {{10000, 0, -10000}, true} };
		EXPECT_EQ(1, CameraCalcPickFocus(0, {}, targets, 2, -1, CAMERA_PI, unlimited, 0));
		targets[1].Available = false;
		EXPECT_EQ(0, CameraCalcPickFocus(0, {}, targets, 2, -1, CAMERA_PI, unlimited, 0));
		targets[0].Available = false;
		EXPECT_EQ(-1, CameraCalcPickFocus(0, {}, targets, 2, -1, CAMERA_PI, unlimited, 0));
	}

	TEST(CameraCalcTest, YawTo_UsesHorizontalDirectionAndNearFallback)
	{
		EXPECT_NEAR(CAMERA_PI * 0.5f, CameraCalcYawTo({}, {10, 7, 0}, 0), CAMERA_EPSILON);
		EXPECT_NEAR(1.2f, CameraCalcYawTo({}, {0.00001f, 10, 0}, 1.2f), CAMERA_EPSILON);
	}

	TEST(CameraCalcTest, FocusPoint_UsesRequestedRateForNearbyEnemy)
	{
		const VEC3 p = CameraCalcFocusPoint({1,2,3}, {3,2,5}, 0, 0, 8, CAMERA_PI/3, 1.7f, 0.15f, 1.5f, 0.25f);
		EXPECT_NEAR(1.5f, p.X, CAMERA_EPSILON);
		EXPECT_NEAR(2.0f, p.Y, CAMERA_EPSILON);
		EXPECT_NEAR(3.5f, p.Z, CAMERA_EPSILON);
	}

	TEST(CameraCalcTest, FocusPoint_KeepsWholePlayerInsideBothScreenAxes)
	{
		for (float aspect : {0.6f, 1.777f}) {
			for (VEC3 enemy : {VEC3{100, 0, 10}, VEC3{0, 60, 100}, VEC3{-80, -40, 100}}) {
				const float pitch = -0.6f, radius = 1.6f, distance = 8;
				const VEC3 point = CameraCalcFocusPoint({}, enemy, 0, pitch, distance, CAMERA_PI/3, aspect, 0.15f, radius, 0.4f);
				EXPECT_TRUE(Vec3Length(point) < Vec3Length(enemy) * 0.4f);
				const VEC3 eye = CameraCalcOrbitPosition(point, 0, pitch, distance);
				const VEC3 relative = Vec3Scale(eye, -1);
				const float depth = relative.Y * sinf(pitch) + relative.Z * cosf(pitch);
				const float vertical = relative.Y * cosf(pitch) - relative.Z * sinf(pitch);
				const float tanY = tanf(CAMERA_PI/6) * 0.85f;
				EXPECT_TRUE(fabsf(relative.X) + radius * sqrtf(1 + tanY*tanY*aspect*aspect) <= depth*tanY*aspect + CAMERA_EPSILON);
				EXPECT_TRUE(fabsf(vertical) + radius * sqrtf(1 + tanY*tanY) <= depth*tanY + CAMERA_EPSILON);
			}
		}
	}

	// ロックオン中は右へ寄せ、フリーカメラでは真ん中。右へ動くとカメラを左へ残してキャラを右へ流す。
	TEST(CameraCalcTest, SideOffsetGoal_ShouldersOnLockAndDriftsWithMove)
	{
		EXPECT_NEAR(0.9f, CameraCalcSideOffsetGoal(true, 0.0f, 0.9f, 1.2f), CAMERA_EPSILON);
		EXPECT_NEAR(0.0f, CameraCalcSideOffsetGoal(false, 0.0f, 0.9f, 1.2f), CAMERA_EPSILON);
		EXPECT_NEAR(-0.3f, CameraCalcSideOffsetGoal(true, 1.0f, 0.9f, 1.2f), CAMERA_EPSILON);
		EXPECT_NEAR(1.2f, CameraCalcSideOffsetGoal(false, -1.0f, 0.9f, 1.2f), CAMERA_EPSILON);
	}

	// 正面が +Z なら右は +X、正面が +X なら右は -Z。
	TEST(CameraCalcTest, SideShift_MovesToCameraRight)
	{
		const VEC3 a = CameraCalcSideShift({1, 2, 3}, 0.0f, 2.0f);
		EXPECT_NEAR(3.0f, a.X, CAMERA_EPSILON);
		EXPECT_NEAR(2.0f, a.Y, CAMERA_EPSILON);
		EXPECT_NEAR(3.0f, a.Z, CAMERA_EPSILON);
		const VEC3 b = CameraCalcSideShift({0, 0, 0}, CAMERA_PI / 2, 2.0f);
		EXPECT_NEAR(0.0f, b.X, CAMERA_EPSILON);
		EXPECT_NEAR(-2.0f, b.Z, CAMERA_EPSILON);
	}

	// カメラの状態はハード → ソフト → フリー → ハードの順に回る。
	TEST(CameraCalcTest, NextModeCycles)
	{
		EXPECT_EQ(CAMERA_MODE_SOFT, CameraCalcNextMode(CAMERA_MODE_HARD));
		EXPECT_EQ(CAMERA_MODE_FREE, CameraCalcNextMode(CAMERA_MODE_SOFT));
		EXPECT_EQ(CAMERA_MODE_HARD, CameraCalcNextMode(CAMERA_MODE_FREE));
	}

	// ソフトロックは画面に映っている相手のうち真ん中に近い相手を選び、後ろ・画面の外・高台の上で画面の外の相手は選ばない。
	// 今狙っている相手は、少しだけ真ん中から離れていても選び続ける。
	TEST(CameraCalcTest, PickVisible_CenterMostOnScreen)
	{
		const float fov = CAMERA_PI / 3.0f;	// 縦 60°
		const float aspect = 16.0f / 9.0f;
		CAMERA_FOCUS_TARGET targets[4] = {
			{ { 0.0f, 0.0f, -10.0f }, true },	// 後ろ
			{ { 3.0f, 0.0f, 10.0f }, true },	// 右前（画面の中）
			{ { 30.0f, 0.0f, 10.0f }, true },	// 右の画面の外
			{ { 0.0f, 15.0f, 10.0f }, true },	// 上の画面の外（高台の上の高すぎる相手）
		};
		EXPECT_EQ(1, CameraCalcPickVisible({ 0, 0, 0 }, 0.0f, 0.0f, fov, aspect, 0.05f, 40.0f, targets, 4, -1, 0.0f));
		// 上を向けば、高いところの相手が画面に入って選ばれる。
		EXPECT_EQ(3, CameraCalcPickVisible({ 0, 0, 0 }, 0.0f, CAMERA_PI / 4.0f, fov, aspect, 0.05f, 40.0f, targets, 4, -1, 0.0f));
		// 上の画面の外の相手しかいなければ選ばない。
		EXPECT_EQ(-1, CameraCalcPickVisible({ 0, 0, 0 }, 0.0f, 0.0f, fov, aspect, 0.05f, 40.0f, &targets[3], 1, -1, 0.0f));
		// 遠すぎれば選ばない。
		EXPECT_EQ(-1, CameraCalcPickVisible({ 0, 0, 0 }, 0.0f, 0.0f, fov, aspect, 0.05f, 5.0f, targets, 4, -1, 0.0f));

		CAMERA_FOCUS_TARGET two[2] = { { { 1.0f, 0.0f, 10.0f }, true }, { { -2.0f, 0.0f, 10.0f }, true } };
		EXPECT_EQ(0, CameraCalcPickVisible({ 0, 0, 0 }, 0.0f, 0.0f, fov, aspect, 0.05f, 40.0f, two, 2, -1, 0.0f));
		EXPECT_EQ(1, CameraCalcPickVisible({ 0, 0, 0 }, 0.0f, 0.0f, fov, aspect, 0.05f, 40.0f, two, 2, 1, 0.25f));
	}

	// 地形のマップ：見ている点の後ろ（+Z）に、指定した高さの台を1つ置き、残りは高さ 0 の床
	static STAGE MakeTerrainStage(float blockMinZ, float blockMaxZ, float blockTop)
	{
		STAGE stage = MakeCameraStage();
		stage.HasTerrain = true;
		stage.GroundBottomY = -6.0f;
		stage.Regions[0] = { -3.0f, 3.0f, blockMinZ, blockMaxZ, 0.0f, blockTop, 0.0f, 0.0f, true, STAGE_SURFACE_CENTER };
		stage.Regions[1] = { -30.0f, 30.0f, -30.0f, 30.0f, 0.0f, 0.0f, 0.0f, 0.0f, true, STAGE_SURFACE_FLOOR };
		stage.RegionCount = 2;
		return stage;
	}

	static constexpr float TERRAIN_HEAD_Y = 1.44f;					// 頭の高さ（体の高さ 1.8 × 0.8）
	static constexpr float TERRAIN_YAW = CAMERA_PI;					// -Z を見る（カメラは +Z 側の台の上へ下がる）
	static constexpr float TERRAIN_PITCH = -12.0f * CAMERA_PI / 180.0f;	// ふだんの見下ろす角度
	static constexpr float TERRAIN_MIN_PITCH = -35.0f * CAMERA_PI / 180.0f;
	static constexpr float TERRAIN_STEP = 2.5f * CAMERA_PI / 180.0f;
	static constexpr float TERRAIN_DISTANCE = 4.5f;
	static constexpr float TERRAIN_MARGIN = 0.3f;
	static constexpr float TERRAIN_NEAR = 0.5f;

	// 不具合の再現：頭（1.44）より低い台（1.2）を背にしても、頭の中まで寄らずに距離を保つ（前は余白 0.3 のせいで距離 0 になった）
	TEST(CameraCalcTest, ClearDistance_LowStepBehindKeepsDistance)
	{
		const STAGE stage = MakeTerrainStage(0.0f, 4.0f, 1.2f);
		const float clear = CameraCalcClearDistance({ 0.0f, TERRAIN_HEAD_Y, -0.1f }, TERRAIN_YAW, TERRAIN_PITCH, TERRAIN_DISTANCE,
			stage, TERRAIN_MARGIN, TERRAIN_NEAR);
		EXPECT_NEAR(TERRAIN_DISTANCE, clear, CAMERA_EPSILON);
	}

	// 頭より少し高い台は、見下ろす角度を深くして上から越え、距離は保つ
	TEST(CameraCalcTest, ClearPitch_ClimbsOverLowTerrain)
	{
		const STAGE stage = MakeTerrainStage(1.0f, 3.0f, 1.5f);
		const VEC3 head{ 0.0f, TERRAIN_HEAD_Y, 0.0f };
		EXPECT_LT(CameraCalcClearDistance(head, TERRAIN_YAW, TERRAIN_PITCH, TERRAIN_DISTANCE, stage, TERRAIN_MARGIN, TERRAIN_NEAR), TERRAIN_DISTANCE);
		const float pitch = CameraCalcClearPitch(head, TERRAIN_YAW, TERRAIN_PITCH, TERRAIN_DISTANCE, TERRAIN_MIN_PITCH, TERRAIN_STEP,
			stage, TERRAIN_MARGIN, TERRAIN_NEAR);
		EXPECT_LT(pitch, TERRAIN_PITCH);
		EXPECT_NEAR(TERRAIN_DISTANCE, CameraCalcClearDistance(head, TERRAIN_YAW, pitch, TERRAIN_DISTANCE, stage, TERRAIN_MARGIN, TERRAIN_NEAR), CAMERA_EPSILON);
	}

	// 越えられない高い壁では、壁の手前まで近づける（壁の中には置かない）
	TEST(CameraCalcTest, ClearPitch_TallWallStopsBeforeWall)
	{
		const STAGE stage = MakeTerrainStage(2.0f, 10.0f, 6.0f);
		const VEC3 head{ 0.0f, TERRAIN_HEAD_Y, 0.0f };
		const float pitch = CameraCalcClearPitch(head, TERRAIN_YAW, TERRAIN_PITCH, TERRAIN_DISTANCE, TERRAIN_MIN_PITCH, TERRAIN_STEP,
			stage, TERRAIN_MARGIN, TERRAIN_NEAR);
		const float clear = CameraCalcClearDistance(head, TERRAIN_YAW, pitch, TERRAIN_DISTANCE, stage, TERRAIN_MARGIN, TERRAIN_NEAR);
		EXPECT_LT(clear, TERRAIN_DISTANCE);
		EXPECT_LT(CameraCalcOrbitPosition(head, TERRAIN_YAW, pitch, clear).Z, 2.0f);
	}

	// 画面の揺れは時間で 0 に戻り、揺れが止まるとずれも 0 になる
	TEST(CameraCalcTest, Shake_DecaysToZero)
	{
		CameraShakeCalc shake;
		shake.Add(0.5f);
		shake.Step(0.1f, 2.5f);
		EXPECT_NEAR(0.25f, shake.GetStrength(), CAMERA_EPSILON);
		shake.Step(1.0f, 2.5f);
		EXPECT_NEAR(0.0f, shake.GetStrength(), CAMERA_EPSILON);
		const VEC3 offset = CameraCalcShakeOffset(shake.GetStrength(), 0.37f, 0.4f);
		EXPECT_NEAR(0.0f, offset.X + offset.Y + offset.Z, CAMERA_EPSILON);
	}

	// 揺れを重ねても強さは 1 を超えず、ずれも振れ幅の中に収まる
	TEST(CameraCalcTest, Shake_StackingIsCapped)
	{
		CameraShakeCalc shake;
		shake.Add(0.6f);
		shake.Add(0.6f);
		EXPECT_NEAR(1.0f, shake.GetStrength(), CAMERA_EPSILON);
		const VEC3 offset = CameraCalcShakeOffset(shake.GetStrength(), 0.21f, 0.4f);
		EXPECT_LE(fabsf(offset.X), 0.4f + CAMERA_EPSILON);
		EXPECT_LE(fabsf(offset.Y), 0.4f + CAMERA_EPSILON);
		EXPECT_GT(fabsf(offset.X) + fabsf(offset.Y) + fabsf(offset.Z), 0.0f);
	}
}
