// タイトル・試合・結果の遷移とメニューの主な動き（DirectX なし）
#include <gtest/gtest.h>
#include "physics/scenecalc.h"

namespace Proto1Tests
{
	static void SelectMode(SceneCalc& scene, MODE_ITEM item)
	{
		for (int i = 0; i < item; ++i) {
			scene.Step({ false, false, 0.0f }, false);
			scene.Step({ false, false, -1.0f }, false);
		}
	}

	TEST(SceneCalcTest, Title_DecideStartsMatch)
	{
		SceneCalc scene;
		scene.Start(true);
		EXPECT_EQ((int)SCENE_TITLE, (int)scene.GetScene());
		EXPECT_EQ(GAME_RULE_DEBUG, scene.GetRule());
		EXPECT_EQ((int)SCENE_EVENT_NONE, (int)scene.Step({ false, false, 0 }, false));
		EXPECT_EQ((int)SCENE_EVENT_START_MATCH, (int)scene.Step({ true, false, 0 }, false));
		EXPECT_EQ((int)SCENE_MATCH, (int)scene.GetScene());
	}

	TEST(SceneCalcTest, Match_DecideOnlyShowsResultAfterTimeUp)
	{
		SceneCalc scene;
		scene.Start(true);
		scene.Step({ true, false, 0 }, false);
		EXPECT_EQ((int)SCENE_EVENT_NONE, (int)scene.Step({ true, false, 0 }, false));
		EXPECT_EQ((int)SCENE_MATCH, (int)scene.GetScene());
		EXPECT_EQ((int)SCENE_EVENT_NONE, (int)scene.Step({ false, false, 0 }, true));
		EXPECT_EQ((int)SCENE_EVENT_SHOW_RESULT, (int)scene.Step({ true, false, 0 }, true));
		EXPECT_EQ((int)SCENE_RESULT, (int)scene.GetScene());
		EXPECT_EQ((int)RESULT_ITEM_RETRY, scene.GetCursor());
	}

	TEST(SceneCalcTest, Restart_StartsMatchDuringPlayAfterTimeUpAndFromResult)
	{
		SceneCalc scene;
		scene.Start(true);
		scene.Step({ true, false, 0 }, false);
		EXPECT_EQ((int)SCENE_EVENT_START_MATCH, (int)scene.Step({ false, true, 0 }, false));
		EXPECT_EQ((int)SCENE_EVENT_START_MATCH, (int)scene.Step({ true, true, 0 }, true));
		EXPECT_EQ((int)SCENE_MATCH, (int)scene.GetScene());
		scene.Step({ true, false, 0 }, true);
		scene.Step({ false, false, -1 }, true); // キャラ選択へを選んでいてもやり直せる
		EXPECT_EQ((int)SCENE_EVENT_START_MATCH, (int)scene.Step({ false, true, 0 }, true));
		EXPECT_EQ((int)SCENE_MATCH, (int)scene.GetScene());
	}

	// 結果の2つ目はキャラ選択へ戻り、3つ目はタイトル（オンラインはロビー）へ戻る。
	TEST(SceneCalcTest, Result_DecideRetriesBacksToCharacterSelectOrTitle)
	{
		SceneCalc scene;
		scene.Start(true);
		scene.Step({ true, false, 0 }, false);
		scene.Step({ true, false, 0 }, true);
		EXPECT_EQ((int)SCENE_EVENT_START_MATCH, (int)scene.Step({ true, false, 0 }, true));
		EXPECT_EQ((int)SCENE_MATCH, (int)scene.GetScene());
		scene.Step({ true, false, 0 }, true);
		scene.Step({ false, false, -1 }, true);
		EXPECT_EQ((int)RESULT_ITEM_CHARACTER, scene.GetCursor());
		EXPECT_EQ((int)SCENE_EVENT_BACK_LOBBY, (int)scene.Step({ true, false, 0 }, true));
		EXPECT_EQ((int)SCENE_TITLE, (int)scene.GetScene());
		scene.Step({ true, false, 0 }, false);
		scene.Step({ true, false, 0 }, true);
		for (int i = 0; i < 2; ++i) {
			scene.Step({ false, false, 0 }, true);
			scene.Step({ false, false, -1 }, true);
		}
		EXPECT_EQ((int)RESULT_ITEM_TITLE, scene.GetCursor());
		EXPECT_EQ((int)SCENE_EVENT_BACK_TITLE, (int)scene.Step({ true, false, 0 }, true));
		EXPECT_EQ((int)SCENE_TITLE, (int)scene.GetScene());
	}

	TEST(SceneCalcTest, Menu_HeldStickWaitsForNeutralAndMovesOncePerFlick)
	{
		SceneCalc scene;
		scene.Start(true);
		scene.Step({ true, false, 0 }, false);
		scene.Step({ true, false, -1 }, true); // 入る前から倒している
		scene.Step({ false, false, -1 }, true);
		EXPECT_EQ((int)RESULT_ITEM_RETRY, scene.GetCursor());
		scene.Step({ false, false, 0 }, true);
		scene.Step({ false, false, -0.7f }, true);
		EXPECT_EQ((int)RESULT_ITEM_CHARACTER, scene.GetCursor());
		for (int i = 0; i < 3; ++i) scene.Step({ false, false, -1 }, true);
		scene.Step({ false, false, 1 }, true); // 戻さず逆に倒しても次の弾きにしない
		EXPECT_EQ((int)RESULT_ITEM_CHARACTER, scene.GetCursor());
		scene.Step({ false, false, 0.2f }, true);
		scene.Step({ false, false, 0.7f }, true);
		EXPECT_EQ((int)RESULT_ITEM_RETRY, scene.GetCursor());
	}

