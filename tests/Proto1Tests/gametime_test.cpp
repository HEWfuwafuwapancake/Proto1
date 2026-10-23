// core/gametime.h（1フレームの秒数・経過時間・時間の進む速さ）の単体テスト
#include "CppUnitTest.h"
#include "core/gametime.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace Proto1Tests
{
	static constexpr float GAMETIME_EPSILON = 1.0e-5f;	// float の比較で許す誤差（足し算を重ねるので少し大きめ）

	TEST_CLASS(GameTimeTest)
	{
	public:
		// 時間は1つしかない（static）ので、テストのたびに最初の状態に戻す
		TEST_METHOD_INITIALIZE(Reset)
		{
			GameTimeInitialize();
		}

		// 1フレームは 1/60 秒、経過時間は 0 から始まる
		TEST_METHOD(DeltaTime_IsOneSixtieth)
		{
			Assert::AreEqual(1.0f / 60.0f, GameTimeGetDeltaTime(), GAMETIME_EPSILON);
			Assert::AreEqual(0.0f, GameTimeGetElapsed(), GAMETIME_EPSILON);
			Assert::AreEqual(1.0f, GameTimeGetScale(), GAMETIME_EPSILON);
		}

		// 60 フレーム進めると、ちょうど 1 秒たつ
		TEST_METHOD(Elapsed_OneSecondAfter60Frames)
		{
			for (int i = 0; i < 60; i++) {
				GameTimeUpdate();
			}
			Assert::AreEqual(1.0f, GameTimeGetElapsed(), GAMETIME_EPSILON);
		}

		// 速さ 0 の間は、dt も 0 で、経過時間も増えない
		TEST_METHOD(ScaleZero_StopsTime)
		{
			GameTimeUpdate();
			const float before = GameTimeGetElapsed();

			GameTimeSetScale(0.0f);
			for (int i = 0; i < 10; i++) {
				GameTimeUpdate();
			}
			Assert::AreEqual(0.0f, GameTimeGetDeltaTime(), GAMETIME_EPSILON);
			Assert::AreEqual(before, GameTimeGetElapsed(), GAMETIME_EPSILON);
		}

		// 速さ 0.5 なら、dt も経過時間も半分
		TEST_METHOD(ScaleHalf_HalvesTime)
		{
			GameTimeSetScale(0.5f);
			Assert::AreEqual(0.5f / 60.0f, GameTimeGetDeltaTime(), GAMETIME_EPSILON);
			for (int i = 0; i < 60; i++) {
				GameTimeUpdate();
			}
			Assert::AreEqual(0.5f, GameTimeGetElapsed(), GAMETIME_EPSILON);
		}

		// 負の速さは 0 にする（時間は戻らない）
		TEST_METHOD(NegativeScale_BecomesZero)
		{
			GameTimeSetScale(-1.0f);
			Assert::AreEqual(0.0f, GameTimeGetScale(), GAMETIME_EPSILON);
			GameTimeUpdate();
			Assert::AreEqual(0.0f, GameTimeGetElapsed(), GAMETIME_EPSILON);
		}

		// Initialize で経過時間と速さが最初に戻る
		TEST_METHOD(Initialize_ResetsElapsedAndScale)
		{
			GameTimeSetScale(2.0f);
			GameTimeUpdate();
			GameTimeInitialize();
			Assert::AreEqual(0.0f, GameTimeGetElapsed(), GAMETIME_EPSILON);
			Assert::AreEqual(1.0f, GameTimeGetScale(), GAMETIME_EPSILON);
		}
	};
}
