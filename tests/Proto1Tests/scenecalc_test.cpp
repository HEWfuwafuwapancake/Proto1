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

	// キャラ選択の決定でマップ選択へ進み、マップ選択の決定で試合を始める。
	static void StartMatch(SceneCalc& scene)
	{
		scene.Step({ true, false, 0 }, false);
		scene.Step({ true, false, 0 }, false);
	}

	// メニューを上へ戻る移り方だけを「戻る」とし、先へ進む移り方は戻るにしない（戻るの音を結果で選ぶ。No.195）
	TEST(SceneCalcTest, BackTransition_OnlyWhenReturningUpTheMenu)
	{
		EXPECT_TRUE(SceneCalc::IsBackTransition(SCENE_OPTIONS, SCENE_MODE_SELECT));
		EXPECT_TRUE(SceneCalc::IsBackTransition(SCENE_MODE_SELECT, SCENE_GAME_START));
		EXPECT_TRUE(SceneCalc::IsBackTransition(SCENE_MAP_SELECT, SCENE_TITLE));
		EXPECT_FALSE(SceneCalc::IsBackTransition(SCENE_MODE_SELECT, SCENE_OPTIONS));
		EXPECT_FALSE(SceneCalc::IsBackTransition(SCENE_TITLE, SCENE_MAP_SELECT));
	}

	TEST(SceneCalcTest, Title_DecideOpensMapSelectThenStartsMatch)
	{
		SceneCalc scene;
		scene.Start(true);
		EXPECT_EQ((int)SCENE_TITLE, (int)scene.GetScene());
		EXPECT_EQ(GAME_RULE_DEBUG, scene.GetRule());
		EXPECT_EQ((int)SCENE_EVENT_NONE, (int)scene.Step({ false, false, 0 }, false));
		EXPECT_EQ((int)SCENE_EVENT_OPEN_MAP_SELECT, (int)scene.Step({ true, false, 0 }, false));
		EXPECT_EQ((int)SCENE_MAP_SELECT, (int)scene.GetScene());
		EXPECT_EQ((int)SCENE_EVENT_START_MATCH, (int)scene.Step({ true, false, 0 }, false));
		EXPECT_EQ((int)SCENE_MATCH, (int)scene.GetScene());
	}

	// マップ選択は左右の弾きで1つずつ動き（一周する）、選んだマップで試合をして、もう一度でも同じマップを使う。
	// 戻るとキャラ選択へ戻る。
	TEST(SceneCalcTest, MapSelect_FlickCyclesMapKeptOnRetryAndBackReturnsToLobby)
	{
		SceneCalc scene;
		scene.Start(true);
		scene.Step({ true, false, 0 }, false);
		ASSERT_EQ((int)SCENE_MAP_SELECT, (int)scene.GetScene());
		EXPECT_EQ(STAGE_MAP_POST_OFFICE, scene.GetMap());
		SCENE_INPUT right{ false, false, 0 };
		right.MoveX = 1.0f;
		SCENE_INPUT left{ false, false, 0 };
		left.MoveX = -1.0f;
		scene.Step(right, false);
		scene.Step(right, false); // 倒したままでは次へ動かない
		EXPECT_EQ(STAGE_MAP_MARKET, scene.GetMap());
		scene.Step({ false, false, 0 }, false);
		scene.Step(left, false);
		scene.Step({ false, false, 0 }, false);
		scene.Step(left, false);
		EXPECT_EQ(STAGE_MAP_ISLAND, scene.GetMap());
		EXPECT_EQ((int)SCENE_EVENT_BACK_LOBBY, (int)scene.Step({ false, false, 0, true }, false));
		EXPECT_EQ((int)SCENE_TITLE, (int)scene.GetScene());
		scene.Step({ true, false, 0 }, false);
		scene.Step({ false, false, 0 }, false);
		scene.Step(right, false);
		ASSERT_EQ(STAGE_MAP_POST_OFFICE, scene.GetMap());
		EXPECT_EQ((int)SCENE_EVENT_START_MATCH, (int)scene.Step({ true, false, 0 }, false));
		scene.Step({ true, false, 0 }, true);
		EXPECT_EQ((int)SCENE_EVENT_START_MATCH, (int)scene.Step({ true, false, 0 }, true));
		EXPECT_EQ(STAGE_MAP_POST_OFFICE, scene.GetMap());
	}

	TEST(SceneCalcTest, Match_DecideOnlyShowsResultAfterTimeUp)
	{
		SceneCalc scene;
		scene.Start(true);
		StartMatch(scene);
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
		StartMatch(scene);
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
		StartMatch(scene);
		scene.Step({ true, false, 0 }, true);
		EXPECT_EQ((int)SCENE_EVENT_START_MATCH, (int)scene.Step({ true, false, 0 }, true));
		EXPECT_EQ((int)SCENE_MATCH, (int)scene.GetScene());
		scene.Step({ true, false, 0 }, true);
		scene.Step({ false, false, -1 }, true);
		EXPECT_EQ((int)RESULT_ITEM_CHARACTER, scene.GetCursor());
		EXPECT_EQ((int)SCENE_EVENT_BACK_LOBBY, (int)scene.Step({ true, false, 0 }, true));
		EXPECT_EQ((int)SCENE_TITLE, (int)scene.GetScene());
		StartMatch(scene);
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
		StartMatch(scene);
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

	// GAME START → 対戦方法（SOLO）→ キャラ選択 → 試合。結果からタイトルへ戻ると GAME START。
	TEST(SceneCalcTest, GameStart_ModeThenCharacterSelectAndResultReturnsToGameStart)
	{
		SceneCalc scene;
		scene.Start();
		EXPECT_EQ((int)SCENE_GAME_START, (int)scene.GetScene());
		scene.Step({ true, false, 0 }, false);
		EXPECT_EQ(SCENE_MODE_SELECT, scene.GetScene());
		scene.Step({ true, false, 0 }, false);
		EXPECT_EQ(SCENE_TITLE, scene.GetScene());
		EXPECT_TRUE(scene.IsSolo());
		EXPECT_EQ(SCENE_EVENT_OPEN_MAP_SELECT, scene.Step({ true, false, 0 }, false));
		EXPECT_EQ(SCENE_EVENT_START_MATCH, scene.Step({ true, false, 0 }, false));
		scene.Step({ true, false, 0 }, true);
		for (int i = 0; i < 2; ++i) {
			scene.Step({ false, false, 0 }, true);
			scene.Step({ false, false, -1 }, true);
		}
		EXPECT_EQ((int)SCENE_EVENT_BACK_TITLE, (int)scene.Step({ true, false, 0 }, true));
		EXPECT_EQ(SCENE_GAME_START, scene.GetScene());
	}

	// キャラ選択で切り替えたルールを、試合と RETRY で使う。
	TEST(SceneCalcTest, Rule_CycledInCharacterSelectIsKeptOnRetry)
	{
		SceneCalc scene;
		scene.ShowModes();
		scene.Step({ true, false, 0 }, false);
		ASSERT_EQ(SCENE_TITLE, scene.GetScene());
		EXPECT_EQ(GAME_RULE_GAME, scene.GetRule());
		scene.CycleRule(1);
		EXPECT_EQ(GAME_RULE_DEBUG, scene.GetRule());
		ASSERT_EQ(SCENE_EVENT_OPEN_MAP_SELECT, scene.Step({ true, false, 0 }, false));
		ASSERT_EQ(SCENE_EVENT_START_MATCH, scene.Step({ true, false, 0 }, false));
		ASSERT_EQ(SCENE_EVENT_SHOW_RESULT, scene.Step({ true, false, 0 }, true));
		EXPECT_EQ(SCENE_EVENT_START_MATCH, scene.Step({ true, false, 0 }, true));
		EXPECT_EQ(GAME_RULE_DEBUG, scene.GetRule());
	}

	// LOCAL VS はすぐキャラ選択へ。キャラ選択から戻ると対戦方法の同じ行、さらに戻ると GAME START。
	TEST(SceneCalcTest, Modes_LocalLobbyBackToModesAndGameStart)
	{
		SceneCalc scene;
		scene.ShowModes();
		SelectMode(scene, MODE_ITEM_LOCAL);
		scene.Step({ true, false, 0 }, false);
		EXPECT_EQ((int)SCENE_TITLE, (int)scene.GetScene());
		EXPECT_FALSE(scene.IsSolo());
		EXPECT_EQ((int)SCENE_EVENT_BACK_MENU, (int)scene.Step({ false, false, 0, true }, false));
		EXPECT_EQ((int)SCENE_MODE_SELECT, (int)scene.GetScene());
		EXPECT_EQ(MODE_ITEM_LOCAL, scene.GetCursor());
		scene.Step({ false, false, 0, true }, false);
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

	// オプションの視点感度の行で左右を押すと、感度の変更をゲーム側に伝える。
	TEST(SceneCalcTest, Options_LookSensitivityChange)
	{
		SceneCalc scene;
		scene.Start();
		scene.Step({ false, false, -1 }, false);
		scene.Step({ true, false, 0 }, false);
		for (int i = 0; i < OPTION_ITEM_LOOK_SENSITIVITY; ++i) {
			scene.Step({ false, false, 0 }, false);
			scene.Step({ false, false, -1 }, false);
		}
		ASSERT_EQ(OPTION_ITEM_LOOK_SENSITIVITY, scene.GetCursor());
		EXPECT_EQ(SCENE_EVENT_LOOK_SENSITIVITY_CHANGE, scene.Step({ false, false, 0, false, false, 1 }, false));
		EXPECT_EQ(SCENE_EVENT_LOOK_SENSITIVITY_CHANGE, scene.Step({ false, false, 0, false, false, -1 }, false));
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

	// 表の行が見せる数より多いとき、選んでいる行が下の端・上の端を越えたぶんだけスクロールし、見えている間は動かさない。
	TEST(SceneCalcTest, ScrollTopFollowsSelectedRowOnlyWhenItLeavesView)
	{
		EXPECT_EQ(0, SceneCalcScrollTop(0, 7, 8, 10));
		EXPECT_EQ(1, SceneCalcScrollTop(0, 8, 8, 10));
		EXPECT_EQ(2, SceneCalcScrollTop(1, 9, 8, 10));
		EXPECT_EQ(2, SceneCalcScrollTop(2, 3, 8, 10));
		EXPECT_EQ(1, SceneCalcScrollTop(2, 1, 8, 10));
		EXPECT_EQ(1, SceneCalcScrollTop(1, -1, 8, 10));
		EXPECT_EQ(0, SceneCalcScrollTop(0, 7, 8, 8));
	}

	// モード選択からの設定変更でも、ルールと戻り先のカーソルを保つ。
	TEST(SceneCalcTest, Options_ModesKeepRuleAndReturnToOptionsRow)
	{
		SceneCalc scene;
		scene.Start();
		scene.Step({ true, false, 0 }, false);
		scene.Step({ true, false, 0 }, false);
		scene.CycleRule(1);
		ASSERT_EQ(GAME_RULE_DEBUG, scene.GetRule());
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
		EXPECT_EQ((int)SCENE_EVENT_HOST, (int)scene.Step({ true, false, 0 }, false));
		scene.OpenLobby();
		EXPECT_EQ((int)SCENE_TITLE, (int)scene.GetScene());
		scene.Step({ false, false, 0, true }, false);
		SelectMode(scene, static_cast<MODE_ITEM>(MODE_ITEM_JOIN - MODE_ITEM_HOST));
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

	// 試合中のメニュー：開く・閉じる、2つ目でキャラ選択、3つ目でタイトルへ。時間切れのあとは開かない
	TEST(SceneCalcTest, Pause_ResumesOrBacksToLobbyOrTitle)
	{
		SceneCalc scene;
		scene.Start(false);
		scene.Step({ true, false, 0 }, false); // モード選択へ
		scene.Step({ true, false, 0 }, false); // キャラ選択へ
		StartMatch(scene); // マップ選択を通って試合へ
		ASSERT_EQ((int)SCENE_MATCH, (int)scene.GetScene());
		SCENE_INPUT pause{ false, false, 0 };
		pause.Pause = true;
		EXPECT_EQ((int)SCENE_EVENT_NONE, (int)scene.Step(pause, false));
		EXPECT_EQ((int)SCENE_PAUSE, (int)scene.GetScene());
		EXPECT_EQ((int)SCENE_EVENT_NONE, (int)scene.Step(pause, false));
		EXPECT_EQ((int)SCENE_MATCH, (int)scene.GetScene());

		scene.Step(pause, false);
		scene.Step({ false, false, -1.0f }, false);
		EXPECT_EQ((int)SCENE_EVENT_BACK_LOBBY, (int)scene.Step({ true, false, 0 }, false));
		EXPECT_EQ((int)SCENE_TITLE, (int)scene.GetScene());

		StartMatch(scene); // もう一度試合へ
		scene.Step(pause, false);
		scene.Step({ false, false, -1.0f }, false);
		scene.Step({ false, false, 0 }, false);
		scene.Step({ false, false, -1.0f }, false);
		EXPECT_EQ((int)SCENE_EVENT_BACK_TITLE, (int)scene.Step({ true, false, 0 }, false));
		EXPECT_EQ((int)SCENE_GAME_START, (int)scene.GetScene());

		SceneCalc finished;
		finished.Start(true);
		StartMatch(finished);
		finished.Step(pause, true);
		EXPECT_EQ((int)SCENE_MATCH, (int)finished.GetScene());
	}
}