	// GAME START → 対戦方法（SOLO）→ ルール → キャラ選択 → 試合。結果からタイトルへ戻ると GAME START。
	TEST(SceneCalcTest, GameStart_ModeThenRuleThenCharacterSelectAndResultReturnsToGameStart)
	{
		SceneCalc scene;
		scene.Start();
		EXPECT_EQ((int)SCENE_GAME_START, (int)scene.GetScene());
		scene.Step({ true, false, 0 }, false);
		EXPECT_EQ(SCENE_MODE_SELECT, scene.GetScene());
		scene.Step({ true, false, 0 }, false);
		EXPECT_EQ(SCENE_RULE_SELECT, scene.GetScene());
		scene.Step({ true, false, 0 }, false);
		EXPECT_EQ(GAME_RULE_GAME, scene.GetRule());
		EXPECT_EQ(SCENE_TITLE, scene.GetScene());
		EXPECT_TRUE(scene.IsSolo());
		EXPECT_EQ(SCENE_EVENT_START_MATCH, scene.Step({ true, false, 0 }, false));
		scene.Step({ true, false, 0 }, true);
		for (int i = 0; i < 2; ++i) {
			scene.Step({ false, false, 0 }, true);
			scene.Step({ false, false, -1 }, true);
		}
		EXPECT_EQ((int)SCENE_EVENT_BACK_TITLE, (int)scene.Step({ true, false, 0 }, true));
		EXPECT_EQ(SCENE_GAME_START, scene.GetScene());
	}

	// 倒しっぱなしでは選択を動かさず、戻ると対戦方法を表示する。決定したルールは RETRY でも使う。
	TEST(SceneCalcTest, RuleSelect_WaitsForNeutralGoesBackAndKeepsChoiceOnRetry)
	{
		SceneCalc scene;
		scene.ShowModes();
		scene.Step({ true, false, -1 }, false);
		ASSERT_EQ(SCENE_RULE_SELECT, scene.GetScene());
		scene.Step({ false, false, -1 }, false);
		EXPECT_EQ(GAME_RULE_GAME, scene.GetCursor());
		scene.Step({ false, false, 0, true }, false);
		ASSERT_EQ(SCENE_MODE_SELECT, scene.GetScene());
		EXPECT_EQ(MODE_ITEM_SOLO, scene.GetCursor());
		scene.Step({ true, false, 0 }, false);
		scene.Step({ false, false, -1 }, false);
		EXPECT_EQ(GAME_RULE_DEBUG, scene.GetCursor());
		scene.Step({ true, false, -1 }, false);
		ASSERT_EQ(SCENE_TITLE, scene.GetScene());
		EXPECT_EQ(GAME_RULE_DEBUG, scene.GetRule());
		ASSERT_EQ(SCENE_EVENT_START_MATCH, scene.Step({ true, false, 0 }, false));
		ASSERT_EQ(SCENE_EVENT_SHOW_RESULT, scene.Step({ true, false, 0 }, true));
		EXPECT_EQ(SCENE_EVENT_START_MATCH, scene.Step({ true, false, 0 }, true));
		EXPECT_EQ(GAME_RULE_DEBUG, scene.GetRule());
	}

	// LOCAL VS はルールのあとキャラ選択へ。キャラ選択から戻るとルール、さらに戻ると対戦方法と GAME START。
	TEST(SceneCalcTest, Modes_LocalLobbyBackToRuleThenModesAndGameStart)
	{
		SceneCalc scene;
		scene.ShowModes();
		SelectMode(scene, MODE_ITEM_LOCAL);
		scene.Step({ true, false, 0 }, false);
		scene.Step({ true, false, 0 }, false);
		EXPECT_EQ((int)SCENE_TITLE, (int)scene.GetScene());
		EXPECT_FALSE(scene.IsSolo());
		EXPECT_EQ((int)SCENE_EVENT_BACK_MENU, (int)scene.Step({ false, false, 0, true }, false));
		EXPECT_EQ((int)SCENE_RULE_SELECT, (int)scene.GetScene());
		scene.Step({ false, false, 0, true }, false);
		EXPECT_EQ((int)SCENE_MODE_SELECT, (int)scene.GetScene());
		EXPECT_EQ(MODE_ITEM_LOCAL, scene.GetCursor());
		SelectMode(scene, static_cast<MODE_ITEM>(MODE_ITEM_BACK - MODE_ITEM_LOCAL));
		scene.Step({ true, false, 0 }, false);
		EXPECT_EQ((int)SCENE_GAME_START, (int)scene.GetScene());
	}

