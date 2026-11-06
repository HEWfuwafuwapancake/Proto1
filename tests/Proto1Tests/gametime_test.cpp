// core/gametime.h（1フレームの秒数・経過時間・時間の進む速さ）の単体テスト
#include <gtest/gtest.h>
#include "core/gametime.h"

namespace Proto1Tests
{
	static constexpr float GAMETIME_EPSILON = 1.0e-5f;	// float の比較で許す誤差（足し算を重ねるので少し大きめ）

	class GameTimeTest : public ::testing::Test
	{
	protected:
		// 時間は1つしかない（static）ので、テストのたびに最初の状態に戻す
		void SetUp() override
		{
			GameTimeInitialize();
		}
	};

	// 1フレームは 1/60 秒、経過時間は 0 から始まる
	TEST_F(GameTimeTest, DeltaTime_IsOneSixtieth)
	{
		EXPECT_NEAR(1.0f / 60.0f, GameTimeGetDeltaTime(), GAMETIME_EPSILON);
		EXPECT_NEAR(0.0f, GameTimeGetElapsed(), GAMETIME_EPSILON);
		EXPECT_NEAR(1.0f, GameTimeGetScale(), GAMETIME_EPSILON);
	}

	// 60 フレーム進めると、ちょうど 1 秒たつ
	TEST_F(GameTimeTest, Elapsed_OneSecondAfter60Frames)
	{
		for (int i = 0; i < 60; i++) {
			GameTimeUpdate();
		}
		EXPECT_NEAR(1.0f, GameTimeGetElapsed(), GAMETIME_EPSILON);
	}

	// 速さ 0 の間は、dt も 0 で、経過時間も増えない
	TEST_F(GameTimeTest, ScaleZero_StopsTime)
	{
		GameTimeUpdate();
		const float before = GameTimeGetElapsed();

		GameTimeSetScale(0.0f);
		for (int i = 0; i < 10; i++) {
			GameTimeUpdate();
		}
		EXPECT_NEAR(0.0f, GameTimeGetDeltaTime(), GAMETIME_EPSILON);
		EXPECT_NEAR(before, GameTimeGetElapsed(), GAMETIME_EPSILON);
	}

	// 速さ 0.5 なら、dt も経過時間も半分
	TEST_F(GameTimeTest, ScaleHalf_HalvesTime)
	{
		GameTimeSetScale(0.5f);
		EXPECT_NEAR(0.5f / 60.0f, GameTimeGetDeltaTime(), GAMETIME_EPSILON);
		for (int i = 0; i < 60; i++) {
			GameTimeUpdate();
		}
		EXPECT_NEAR(0.5f, GameTimeGetElapsed(), GAMETIME_EPSILON);
	}
}
