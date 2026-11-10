// あとから効く当たり（DirectX なし）の単体テスト
#include <gtest/gtest.h>
#include "physics/delayedhitcalc.h"

namespace Proto1Tests
{
	// 居合の遅れ：0.4 秒たつまでは効かず、60fps の刻みで 0.4 秒後に1回だけ取り出せる。
	TEST(DelayedHitCalcTest, Step_ReleasesHitOnceAfterDelayAt60Fps)
	{
		DelayedHitCalc calc;
		DELAYED_HIT hit;
		hit.Target = 2;
		hit.Damage = 12.0f;
		hit.Seconds = 0.4f;
		EXPECT_TRUE(calc.Add(hit));

		DELAYED_HIT out[DELAYED_HIT_MAX];
		for (int frame = 0; frame < 23; ++frame) EXPECT_EQ(calc.Step(1.0f / 60.0f, out, DELAYED_HIT_MAX), 0);
		int released = 0;
		for (int frame = 0; frame < 3; ++frame) released += calc.Step(1.0f / 60.0f, out, DELAYED_HIT_MAX);
		EXPECT_EQ(released, 1);
		EXPECT_EQ(out[0].Target, 2);
		EXPECT_NEAR(out[0].Damage, 12.0f, 0.001f);
		EXPECT_EQ(calc.GetCount(), 0);
	}

	// 複数の相手：早く効くものだけを取り出し、残りは待ち続ける。
	TEST(DelayedHitCalcTest, Step_KeepsHitsThatAreNotReady)
	{
		DelayedHitCalc calc;
		DELAYED_HIT first;
		first.Target = 0;
		first.Seconds = 0.1f;
		DELAYED_HIT second;
		second.Target = 1;
		second.Seconds = 0.5f;
		calc.Add(first);
		calc.Add(second);

		DELAYED_HIT out[DELAYED_HIT_MAX];
		EXPECT_EQ(calc.Step(0.2f, out, DELAYED_HIT_MAX), 1);
		EXPECT_EQ(out[0].Target, 0);
		ASSERT_EQ(calc.GetCount(), 1);
		EXPECT_EQ(calc.Get(0).Target, 1);
		EXPECT_NEAR(calc.Get(0).Seconds, 0.3f, 0.001f);
	}
}
