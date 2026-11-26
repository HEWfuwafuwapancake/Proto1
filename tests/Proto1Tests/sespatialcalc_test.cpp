#include <gtest/gtest.h>
#include "audio/sespatialcalc.h"

namespace
{
	// 原点にいて、カメラの右が +X の聞き手
	SE_LISTENER MakeListener(float x, float z, int slot)
	{
		SE_LISTENER listener;
		listener.Position = { x, 0.0f, z };
		listener.Right = { 1.0f, 0.0f, 0.0f };
		listener.Slot = slot;
		return listener;
	}
}

// 遠い音ほど小さく、近い音は表の大きさのまま
TEST(SeSpatialCalcTest, FartherSoundIsQuieter)
{
	const SE_LISTENER listener = MakeListener(0.0f, 0.0f, 0);
	const SE_MIX nearMix = SeSpatialCalcMix(&listener, 1, { 0.0f, 0.0f, 3.0f }, 0, -1);
	const SE_MIX farMix = SeSpatialCalcMix(&listener, 1, { 0.0f, 0.0f, 30.0f }, 0, -1);
	EXPECT_NEAR(nearMix.Volume, 1.0f, 0.001f);
	EXPECT_LT(farMix.Volume, nearMix.Volume);
}

// カメラの右にある音は右から、左にある音は左から鳴り、カメラを半回転すると左右が入れ替わる
TEST(SeSpatialCalcTest, PanFollowsCameraRight)
{
	SE_LISTENER listener = MakeListener(0.0f, 0.0f, 0);
	EXPECT_GT(SeSpatialCalcMix(&listener, 1, { 10.0f, 0.0f, 0.0f }, 0, -1).Pan, 0.3f);
	EXPECT_LT(SeSpatialCalcMix(&listener, 1, { -10.0f, 0.0f, 0.0f }, 0, -1).Pan, -0.3f);
	EXPECT_NEAR(SeSpatialCalcMix(&listener, 1, { 0.0f, 0.0f, 10.0f }, 0, -1).Pan, 0.0f, 0.001f); // 正面は真ん中
	listener.Right = { -1.0f, 0.0f, 0.0f };
	EXPECT_LT(SeSpatialCalcMix(&listener, 1, { 10.0f, 0.0f, 0.0f }, 0, -1).Pan, -0.3f);
}

// 同じ距離なら、この PC で遊ぶ人が当てた・当てられた音のほうが、ほかの人どうしの音より大きい
TEST(SeSpatialCalcTest, OwnHitIsLouderThanThirdParty)
{
	const SE_LISTENER listener = MakeListener(0.0f, 0.0f, 0);
	const VEC3 source = { 0.0f, 0.0f, 8.0f };
	const float attacker = SeSpatialCalcMix(&listener, 1, source, 0, 2).Volume;
	const float victim = SeSpatialCalcMix(&listener, 1, source, 2, 0).Volume;
	const float others = SeSpatialCalcMix(&listener, 1, source, 2, 3).Volume;
	EXPECT_NEAR(attacker, victim, 0.001f);
	EXPECT_LT(others, attacker);
}

// 同じ PC の分割画面では、音にいちばん近い人を基準にし、同じ距離なら前の人（枠の小さい人）を選ぶ
TEST(SeSpatialCalcTest, PicksNearestListenerAndFirstOnTie)
{
	const SE_LISTENER listeners[2] = { MakeListener(0.0f, 0.0f, 0), MakeListener(20.0f, 0.0f, 1) };
	EXPECT_EQ(SeSpatialCalcMix(listeners, 2, { 18.0f, 0.0f, 0.0f }, 2, 3).Listener, 1);
	EXPECT_EQ(SeSpatialCalcMix(listeners, 2, { 10.0f, 0.0f, 0.0f }, 2, 3).Listener, 0);
	// 分割画面ではどちらの人も当事者になる
	EXPECT_NEAR(SeSpatialCalcMix(listeners, 2, { 0.0f, 0.0f, 4.0f }, 1, 3).Volume, 1.0f, 0.001f);
}

// 左右の配り方は、真ん中でも端でも聞こえる大きさ（2乗の和）が同じ
TEST(SeSpatialCalcTest, PanGainsKeepPower)
{
	float left = 0.0f, right = 0.0f;
	SeSpatialCalcPanGains(0.0f, left, right);
	EXPECT_NEAR(left, right, 0.001f);
	EXPECT_NEAR(left * left + right * right, 1.0f, 0.001f);
	SeSpatialCalcPanGains(0.7f, left, right);
	EXPECT_GT(right, left);
	EXPECT_NEAR(left * left + right * right, 1.0f, 0.001f);
}
