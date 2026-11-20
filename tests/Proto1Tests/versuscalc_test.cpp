// physics/versuscalc.h（機器の割り当て・最後の攻撃者とKO・体当たり）の単体テスト
#include <gtest/gtest.h>
#include "physics/versuscalc.h"
#include "physics/dummycalc.h"

namespace Proto1Tests
{
	static constexpr float VERSUS_EPSILON = 1.0e-4f;
	static constexpr float BODY_RADIUS = 0.5f;
	static constexpr float BODY_HEIGHT = 1.8f;

	// キャラ選択：決定で参加 → 準備OK。全員が準備OKのときだけ 1P の決定で開始し、1P 以外の決定では始めない。
	TEST(VersusCalcTest, Lobby_JoinReadyAndOnlyFirstPlayerStartsWhenAllReady)
	{
		VersusCalc versus;
		versus.Reset();
		EXPECT_EQ(VERSUS_LOBBY_DECIDE_JOIN, versus.LobbyDecide(4, true));
		EXPECT_EQ(VERSUS_LOBBY_DECIDE_JOIN, versus.LobbyDecide(2, true));
		EXPECT_EQ(4, versus.GetDevice(0));
		EXPECT_EQ(2, versus.GetDevice(1));
		EXPECT_EQ(VERSUS_LOBBY_DECIDE_READY, versus.LobbyDecide(4, true));
		EXPECT_FALSE(versus.IsAllReady());
		EXPECT_EQ(VERSUS_LOBBY_DECIDE_NONE, versus.LobbyDecide(4, true)); // 2P がまだ
		EXPECT_EQ(VERSUS_LOBBY_DECIDE_READY, versus.LobbyDecide(2, true));
		EXPECT_TRUE(versus.IsAllReady());
		EXPECT_EQ(VERSUS_LOBBY_DECIDE_NONE, versus.LobbyDecide(2, true));
		EXPECT_EQ(VERSUS_LOBBY_DECIDE_START, versus.LobbyDecide(4, true));
	}

	// SOLO では、決定した機器のほかは参加できない。
	TEST(VersusCalcTest, Lobby_SoloRejectsOtherDevices)
	{
		VersusCalc versus;
		versus.Reset();
		versus.AssignDevice(0);
		EXPECT_EQ(VERSUS_LOBBY_DECIDE_NONE, versus.LobbyDecide(4, false));
		EXPECT_EQ(1, versus.GetHumanCount());
		EXPECT_EQ(VERSUS_LOBBY_DECIDE_READY, versus.LobbyDecide(0, false));
		EXPECT_EQ(VERSUS_LOBBY_DECIDE_START, versus.LobbyDecide(0, false));
	}

	// 戻る：準備OKの取り消し → 2P〜4P は参加をやめる。準備OKでない 1P は前の画面へ戻る。
	TEST(VersusCalcTest, Lobby_CancelUnreadiesLeavesAndFirstPlayerGoesBack)
	{
		VersusCalc versus;
		versus.Reset();
		versus.LobbyDecide(4, true);
		versus.LobbyDecide(1, true);
		versus.LobbyDecide(1, true);
		EXPECT_TRUE(versus.IsReady(1));
		EXPECT_EQ(VERSUS_LOBBY_CANCEL_UNREADY, versus.LobbyCancel(1));
		EXPECT_FALSE(versus.IsReady(1));
		EXPECT_EQ(VERSUS_LOBBY_CANCEL_LEAVE, versus.LobbyCancel(1));
		EXPECT_FALSE(versus.IsHuman(1));
		EXPECT_EQ(VERSUS_LOBBY_CANCEL_NONE, versus.LobbyCancel(1));
		EXPECT_EQ(VERSUS_LOBBY_CANCEL_BACK, versus.LobbyCancel(4));
		EXPECT_TRUE(versus.IsHuman(0));
	}

	// 試合のあとにキャラ選択へ戻ると、参加者はそのままで準備OKだけ消える。
	TEST(VersusCalcTest, Lobby_ClearReadyKeepsParticipants)
	{
		VersusCalc versus;
		versus.Reset();
		versus.LobbyDecide(3, true);
		versus.LobbyDecide(3, true);
		versus.ClearReady();
		EXPECT_EQ(3, versus.GetDevice(0));
		EXPECT_FALSE(versus.IsReady(0));
		EXPECT_FALSE(versus.IsAllReady());
	}

