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
		scene.Step({ false, false, -1 }, true); // TITLE を選んでいてもやり直せる
		EXPECT_EQ((int)SCENE_EVENT_START_MATCH, (int)scene.Step({ false, true, 0 }, true));
		EXPECT_EQ((int)SCENE_MATCH, (int)scene.GetScene());
	}

	TEST(SceneCalcTest, Result_DecideRetriesOrReturnsToTitle)
	{
		SceneCalc scene;
		scene.Start(true);
		scene.Step({ true, false, 0 }, false);
		scene.Step({ true, false, 0 }, true);
		EXPECT_EQ((int)SCENE_EVENT_START_MATCH, (int)scene.Step({ true, false, 0 }, true));
		EXPECT_EQ((int)SCENE_MATCH, (int)scene.GetScene());
		scene.Step({ true, false, 0 }, true);
		scene.Step({ false, false, -1 }, true);
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
		EXPECT_EQ((int)RESULT_ITEM_TITLE, scene.GetCursor());
		for (int i = 0; i < 3; ++i) scene.Step({ false, false, -1 }, true);
		scene.Step({ false, false, 1 }, true); // 戻さず逆に倒しても次の弾きにしない
		EXPECT_EQ((int)RESULT_ITEM_TITLE, scene.GetCursor());
		scene.Step({ false, false, 0.2f }, true);
		scene.Step({ false, false, 0.7f }, true);
		EXPECT_EQ((int)RESULT_ITEM_RETRY, scene.GetCursor());
	}

	TEST(SceneCalcTest, GameStart_CharacterSelectThenSoloAndLocalResultReturnsToGameStart)
	{
		SceneCalc scene;
		scene.Start();
		EXPECT_EQ((int)SCENE_GAME_START, (int)scene.GetScene());
		scene.Step({ true, false, 0 }, false);
		EXPECT_EQ(SCENE_CHARACTER_SELECT, scene.GetScene());
		scene.Step({ true, false, 0, false, false, 1 }, false);
		EXPECT_EQ(CHARACTER_MEKAGIRL, scene.GetCharacter());
		EXPECT_EQ(SCENE_MODE_SELECT, scene.GetScene());
		EXPECT_EQ(SCENE_EVENT_START_MATCH, scene.Step({ true, false, 0 }, false));
		scene.Step({ true, false, 0 }, true);
		scene.Step({ false, false, -1 }, true);
		EXPECT_EQ((int)SCENE_EVENT_BACK_TITLE, (int)scene.Step({ true, false, 0 }, true));
		EXPECT_EQ(SCENE_GAME_START, scene.GetScene());
		EXPECT_EQ(CHARACTER_MEKAGIRL, scene.GetCharacter());
	}

	// 左右で両端をまたぎ、戻ると GAME START を表示する。
	TEST(SceneCalcTest, CharacterSelect_WrapsInBothDirectionsAndBackReturnsToGameStart)
	{
		SceneCalc scene;
		scene.Start();
		scene.Step({ true, false, 0 }, false);
		scene.Step({ false, false, 0, false, false, -1 }, false);
		EXPECT_EQ(CHARACTER_MEKAGIRL, scene.GetCharacter());
		scene.Step({ false, false, 0, false, false, 1 }, false);
		EXPECT_EQ(CHARACTER_MEKA, scene.GetCharacter());
		scene.Step({ false, false, 0, true }, false);
		EXPECT_EQ(SCENE_GAME_START, scene.GetScene());
	}

	TEST(SceneCalcTest, Modes_LocalLobbyAndBackToGameStart)
	{
		SceneCalc scene;
		scene.ShowModes();
		SelectMode(scene, MODE_ITEM_LOCAL);
		scene.Step({ true, false, 0 }, false);
		EXPECT_EQ((int)SCENE_TITLE, (int)scene.GetScene());
		EXPECT_EQ((int)SCENE_EVENT_BACK_MENU, (int)scene.Step({ false, false, 0, true }, false));
		EXPECT_EQ((int)SCENE_MODE_SELECT, (int)scene.GetScene());
		SelectMode(scene, MODE_ITEM_BACK);
		scene.Step({ true, false, 0 }, false);
		EXPECT_EQ((int)SCENE_GAME_START, (int)scene.GetScene());
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
