// physics/stagecalc.h（浮島の上にいるか・場外に落ちたか）の単体テスト
#include "CppUnitTest.h"
#include "physics/stagecalc.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace Proto1Tests
{
	// ゲームと同じ形の浮島（30 × 30、上の面 y = 0、下の線 y = -15）。中心だけずらして、中心を使っているかも確かめる
	static STAGE MakeStage(void)
	{
		STAGE stage{};
		stage.CenterX = 2.0f;
		stage.CenterZ = -1.0f;
		stage.HalfWidth = 15.0f;
		stage.HalfDepth = 15.0f;
		stage.TopY = 0.0f;
		stage.Thickness = 2.0f;
		stage.OutY = -15.0f;
		return stage;
	}

	TEST_CLASS(StageCalcTest)
	{
	public:
		// 浮島の真ん中は「浮島がある」
		TEST_METHOD(OnPlatform_Center)
		{
			STAGE stage = MakeStage();
			Assert::IsTrue(StageCalcIsOnPlatform(stage, 2.0f, -1.0f));
		}

		// 端ちょうどは「ある」、少しでも外は「ない」（4つの辺すべて）
		TEST_METHOD(OnPlatform_EdgesAndOutside)
		{
			STAGE stage = MakeStage();
			Assert::IsTrue(StageCalcIsOnPlatform(stage, 17.0f, -1.0f));		// 右の端
			Assert::IsTrue(StageCalcIsOnPlatform(stage, -13.0f, -1.0f));	// 左の端
			Assert::IsTrue(StageCalcIsOnPlatform(stage, 2.0f, 14.0f));		// 奥の端
			Assert::IsTrue(StageCalcIsOnPlatform(stage, 2.0f, -16.0f));		// 手前の端

			Assert::IsFalse(StageCalcIsOnPlatform(stage, 17.1f, -1.0f));
			Assert::IsFalse(StageCalcIsOnPlatform(stage, -13.1f, -1.0f));
			Assert::IsFalse(StageCalcIsOnPlatform(stage, 2.0f, 14.1f));
			Assert::IsFalse(StageCalcIsOnPlatform(stage, 2.0f, -16.1f));
		}

		// 下の線より上は場外でない。ちょうどもまだ場外でない。下なら場外
		TEST_METHOD(IsOut_BottomLine)
		{
			STAGE stage = MakeStage();
			Assert::IsFalse(StageCalcIsOut(stage, 0.0f));
			Assert::IsFalse(StageCalcIsOut(stage, -14.9f));
			Assert::IsFalse(StageCalcIsOut(stage, -15.0f));
			Assert::IsTrue(StageCalcIsOut(stage, -15.1f));
		}

		// 横や上に飛ばされても、高さが下の線より上なら、まだ場外でない（場外の線は下だけ）
		TEST_METHOD(IsOut_FarSideOrHighIsNotOut)
		{
			STAGE stage = MakeStage();
			Assert::IsFalse(StageCalcIsOnPlatform(stage, 100.0f, 0.0f));
			Assert::IsFalse(StageCalcIsOut(stage, 50.0f));
		}
	};
}