	// 1P の選択位置は、左右の弾きで横に並んだカードを 1P → 人のいない枠 と動き（人のいる枠は飛ばす）、上でルールの行、下で前のカードへ戻る。
	// 人のいない枠はダミーと CPU の強さを両方向に切り替えられる。
	TEST(VersusCalcTest, Lobby_FocusSkipsHumansAndTogglesCpu)
	{
		VersusCalc versus;
		versus.Reset();
		versus.AssignDevice(4);
		versus.AssignDevice(0);
		EXPECT_EQ(0, versus.GetLobbyFocus());
		versus.MoveLobbyFocus(0.0f, -1.0f, true); // カードの行で下へ倒しても横へは動かない
		EXPECT_EQ(0, versus.GetLobbyFocus());
		versus.MoveLobbyFocus(0.0f, 0.0f, true);
		versus.MoveLobbyFocus(1.0f, 0.0f, true);
		EXPECT_EQ(2, versus.GetLobbyFocus()); // 2P は人なので飛ばす
		versus.MoveLobbyFocus(-1.0f, 0.0f, true); // 戻さずに倒しても動かない
		EXPECT_EQ(2, versus.GetLobbyFocus());
		versus.MoveLobbyFocus(0.0f, 0.0f, true);
		versus.MoveLobbyFocus(0.0f, 1.0f, true);
		EXPECT_EQ(VERSUS_LOBBY_FOCUS_RULE, versus.GetLobbyFocus());
		versus.MoveLobbyFocus(0.0f, 0.0f, true);
		versus.MoveLobbyFocus(0.0f, -1.0f, true);
		EXPECT_EQ(2, versus.GetLobbyFocus()); // 下で前に選んでいたカードへ戻る
		EXPECT_EQ(VERSUS_CPU_NORMAL, versus.GetCpuLevel(2)); // はじめは CPU ふつう
		versus.CycleCpu(2, 1);
		EXPECT_EQ(VERSUS_CPU_HARD, versus.GetCpuLevel(2));
		versus.CycleCpu(2, 1);
		EXPECT_FALSE(versus.IsCpu(2)); // つよいの次はダミー
		versus.CycleCpu(2, -1);
		EXPECT_EQ(VERSUS_CPU_HARD, versus.GetCpuLevel(2)); // 逆向きではダミーの前がつよい
		versus.CycleCpu(1, 1);
		EXPECT_EQ(VERSUS_CPU_NONE, versus.GetCpuLevel(1)); // 人のいる枠は変えない
		versus.CycleCpu(2, 1);
		versus.AssignDevice(2); // 選んでいた枠に人が入ると 1P のカードへ戻る
		versus.MoveLobbyFocus(0.0f, 0.0f, true);
		EXPECT_EQ(0, versus.GetLobbyFocus());
		EXPECT_FALSE(versus.IsCpu(2));
	}

	// RETRYでは機器の割り当てを保ってKOと攻撃記録を消し、TITLEでは参加を消す。
	TEST(VersusCalcTest, Retry_KeepsParticipantsTitleClearsThem)
	{
		VersusCalc versus;
		versus.Reset();
		versus.AssignDevice(3);
		versus.AssignDevice(4);
		versus.RecordAttack(1, 2);
		versus.RecordOut(2);
		versus.RecordAttack(0, 1);
		versus.ResetMatch();
		EXPECT_EQ(2, versus.GetHumanCount());
		EXPECT_EQ(3, versus.GetDevice(0));
		EXPECT_EQ(4, versus.GetDevice(1));
		EXPECT_EQ(0, versus.GetScore(1));
		EXPECT_EQ(-1, versus.GetLastAttacker(1));
		versus.RecordOut(1);
		EXPECT_EQ(0, versus.GetScore(0));
		versus.Reset();
		EXPECT_EQ(0, versus.GetHumanCount());
		EXPECT_EQ(0, versus.AssignDevice(0));
		EXPECT_EQ(0, versus.GetDevice(0));
	}