	// オプションから元のメニューへ戻り、値変更と標準への復帰をゲーム側に伝える。
	TEST(SceneCalcTest, Options_HomeChangesResetsAndReturnsWithoutStartingMatch)
	{
		SceneCalc scene;
		scene.Start();
		scene.Step({ false, false, -1 }, false);
		scene.Step({ true, false, 0 }, false);
		ASSERT_EQ(SCENE_OPTIONS, scene.GetScene());
		EXPECT_EQ(SCENE_EVENT_TUNING_CHANGE, scene.Step({ false, false, 0, false, false, 1 }, false));
		for (int i = 0; i < OPTION_ITEM_RESET; ++i) {
			scene.Step({ false, false, 0 }, false);
			scene.Step({ false, false, -1 }, false);
		}
		EXPECT_EQ(SCENE_EVENT_TUNING_RESET, scene.Step({ true, false, 0 }, false));
		scene.Step({ false, false, -1 }, false);
		EXPECT_EQ(SCENE_EVENT_NONE, scene.Step({ true, false, 0 }, false));
		EXPECT_EQ(SCENE_GAME_START, scene.GetScene());
		EXPECT_EQ(1, scene.GetCursor());
	}

	// オプションから耐久の減る量の表を開き、値の変更を伝えて、戻るとオプションの同じ行へ戻る。
	TEST(SceneCalcTest, Options_ArmCostsChangeAndReturnToOptions)
	{
		SceneCalc scene;
		scene.Start();
		scene.Step({ false, false, -1 }, false);
		scene.Step({ true, false, 0 }, false);
		for (int i = 0; i < OPTION_ITEM_ARM_COSTS; ++i) {
			scene.Step({ false, false, 0 }, false);
			scene.Step({ false, false, -1 }, false);
		}
		scene.Step({ true, false, 0 }, false);
		ASSERT_EQ(SCENE_ARM_COSTS, scene.GetScene());
		EXPECT_EQ(SCENE_EVENT_ARM_COST_CHANGE, scene.Step({ false, false, 0, false, false, -1 }, false));
		EXPECT_EQ(SCENE_EVENT_NONE, scene.Step({ false, false, 0, true }, false));
		EXPECT_EQ(SCENE_OPTIONS, scene.GetScene());
		EXPECT_EQ(OPTION_ITEM_ARM_COSTS, scene.GetCursor());
	}

	// モード選択からの設定変更でも、ルールと戻り先のカーソルを保つ。
	TEST(SceneCalcTest, Options_ModesKeepRuleAndReturnToOptionsRow)
	{
		SceneCalc scene;
		scene.Start();
		scene.Step({ true, false, 0 }, false);
		scene.Step({ true, false, 0 }, false);
		scene.Step({ false, false, -1 }, false);
		scene.Step({ true, false, 0 }, false);
		ASSERT_EQ(GAME_RULE_DEBUG, scene.GetRule());
		scene.Step({ false, false, 0, true }, false);
		scene.Step({ false, false, 0, true }, false);
		ASSERT_EQ(SCENE_MODE_SELECT, scene.GetScene());
		SelectMode(scene, MODE_ITEM_OPTIONS);
		scene.Step({ true, false, 0 }, false);
		ASSERT_EQ(SCENE_OPTIONS, scene.GetScene());
		scene.Step({ false, false, 0, true }, false);
		EXPECT_EQ(SCENE_MODE_SELECT, scene.GetScene());
		EXPECT_EQ(MODE_ITEM_OPTIONS, scene.GetCursor());
		EXPECT_EQ(GAME_RULE_DEBUG, scene.GetRule());
	}

	TEST(SceneCalcTest, Modes_HostAndValidatedJoinOpenLobby)
	{
		SceneCalc scene;
		scene.ShowModes();
		SelectMode(scene, MODE_ITEM_HOST);
		scene.Step({ true, false, 0 }, false);
		EXPECT_EQ((int)SCENE_RULE_SELECT, (int)scene.GetScene());
		EXPECT_EQ((int)SCENE_EVENT_HOST, (int)scene.Step({ true, false, 0 }, false));
		scene.OpenLobby();
		EXPECT_EQ((int)SCENE_TITLE, (int)scene.GetScene());
		scene.Step({ false, false, 0, true }, false);
		SelectMode(scene, MODE_ITEM_JOIN);
		scene.Step({ true, false, 0 }, false);
		EXPECT_EQ((int)SCENE_JOIN_ADDRESS, (int)scene.GetScene());
		EXPECT_EQ((int)SCENE_EVENT_NONE, (int)scene.Step({ true, false, 0, false, false }, false));
		EXPECT_EQ((int)SCENE_EVENT_JOIN, (int)scene.Step({ true, false, 0, false, true }, false));
		scene.Step({ false, false, 0, true }, false);
		EXPECT_EQ((int)SCENE_MODE_SELECT, (int)scene.GetScene());
		scene.OpenLobby(); // 参加成功後も切断時には直接モード選択へ戻せる。
		scene.ShowModes();
		EXPECT_EQ((int)SCENE_MODE_SELECT, (int)scene.GetScene());
	}
}