	// 人だけをKO順で並べ、同点は同順位（1・1・3）。同点の表示順はスロット順。
	TEST(VersusCalcTest, Result_RanksHumansByKoWithTies)
	{
		VersusCalc versus;
		versus.Reset();
		versus.AssignDevice(0);
		versus.AssignDevice(1);
		versus.AssignDevice(4);
		for (int attacker = 1; attacker <= 2; ++attacker) {
			for (int ko = 0; ko < 2; ++ko) {
				versus.RecordAttack(attacker, 3);
				versus.RecordOut(3);
			}
		}
		EXPECT_EQ(1, versus.GetRank(1));
		EXPECT_EQ(1, versus.GetRank(2));
		EXPECT_EQ(3, versus.GetRank(0));
		EXPECT_EQ(0, versus.GetRank(3)); // ダミーは順位に入れない
		EXPECT_EQ(1, versus.GetRankedSlot(0));
		EXPECT_EQ(2, versus.GetRankedSlot(1));
		EXPECT_EQ(0, versus.GetRankedSlot(2));
	}

	// 押した順に1P〜4P。同じ機器は二重参加せず、残りはダミー。
	TEST(VersusCalcTest, Slots_AssignDevicesInOrder)
	{
		VersusCalc versus;
		versus.Reset();
		EXPECT_EQ(0, versus.AssignDevice(4));
		EXPECT_EQ(1, versus.AssignDevice(2));
		EXPECT_EQ(4, versus.GetDevice(0));
		EXPECT_EQ(2, versus.GetDevice(1));
		EXPECT_FALSE(versus.IsHuman(2));
		EXPECT_EQ(1, versus.AssignDevice(2));
		EXPECT_EQ(2, versus.AssignDevice(0));
		EXPECT_EQ(3, versus.AssignDevice(3));
		EXPECT_EQ(-1, versus.AssignDevice(1));
	}

	// 段階Bの1人用合成入力でも、候補は自分以外の3体。
	TEST(VersusCalcTest, Slots_SinglePlayerHasThreeOpponents)
	{
		VersusCalc versus;
		versus.Reset();
		EXPECT_EQ(0, versus.AssignDevice(VERSUS_DEVICE_COMBINED));
		EXPECT_TRUE(versus.IsHuman(0));
		EXPECT_EQ(VERSUS_DEVICE_COMBINED, versus.GetDevice(0));
		for (int self = 0; self < FIGHTER_COUNT; ++self) {
			int count = 0;
			for (int target = 0; target < FIGHTER_COUNT; ++target) {
				if (versus.IsOpponent(self, target)) ++count;
			}
			EXPECT_FALSE(versus.IsOpponent(self, self));
			EXPECT_EQ(3, count);
		}
	}

	// 人でもダミーでも、落とした人に+1。落ちた人の得点は減らない。
	TEST(VersusCalcTest, Ko_HumanScoresForHumanAndDummy)
	{
		VersusCalc versus;
		versus.Reset();
		versus.AssignDevice(0);
		versus.AssignDevice(4);
		versus.RecordAttack(0, 1);
		versus.RecordOut(1);
		versus.RecordAttack(1, 2);
		versus.RecordOut(2);
		versus.RecordAttack(0, 3);
		versus.RecordOut(3);
		EXPECT_EQ(2, versus.GetScore(0));
		EXPECT_EQ(1, versus.GetScore(1));
	}

	TEST(VersusCalcTest, Ko_LastAttackerWins)
	{
		VersusCalc versus;
		versus.Reset();
		versus.AssignDevice(0);
		versus.AssignDevice(1);
		versus.RecordAttack(0, 2);
		versus.RecordAttack(1, 2);
		EXPECT_EQ(1, versus.GetLastAttacker(2));
		versus.RecordOut(2);
		EXPECT_EQ(0, versus.GetScore(0));
		EXPECT_EQ(1, versus.GetScore(1));
	}

	// 自滅、ダミーの攻撃、前の生存期間の攻撃で点を取らない。
	TEST(VersusCalcTest, Ko_SuicideAndDummyDoNotScore)
	{
		VersusCalc versus;
		versus.Reset();
		versus.AssignDevice(0);
		versus.RecordOut(1);
		versus.RecordAttack(0, 2);
		versus.RecordAttack(3, 2);
		versus.RecordOut(2);
		EXPECT_EQ(0, versus.GetScore(0));
		EXPECT_EQ(0, versus.GetScore(3));
		versus.RecordAttack(0, 1);
		versus.RecordOut(1);
		EXPECT_EQ(-1, versus.GetLastAttacker(1));
		versus.RecordOut(1);
		EXPECT_EQ(1, versus.GetScore(0));
	}

	TEST(VersusCalcTest, Restart_ClearsSlotsScoresAndAttacks)
	{
		VersusCalc versus;
		versus.Reset();
		versus.AssignDevice(0);
		versus.RecordAttack(0, 1);
		versus.RecordOut(1);
		versus.RecordAttack(0, 2);
		versus.Reset();
		EXPECT_EQ(0, versus.GetScore(0));
		EXPECT_EQ(-1, versus.GetLastAttacker(2));
		EXPECT_FALSE(versus.IsHuman(0));
		EXPECT_EQ(0, versus.AssignDevice(4));
	}

	// 人対人は横の押し出しを半分ずつ受け、両方の体が離れる。
	TEST(VersusCalcTest, Push_HumansShareSeparation)
	{
		VEC3 first{ 0.0f, 0.0f, 0.0f }, second{ 0.0f, 0.0f, 0.7f };
		EXPECT_TRUE(VersusCalcPushBodies(first, second, true, true, BODY_RADIUS, BODY_HEIGHT));
		EXPECT_NEAR(first.Z, 0.7f - second.Z, VERSUS_EPSILON);
		EXPECT_TRUE(first.Z < 0.0f);
		EXPECT_FALSE(CollisionCapsuleCapsule(DummyCalcBodyCapsule(first, BODY_RADIUS, BODY_HEIGHT),
			DummyCalcBodyCapsule(second, BODY_RADIUS, BODY_HEIGHT)).Hit);
	}

	// スロット順によらず、人がダミーを押す従来の動きを保つ。
	TEST(VersusCalcTest, Push_HumanMovesDummy)
	{
		VEC3 dummy{ 0.0f, 0.0f, 0.7f }, human{};
		EXPECT_TRUE(VersusCalcPushBodies(dummy, human, false, true, BODY_RADIUS, BODY_HEIGHT));
		EXPECT_NEAR(0.0f, human.Z, VERSUS_EPSILON);
		EXPECT_TRUE(dummy.Z > 1.0f);
	}

	// 人どうしで頭に乗ったときは上の体を押し上げ、下の体を地面へ埋めない。
	TEST(VersusCalcTest, Push_HumanOnHeadMovesUpperBody)
	{
		VEC3 lower{}, upper{ 0.0f, 1.5f, 0.0f };
		EXPECT_TRUE(VersusCalcPushBodies(lower, upper, true, true, BODY_RADIUS, BODY_HEIGHT));
		EXPECT_NEAR(0.0f, lower.Y, VERSUS_EPSILON);
		EXPECT_TRUE(upper.Y >= BODY_HEIGHT);
	}

	// 1人は全画面、2人は右半分が2人目。
	TEST(VersusCalcTest, Viewport_OneFullTwoSideBySide)
	{
		const VERSUS_VIEWPORT one = VersusCalcViewport(1, 0, 1280.0f, 720.0f);
		EXPECT_EQ(0.0f, one.X);
		EXPECT_EQ(1280.0f, one.Width);
		EXPECT_EQ(720.0f, one.Height);
		const VERSUS_VIEWPORT second = VersusCalcViewport(2, 1, 1280.0f, 720.0f);
		EXPECT_EQ(640.0f, second.X);
		EXPECT_EQ(0.0f, second.Y);
		EXPECT_EQ(640.0f, second.Width);
		EXPECT_EQ(720.0f, second.Height);
	}

	// 3〜4人は4分割で、左上・右上・左下・右下の順。
	TEST(VersusCalcTest, Viewport_FourQuarters)
	{
		const VERSUS_VIEWPORT third = VersusCalcViewport(4, 2, 1280.0f, 720.0f);
		EXPECT_EQ(0.0f, third.X);
		EXPECT_EQ(360.0f, third.Y);
		const VERSUS_VIEWPORT fourth = VersusCalcViewport(3, 3, 1280.0f, 720.0f);
		EXPECT_EQ(640.0f, fourth.X);
		EXPECT_EQ(360.0f, fourth.Y);
		EXPECT_EQ(640.0f, fourth.Width);
		EXPECT_EQ(360.0f, fourth.Height);
	}
}
